#!/usr/bin/env python3
# Makes the three network namespaces moon's peers run in, with up, or deletes them, with down; namespaces.service runs it at boot and when stopped. A network namespace is a network stack of its own, with its own interfaces, addresses, routes, and sockets, so a program inside one sees only what's there. Each is joined to the box by a veth pair, two virtual Ethernet interfaces wired to each other, one end on the box named veth-<namespace> and the other inside named eth0. The box's end is the namespace's router, at .1 of its private /24, and the peer is at .2, so everything a peer sends crosses to the box, where the rules in rules.v4 and rules.v6 decide what passes and how it's translated.
# cone and symmetric are IPv4 alone, translated by the two masquerade rules in rules.v4. firewall is dual-stack: IPv4 behind a plain masquerade, so its peer can look up names and moon's Go service can reach it, and IPv6 with an address of its own from moon's /64, which nothing translates and rules.v6 filters statefully, the shape of a home router's IPv6 side. That address is the /64 moon's own ::1 sits in, with ::6 at the end, worked out here at boot rather than written in any file. Two more things make it reachable: a route on the box sending it down the veth pair, and a proxy entry for neighbor discovery on ens2, so when the provider's router asks who has the address on the link, the box answers for the namespace.
import ipaddress#the /64, and an address within it
import json#reading ip's answers in JSON
import subprocess#running ip
import sys#up or down, and the exit
import time#waiting for ::1

namespaces = [#name, the first three parts of its private IPv4 /24, and whether it gets an IPv6 address of its own
	('cone',      '10.88.1', False),
	('symmetric', '10.88.2', False),
	('firewall',  '10.88.3', True),
]
interface = 'ens2'#the provider's interface, the one cloud-init's netplan drop-in names
suffix = 6#the IPv6 peer's address within moon's /64: ::6, beside moon's own ::1

def ip(*words, check=True):#runs ip with these words, stopping the script if it fails unless check is False
	subprocess.run(['ip', *words], check=check)

def network():#moon's /64, from the address ending ::1 on ens2, which networkd forms from the router's advertisement; waits up to a minute for it, since at boot the advertisement can come after the rest of the network is up
	for _ in range(60):
		answer = subprocess.run(['ip', '-6', '-json', 'address', 'show', 'dev', interface, 'scope', 'global'], capture_output=True, text=True, check=True).stdout
		for link in json.loads(answer):
			for address in link.get('addr_info', []):
				if 'local' in address and address.get('prefixlen') == 64 and address['local'].endswith('::1'):
					return ipaddress.IPv6Network(f"{address['local']}/64", strict=False)
		time.sleep(1)
	sys.exit(f'no ::1 address on {interface} after a minute')

def up():
	for name, subnet, ipv6 in namespaces:
		ip('netns', 'delete', name, check=False)#one an earlier start left behind, if any
		ip('netns', 'add', name)#a namespace, named by a file in /run/netns that a peer's unit points NetworkNamespacePath= at
		ip('link', 'add', f'veth-{name}', 'type', 'veth', 'peer', 'name', 'eth0', 'netns', name)#the pair, its far end made straight inside
		ip('address', 'add', f'{subnet}.1/24', 'dev', f'veth-{name}')
		ip('link', 'set', f'veth-{name}', 'up')
		ip('-n', name, 'address', 'add', f'{subnet}.2/24', 'dev', 'eth0')#-n runs the command in the named namespace
		ip('-n', name, 'link', 'set', 'eth0', 'up')
		ip('-n', name, 'link', 'set', 'lo', 'up')#a new namespace keeps its loopback down
		ip('-n', name, 'route', 'add', 'default', 'via', f'{subnet}.1')
		if ipv6:
			address = str(network()[suffix])
			ip('-6', 'address', 'add', 'fe80::1/64', 'dev', f'veth-{name}')#a link-local address on the box's end that the namespace can name as its router
			ip('-n', name, '-6', 'address', 'add', f'{address}/128', 'dev', 'eth0', 'nodad')#the peer's public address; nodad skips the second of duplicate-address detection, since the box answers for the address on the link and nothing else can hold it
			ip('-n', name, '-6', 'route', 'add', 'default', 'via', 'fe80::1', 'dev', 'eth0')
			ip('-6', 'route', 'add', f'{address}/128', 'dev', f'veth-{name}')#on the box, the way to the address is down the veth pair
			ip('-6', 'neigh', 'del', 'proxy', address, 'dev', interface, check=False)#one an earlier start left behind, if any, as with the namespace
			ip('-6', 'neigh', 'add', 'proxy', address, 'dev', interface)#and on the link, the box answers neighbor discovery for it

def down():
	for name, subnet, ipv6 in namespaces:
		ip('netns', 'delete', name, check=False)#a namespace goes once nothing runs in it, and its veth pair and the route down it go with it
	answer = subprocess.run(['ip', '-6', '-json', 'neigh', 'show', 'proxy', 'dev', interface], capture_output=True, text=True).stdout
	for entry in json.loads(answer or '[]'):#ip prints nothing at all when there are none
		ip('-6', 'neigh', 'del', 'proxy', entry['dst'], 'dev', interface, check=False)#the proxy entries, which outlive the namespace

if sys.argv[1:] == ['up']:
	up()
elif sys.argv[1:] == ['down']:
	down()
else:
	sys.exit('namespaces.py up, or namespaces.py down')
