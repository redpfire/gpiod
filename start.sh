#!/bin/bash

# warning! this script cannot exit.

netctl stop-all
ip addr add 172.16.93.1/24 dev wlan0
systemctl start dnsmasq
hostapd /etc/hostapd/hostapd.conf

