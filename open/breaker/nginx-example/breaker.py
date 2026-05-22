#!/usr/bin/python3
# /usr/local/bin/breaker.py — reconcile nginx + iptables to the gauge's
# cool-down decisions in /opt/open.ftorrent.com/data/breaker.json.
#
# Triggered by breaker.path on each gauge tick (~once per minute, whether
# or not breaker.json's content changed). The wall clock plus six startOn
# epoch-ms timestamps are the only inputs; live state — /etc/nginx/breaker.conf
# and the iptables/ip6tables raw PREROUTING DROP rules tagged "breaker-udp" —
# is the only cache.
#
# Per service: now > startOn means the service should be running ("on");
# now < startOn means paused ("off"); 0 is just a past time.
#
# Logs to journal (syslog ident "breaker") only on real transitions. The
# path unit fires ~1440 times/day — quiet runs are silent on purpose.

import json
import subprocess
import syslog
import sys
import time
from pathlib import Path

BREAKER_JSON = Path("/opt/open.ftorrent.com/data/breaker.json")
NGINX_CONF = Path("/etc/nginx/breaker.conf")
NGINX_CONF_BAK = Path("/etc/nginx/breaker.conf.bak")
NGINX_CONF_TMP = Path("/etc/nginx/breaker.conf.tmp")

# Absolute binary paths — eliminates any PATH-related surprise under systemd
# and makes the script self-documenting about which binaries it expects.
# iptables/ip6tables on Ubuntu are alternatives symlinks routing to nft or
# legacy; /usr/sbin/* is the stable entry point either way.
NGINX = "/usr/sbin/nginx"
IPTABLES = "/usr/sbin/iptables"
IP6TABLES = "/usr/sbin/ip6tables"

SERVICES = ("udp4", "udp6", "http4", "http6", "ws4", "ws6")

syslog.openlog(ident="breaker", facility=syslog.LOG_DAEMON)


def log(msg: str) -> None:
    syslog.syslog(syslog.LOG_INFO, msg)


def read_start_on() -> dict[str, int] | None:
    """Parse breaker.json. Returns None on any error (fail closed — leave
    live state alone rather than acting on bad input)."""
    try:
        data = json.loads(BREAKER_JSON.read_text())
        start_on = data["startOn"]
        result: dict[str, int] = {}
        for service in SERVICES:
            v = start_on[service]
            # bool is a subclass of int in Python; reject explicitly so a
            # stray `true`/`false` in breaker.json doesn't pass for a number.
            if isinstance(v, bool) or not isinstance(v, (int, float)):
                return None
            result[service] = int(v)
        return result
    except (FileNotFoundError, PermissionError, UnicodeDecodeError, json.JSONDecodeError, KeyError, TypeError):
        return None


def desired_states(start_on: dict[str, int], now_ms: int) -> dict[str, str]:
    """Per-service desired state: 'off' if startOn is in the future, else 'on'."""
    return {s: "off" if start_on[s] > now_ms else "on" for s in SERVICES}


def render_nginx_conf(s: dict[str, str]) -> str:
    """Generate the 4-line breaker.conf body. Whitespace must match the
    initial shipped breaker.conf byte-for-byte (3 spaces after $service_ws4
    and $service_ws6 to align quotes with http4/http6) so a no-op tick
    produces an identical comparison and no needless reload."""
    return (
        f'set $service_http4 "{s["http4"]}";\n'
        f'set $service_http6 "{s["http6"]}";\n'
        f'set $service_ws4   "{s["ws4"]}";\n'
        f'set $service_ws6   "{s["ws6"]}";\n'
    )


def reconcile_nginx(states: dict[str, str]) -> None:
    desired = render_nginx_conf(states)
    current = NGINX_CONF.read_text() if NGINX_CONF.exists() else ""
    if desired == current:
        return

    # Backup current (if present), atomic-rename the new content into place,
    # nginx -t. Restore from backup on test failure — defensive; should
    # never fire because the template is hardcoded.
    backed_up = False
    if NGINX_CONF.exists():
        NGINX_CONF_BAK.write_bytes(NGINX_CONF.read_bytes())
        backed_up = True

    NGINX_CONF_TMP.write_text(desired)
    NGINX_CONF_TMP.replace(NGINX_CONF)

    test = subprocess.run([NGINX, "-t"], capture_output=True)
    if test.returncode != 0:
        if backed_up:
            NGINX_CONF_BAK.replace(NGINX_CONF)
        sys.stderr.buffer.write(test.stderr)
        log("nginx -t failed on generated breaker.conf; reverted, no reload")
        return

    reload_result = subprocess.run([NGINX, "-s", "reload"], capture_output=True)
    if reload_result.returncode == 0:
        log(f"nginx applied: http4={states['http4']} http6={states['http6']} "
            f"ws4={states['ws4']} ws6={states['ws6']}")
    else:
        sys.stderr.buffer.write(reload_result.stderr)
        log("nginx -t passed but reload failed; new config in place, applies on next nginx start")

    if backed_up:
        NGINX_CONF_BAK.unlink(missing_ok=True)


def ipt_args(action: str) -> list[str]:
    """Argv shared between -C (check), -D (delete), -I (insert). The
    --comment "breaker-udp" tag is structural: -C uses it to find our
    own rule without false-positiving on someone else's."""
    return [
        "-w", "-t", "raw", action, "PREROUTING",
        "-p", "udp", "--dport", "443",
        "-m", "comment", "--comment", "breaker-udp",
        "-j", "DROP",
    ]


def udp_reconcile(cmd: str, desired: str, fam: str) -> None:
    """Ensure the raw PREROUTING DROP rule matches desired for one family.
    cmd is "iptables" (v4) or "ip6tables" (v6); they manage independent rule
    sets."""
    check = subprocess.run([cmd, *ipt_args("-C")], capture_output=True)
    present = check.returncode == 0
    if present and desired == "on":
        if subprocess.run([cmd, *ipt_args("-D")]).returncode == 0:
            log(f"{fam} DROP removed (service back on)")
        else:
            log(f"{fam} DROP removal failed")
    elif not present and desired == "off":
        if subprocess.run([cmd, *ipt_args("-I")]).returncode == 0:
            log(f"{fam} DROP inserted (service paused)")
        else:
            log(f"{fam} DROP insert failed")


def main() -> int:
    try:
        start_on = read_start_on()
        if start_on is None:
            log(f"missing or malformed {BREAKER_JSON}; no action")
            return 0

        now_ms = time.time_ns() // 1_000_000
        states = desired_states(start_on, now_ms)

        reconcile_nginx(states)
        udp_reconcile(IPTABLES,  states["udp4"], "udp4")
        udp_reconcile(IP6TABLES, states["udp6"], "udp6")
        return 0
    except Exception as e:
        log(f"unexpected error: {type(e).__name__}: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
