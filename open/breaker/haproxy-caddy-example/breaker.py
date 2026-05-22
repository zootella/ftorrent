#!/usr/bin/python3
# /usr/local/bin/breaker.py — reconcile HAProxy + iptables to the gauge's
# cool-down decisions in /opt/open.ftorrent.com/data/breaker.json.
#
# Triggered by breaker.path on each gauge tick (~once per minute, whether
# or not breaker.json's content changed). The wall clock plus six startOn
# epoch-ms timestamps are the only inputs; live state — the HAProxy breaker
# map on disk (/etc/haproxy/maps/breaker.map) and in-memory (via the runtime
# socket), and the iptables/ip6tables raw PREROUTING DROP rules tagged
# "breaker-udp" — is the only cache.
#
# Per service: now > startOn means the service should be running ("on");
# now < startOn means paused ("off"); 0 is just a past time.
#
# For each of the four TCP services (http4, http6, ws4, ws6): the disk map
# file is rewritten atomically (temp + rename, so HAProxy can't read a torn
# version on reload), then the runtime socket is pushed so the in-memory
# state matches without a reload. For the two UDP services: idempotent
# add/remove of a raw PREROUTING DROP rule per family — the kernel drops
# packets before they ever reach userspace.
#
# Logs to journal (syslog ident "breaker") only on real transitions. The
# path unit fires ~1440 times/day — quiet runs are silent on purpose.

import json
import socket
import subprocess
import syslog
import sys
import time
from pathlib import Path

BREAKER_JSON = Path("/opt/open.ftorrent.com/data/breaker.json")
BREAKER_MAP = Path("/etc/haproxy/maps/breaker.map")
BREAKER_MAP_TMP = Path("/etc/haproxy/maps/breaker.map.tmp")
HAPROXY_SOCK = "/run/haproxy/admin.sock"

# Absolute binary paths — eliminates any PATH-related surprise under systemd
# and makes the script self-documenting about which binaries it expects.
# iptables/ip6tables on Ubuntu are alternatives symlinks routing to nft or
# legacy; /usr/sbin/* is the stable entry point either way.
IPTABLES = "/usr/sbin/iptables"
IP6TABLES = "/usr/sbin/ip6tables"

SERVICES = ("udp4", "udp6", "http4", "http6", "ws4", "ws6")
TCP_SERVICES = ("http4", "http6", "ws4", "ws6")

syslog.openlog(ident="breaker", facility=syslog.LOG_DAEMON)


def log_info(msg: str) -> None:
    syslog.syslog(syslog.LOG_INFO, msg)


def log_error(msg: str) -> None:
    syslog.syslog(syslog.LOG_ERR, msg)


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
            if isinstance(v, bool):
                return None
            if not isinstance(v, (int, float)):
                return None
            result[service] = int(v)
        return result
    except (FileNotFoundError, PermissionError, UnicodeDecodeError,
            json.JSONDecodeError, KeyError, TypeError):
        return None


def desired_states(start_on: dict[str, int], now_ms: int) -> dict[str, str]:
    """Per-service desired state: 'off' if startOn is in the future, else 'on'."""
    return {s: "off" if start_on[s] > now_ms else "on" for s in SERVICES}


def render_map(states: dict[str, str]) -> str:
    """The seven-line breaker.map content (one header + six data rows).
    Stable line order — matches the initial breaker.map shipped at install
    time so a no-op tick produces byte-identical content and no needless
    write. The header survives rewrites because we emit it every time;
    parse_map ignores it because it doesn't split into exactly two columns."""
    return (
        "# /etc/haproxy/maps/breaker.map — circuit-breaker state, one row per service\n"
        + "".join(f"{s} {states[s]}\n" for s in SERVICES)
    )


def parse_map(content: str) -> dict[str, str]:
    """Parse the two-column breaker.map content into {service: state}."""
    result: dict[str, str] = {}
    for line in content.splitlines():
        parts = line.split()
        if len(parts) == 2:
            result[parts[0]] = parts[1]
    return result


def haproxy_runtime(cmd: str) -> str:
    """Send one command to HAProxy's admin socket and return the reply.
    Connect, write command + newline, drain reply, close."""
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.settimeout(5)
        s.connect(HAPROXY_SOCK)
        s.sendall(cmd.encode() + b"\n")
        chunks: list[bytes] = []
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            chunks.append(chunk)
    return b"".join(chunks).decode(errors="replace")


def reconcile_haproxy(states: dict[str, str]) -> None:
    """Reconcile the HAProxy breaker map to the four TCP services' desired
    state. Atomic disk rewrite first (so a reload picks up the right state),
    then a runtime-socket push per changed service (so the live process sees
    the change without a reload)."""
    desired = render_map(states)
    current = BREAKER_MAP.read_text() if BREAKER_MAP.exists() else ""
    if desired == current:
        return

    BREAKER_MAP_TMP.write_text(desired)
    BREAKER_MAP_TMP.replace(BREAKER_MAP)

    # Push to the live process — one set map command per TCP service whose
    # state differs from what was on disk pre-rewrite. The disk file was
    # authoritative before this run and is authoritative after; the socket
    # push exists to spare HAProxy a reload.
    pre = parse_map(current)
    for service in TCP_SERVICES:
        if pre.get(service) != states[service]:
            reply = haproxy_runtime(f"set map {BREAKER_MAP} {service} {states[service]}")
            # HAProxy runtime API returns empty on success; non-empty = error.
            if reply.strip():
                log_error(f"haproxy set map {service} {states[service]}: {reply.strip()}")

    log_info(f"haproxy applied: http4={states['http4']} http6={states['http6']} "
             f"ws4={states['ws4']} ws6={states['ws6']}")


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
            log_info(f"{fam} DROP removed (service back on)")
        else:
            log_error(f"{fam} DROP removal failed")
    elif not present and desired == "off":
        if subprocess.run([cmd, *ipt_args("-I")]).returncode == 0:
            log_info(f"{fam} DROP inserted (service paused)")
        else:
            log_error(f"{fam} DROP insert failed")


def main() -> int:
    try:
        start_on = read_start_on()
        if start_on is None:
            log_error(f"missing or malformed {BREAKER_JSON}; no action")
            return 0

        now_ms = time.time_ns() // 1_000_000
        states = desired_states(start_on, now_ms)

        reconcile_haproxy(states)
        udp_reconcile(IPTABLES,  states["udp4"], "udp4")
        udp_reconcile(IP6TABLES, states["udp6"], "udp6")
        return 0
    except Exception as e:
        log_error(f"unexpected error: {type(e).__name__}: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
