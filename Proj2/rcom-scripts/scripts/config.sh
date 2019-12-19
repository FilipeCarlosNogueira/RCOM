#!/bin/bash

# pc 1

ifconfig eth0 up
ifconfig eth0 172.16.${bench}0.1/24

route add -net 172.16.${bench}1.0/24 gw 172.16.${bench}0.254
route add default gw 172.16.${bench}0.254

echo "search netlab.fe.up.pt" > /etc/resolv.conf
echo "nameserver 172.16.1.1" >> /etc/resolv.conf

#pc 2

ifconfig eth0 up
ifconfig eth0 172.16.${bench}1.1/24

route add -net 172.16.${bench}0.0/24 gw 172.16.${bench}1.253
route add default gw 172.16.${bench}1.254

echo "search netlab.fe.up.pt" > /etc/resolv.conf
echo "nameserver 172.16.1.1" >> /etc/resolv.conf

#pc 4

ifconfig eth0 up
ifconfig eth0 172.16.${bench}0.254/24

ifconfig eth1 up
ifconfig eth1 172.16.${bench}1.253/24

echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts

route add default gw 172.16.${bench}1.254

echo "search netlab.fe.up.pt" > /etc/resolv.conf
echo "nameserver 172.16.1.1" >> /etc/resolv.conf

