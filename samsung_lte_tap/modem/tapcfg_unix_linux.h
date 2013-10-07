/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2011  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <linux/if_tun.h>
#include <net/if_arp.h>

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	int tap_fd = -1;
	struct ifreq ifr;
	int s, ret;

	/* Create a new tap device */
	tap_fd = open("/dev/net/tun", O_RDWR);
	if (tap_fd == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error opening device /dev/net/tun: %s",
		           strerror(errno));
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Check that you are running the program with "
		           "root privileges");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (ifname && strlen(ifname) < IFNAMSIZ) {
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
	}
	ret = ioctl(tap_fd, TUNSETIFF, &ifr);

	if (ret == -1 && errno == EINVAL && fallback) {
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Opening device '%s' failed, trying to find another one",
		           ifname);
		/* Try again without device name */
		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
		ret = ioctl(tap_fd, TUNSETIFF, &ifr);
	}
	if (ret == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error setting the interface \"%s\": %s",
		           ifname, strerror(errno));
		close(tap_fd);
		return -1;
	}

	/* Set the device name to be the one we got from OS */
	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Device name %s", ifr.ifr_name);
	strncpy(tapcfg->ifname, ifr.ifr_name, sizeof(tapcfg->ifname)-1);

	/* Create a temporary socket for SIOCGIFHWADDR */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (!s) {
		close(tap_fd);
		return -1;
	}

	/* Get the hardware address of the TAP interface */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ret = ioctl(s, SIOCGIFHWADDR, &ifr);
	if (ret == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error getting the hardware address: %s",
		           strerror(errno));
		close(tap_fd);
		close(s);
		return -1;
	}
	memcpy(tapcfg->hwaddr, ifr.ifr_hwaddr.sa_data, HWADDRLEN);
	close(s);

	return tap_fd;
}

static void
tapcfg_stop_dev(tapcfg_t *tapcfg)
{
	/* Nothing needed to cleanup here */
}

static void
tapcfg_iface_prepare_ipv6(tapcfg_t *tapcfg, int flags)
{
	/* No IPv6 preparation needed on Linux */
}

static int
tapcfg_hwaddr_ioctl(tapcfg_t *tapcfg,
                    const char *hwaddr)
{
	struct ifreq ifr;
	int ret;

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifr.ifr_hwaddr.sa_data, hwaddr, HWADDRLEN);

	ret = ioctl(tapcfg->ctrl_fd, SIOCSIFHWADDR, &ifr);
	if (ret == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
			   "Error trying to set new hardware address: %s",
			   strerror(errno));
	}

	return ret;
}

static int
tapcfg_ifaddr_ioctl(tapcfg_t *tapcfg,
                    unsigned int addr,
                    unsigned int mask)
{
	struct ifreq ifr;
	struct sockaddr_in *sin;
	struct in_addr *in_addr;

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);

	ifr.ifr_addr.sa_family = AF_INET;
	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	in_addr = &sin->sin_addr;
	in_addr->s_addr = addr;

	if (ioctl(tapcfg->ctrl_fd, SIOCSIFADDR, &ifr) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error trying to configure IPv4 address: %s",
		           strerror(errno));
		return -1;
	}

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);

	ifr.ifr_addr.sa_family = AF_INET;
	sin = (struct sockaddr_in *) &ifr.ifr_netmask;
	in_addr = &sin->sin_addr;
	in_addr->s_addr = mask;

	if (ioctl(tapcfg->ctrl_fd, SIOCSIFNETMASK, &ifr) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error trying to configure IPv4 netmask: %s",
		           strerror(errno));
		return -1;
	}

	return 0;
}

