SAMSUNG_LTE_TAP
================

In this repository, a USERLAND version of the samsung driver has been implemented:

1) switch/ is a usb modswitch for the dongle. you need to run it in order to get the dongle into a modem state
2) samsung_lte_tap/ is the implementation itself.

Compiling:
Make sure you have libusbx installed.
just run make

when doing lsusb, if you have the device insto storage mode:

$lsusb 
Bus 005 Device 010: ID 04e8:689a Samsung Electronics Co., Ltd LTE Storage Driver [CMC2xx]
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 002 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub


run ./switch

$lsusb 
Bus 005 Device 011: ID 04e8:6889 Samsung Electronics Co., Ltd GT-B3730 Composite LTE device (Commercial)
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 002 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub

the device should now be in "Commerial" mode.

You can now run the lte dongle with the APN name:

./lte -a "orange.fr"

or if you want debug info on a GSMTAP:

./lte -a "orange.fr" -d 192.168.0.1

where the IP is targetting the capturing machine (wireshark)

