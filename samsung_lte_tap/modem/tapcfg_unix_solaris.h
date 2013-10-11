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

#include <stropts.h>
#include <sys/sockio.h>

#include "dlpi.h"

#define TUNNEWPPA       (('T'<<16) | 0x0001)
#define TUNSETPPA       (('T'<<16) | 0x0002)


static int
tapcfg_plumb_device(taplog_t *taplog, int ppa, int *new_tap_fd, int *new_ip_fd, int *new_ip6_fd) {
#define IP_NODE  "/dev/udp"
#define IP6_NODE "/dev/udp6"
#define TAP_NODE "/dev/tap"
#define ARP_NODE "/dev/tap"
	struct lifreq lifr;
	struct strioctl strioc;
	char ifname[128];
	int newppa;
	int tap_fd = -1, ip_fd = -1, ip6_fd = -1;
	int if_fd = -1, arp_fd = -1;
	int ip_muxid = -1, arp_muxid = -1, ip6_muxid = -1;

	*new_tap_fd = *new_ip_fd = *new_ip6_fd = -1;

	ip_fd = open(IP_NODE, O_RDWR, 0);
	if (ip_fd < 0) {
		goto error;
	}

	ip6_fd = open(IP6_NODE, O_RDWR, 0);
	if (ip6_fd < 0) {
		goto error;
	}

	tap_fd = open(TAP_NODE, O_RDWR, 0);
	if (tap_fd < 0) {
		goto error;
	}

	strioc.ic_cmd = TUNNEWPPA;
	strioc.ic_timout = 0;
	strioc.ic_len = sizeof(ppa);
	strioc.ic_dp = (char *) &ppa;
	newppa = ioctl(tap_fd, I_STR, &strioc);
	if (newppa == -1) {
		goto error;
	}

	ifname[sizeof(ifname)-1] = '\0';
	snprintf(ifname, sizeof(ifname)-1, "tap%d", newppa);





	/* Setup IPv4 connectivity! */
	if_fd = open(TAP_NODE, O_RDWR, 0);
	if (if_fd < 0) {
		taplog_log(taplog, TAPLOG_ERR,
		           "Couldn't open interface device");
		goto error;
	}

	if (ioctl(if_fd, I_PUSH, "ip") < 0) {
		taplog_log(taplog, TAPLOG_ERR, "Error pushing the IP module");
		goto error;
	}

	memset(&lifr, 0, sizeof(struct lifreq));
	if (ioctl(if_fd, SIOCGLIFFLAGS, &lifr) == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Can't get interface flags");
		goto error;
	}

	strcpy(lifr.lifr_name, ifname);
	lifr.lifr_ppa = ppa;
	if (ioctl(if_fd, SIOCSLIFNAME, &lifr) == -1) {
		goto error;
	}

	if (ioctl(if_fd, I_PUSH, "arp") == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Error pushing the ARP module to TAP fd");
		goto error;
	}

	/* Push arp module to ip_fd */
	ioctl(ip_fd, I_POP, NULL);
	if (ioctl(ip_fd, I_PUSH, "arp") < 0) {
		taplog_log(taplog, TAPLOG_ERR, "Error pushing ARP module to IP fd");
		goto error;
	}

	arp_fd = open(ARP_NODE, O_RDWR, 0);
	if (arp_fd < 0) {
		taplog_log(taplog, TAPLOG_ERR, "Error opening ARP fd");
		goto error;
	}

	if (ioctl(arp_fd, I_PUSH, "arp") == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Error pushing the ARP module 2");
		goto error;
	}

	strioc.ic_cmd = SIOCSLIFNAME;
	strioc.ic_timout = 0;
	strioc.ic_len = sizeof(lifr);
	strioc.ic_dp = (char *) &lifr;
	if (ioctl(arp_fd, I_STR, &strioc) == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't set interface name to ARP fd");
		goto error;
	}

	ip_muxid = ioctl(ip_fd, I_PLINK, if_fd);
	if (ip_muxid == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't link tap device to IP");
		goto error;
	}
	close(if_fd);
	if_fd = -1;

	arp_muxid = ioctl(ip_fd, I_PLINK, arp_fd);
	if (arp_muxid == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't link tap device to ARP");
		goto error;
	}
	close(arp_fd);
	arp_fd = -1;

	memset(&lifr, 0, sizeof(struct lifreq));
	strcpy(lifr.lifr_name, ifname);
	lifr.lifr_ip_muxid = ip_muxid;
	lifr.lifr_arp_muxid = arp_muxid;

	if (ioctl(ip_fd, SIOCSLIFMUXID, &lifr) < 0) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't call SIOCSLIFMUXID");
		goto error;
	}





	/* Setup IPv6 connectivity! */
	if_fd = open(TAP_NODE, O_RDWR, 0);
	if (if_fd < 0) {
		taplog_log(taplog, TAPLOG_ERR,
		           "Couldn't open interface device for IPv6");
		goto error;
	}

	if (ioctl(if_fd, I_PUSH, "ip") < 0) {
		taplog_log(taplog, TAPLOG_ERR, "Error pushing the IP module for IPv6");
		goto error;
	}

	memset(&lifr, 0, sizeof(struct lifreq));
	if (ioctl(if_fd, SIOCGLIFFLAGS, &lifr) == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Can't get interface flags for IPv6");
		goto error;
	}

	strcpy(lifr.lifr_name, ifname);
	lifr.lifr_flags |= IFF_IPV6;
	lifr.lifr_flags &= ~(IFF_BROADCAST | IFF_IPV4);
	lifr.lifr_ppa = ppa;
	if (ioctl(if_fd, SIOCSLIFNAME, &lifr) == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't set interface name");
		goto error;
	}
	
	ip6_muxid = ioctl(ip6_fd, I_PLINK, if_fd);
	if (ip6_muxid == -1) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't link tap device to IP6");
		goto error;
	}
	close(if_fd);
	if_fd = -1;

	memset(&lifr, 0, sizeof(struct lifreq));
	strcpy(lifr.lifr_name, ifname);
	lifr.lifr_ip_muxid = ip6_muxid;

	if (ioctl(ip6_fd, SIOCSLIFMUXID, &lifr) < 0) {
		taplog_log(taplog, TAPLOG_ERR, "Couldn't call SIOCSLIFMUXID for IPv6");
		goto error;
	}





	/* Return the results */
	*new_tap_fd = tap_fd;
	*new_ip_fd = ip_fd;
	*new_ip6_fd = ip6_fd;

	return newppa;

error:
	if (arp_muxid != -1)
		ioctl(ip_fd, I_PUNLINK, arp_muxid);
	if (ip_muxid != -1)
		ioctl(ip_fd, I_PUNLINK, ip_muxid);
	if (ip6_muxid != -1)
		ioctl(ip6_fd, I_PUNLINK, ip6_muxid);
	if (arp_fd != -1)
		close(arp_fd);
	if (if_fd != -1)
		close(if_fd);
	if (ip_fd != -1)
		close(ip_fd);
	if (ip6_fd != -1)
		close(ip6_fd);
	if (tap_fd != -1)
		close(tap_fd);
	return -1;
}

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	int tap_fd, ip_fd, ip6_fd;
	int ppa, newppa;

	if (strncmp(ifname, "tap", 3)) {
		if (!fallback) {
			taplog_log(&tapcfg->taplog, TAPLOG_DEBUG,
				   "Device name '%s' doesn't start with 'tap'",
				   ifname);
			return -1;
		}
		ppa = 0;
	} else {
		char *endptr;

		ppa = strtol(ifname+3, &endptr, 10);
		if (*endptr != '\0') {
			if (!fallback)
				return -1;
			ppa = 0;
		}
	}

	newppa = tapcfg_plumb_device(&tapcfg->taplog, ppa, &tap_fd, &ip_fd, &ip6_fd);
	if (newppa == -1 && !fallback) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Couldn't open interface: tap%d",
		           ppa);
		return -1;
	} else if (newppa == -1) {
		int i;

		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Opening device '%s' failed, trying to find another one",
		           ifname);
		for (i=0; i<16; i++) {
			if (i == ppa)
				continue;

			newppa = tapcfg_plumb_device(&tapcfg->taplog, i, &tap_fd, &ip_fd, &ip6_fd); 
			if (newppa >= 0)
				break;
		}
		if (i == 16) {
			taplog_log(&tapcfg->taplog, TAPLOG_ERR,
			           "Couldn't find suitable tap device to assign");
			return -1;
		}
	}

	/* Set the device name to be the one we found finally */
	snprintf(tapcfg->ifname,
	         sizeof(tapcfg->ifname)-1,
	         "tap%d", newppa);
	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Device name %s", tapcfg->ifname);

	/* SIOCGENADDR doesn't work on Solaris, so we need to use the attached DLPI interface
	 * and use it to query the hardware address instead, also works for setting */
	if (dlpi_get_physaddr(tap_fd, tapcfg->hwaddr, sizeof(tapcfg->hwaddr)) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Couldn't query physical address from the DLPI interface: %s",
		           strerror(errno));
	}

	tapcfg->ip_fd = ip_fd;
	tapcfg->ip6_fd = ip6_fd;

	return tap_fd;
}

static void
tapcfg_stop_dev(tapcfg_t *tapcfg)
{
	struct lifreq lifr;
	int ip_fd, ip6_fd;

	assert(tapcfg);
	assert(tapcfg->ip_fd >= 0);
	assert(tapcfg->ip6_fd >= 0);

	ip_fd = tapcfg->ip_fd;
	ip6_fd = tapcfg->ip6_fd;

	memset(&lifr, 0, sizeof(struct lifreq));
	strcpy(lifr.lifr_name, tapcfg->ifname);
	if (ioctl(ip_fd, SIOCGLIFFLAGS, &lifr) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't get interface flags when closing");
		return;
	}
	if (ioctl(ip_fd, SIOCGLIFMUXID, &lifr) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't call SIOCGLIFMUXID when closing");
		return;
	}
	if (ioctl(ip_fd, I_PUNLINK, lifr.lifr_arp_muxid) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't unlink ARP muxid when closing");
		return;
	}
	if (ioctl(ip_fd, I_PUNLINK, lifr.lifr_ip_muxid) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't unlink IP muxid when closing");
		return;
	}

	memset(&lifr, 0, sizeof(struct lifreq));
	strcpy(lifr.lifr_name, tapcfg->ifname);
	if (ioctl(ip6_fd, SIOCGLIFFLAGS, &lifr) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't get interface flags when closing");
		return;
	}
	if (ioctl(ip6_fd, SIOCGLIFMUXID, &lifr) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't call SIOCGLIFMUXID when closing");
		return;
	}
	if (ioctl(ip6_fd, I_PUNLINK, lifr.lifr_ip_muxid) < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Can't unlink IP muxid when closing");
		return;
	}

	close(ip_fd);
	tapcfg->ip_fd = -1;
	close(ip6_fd);
	tapcfg->ip6_fd = -1;

	taplog_log(&tapcfg->taplog, TAPLOG_INFO, "Device stopped successfully");
}

static void
tapcfg_iface_prepare_ipv6(tapcfg_t *tapcfg, int flags)
{
	struct lifreq lifr;
	int ctrl_fd;

	ctrl_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (ctrl_fd < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Couldn't open control control socket for IPv6 status change");
		return;
	}

	memset(&lifr, 0, sizeof(lifr));
	strcpy(lifr.lifr_name, tapcfg->ifname);
	if (ioctl(ctrl_fd, SIOCGLIFFLAGS, &lifr) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error calling SIOCGIFFLAGS for interface %s: %s",
		           tapcfg->ifname,
		           strerror(errno));
		close(ctrl_fd);
		return;
	}

	if (flags & TAPCFG_STATUS_IPV6_UP) {
		lifr.lifr_flags |= IFF_UP;
	} else {
		lifr.lifr_flags &= ~IFF_UP;
	}

	if (ioctl(ctrl_fd, SIOCSLIFFLAGS, &lifr) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error calling SIOCSIFFLAGS for interface %s: %s",
		           tapcfg->ifname,
		           strerror(errno));
		close(ctrl_fd);
		return;
	}

	close(ctrl_fd);
}

static int
tapcfg_hwaddr_ioctl(tapcfg_t *tapcfg,
                    const char *hwaddr)
{
	int ret;

	ret = dlpi_set_physaddr(tapcfg->tap_fd, hwaddr, HWADDRLEN);
	if (ret < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
			   "Error trying to set new hardware address: %s",
			   strerror(errno));
		return -1;
	}

	return 0;
}

static int
tapcfg_ifaddr_ioctl(tapcfg_t *tapcfg,
                    unsigned int addr,
                    unsigned int mask)
{
	struct ifreq ifr;
	struct sockaddr_in *sin;

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);

	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mask;

	if (ioctl(tapcfg->ctrl_fd, SIOCSIFNETMASK, &ifr) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error trying to configure IPv4 netmask: %s",
		           strerror(errno));
		return -1;
	}

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);

	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;

	if (ioctl(tapcfg->ctrl_fd, SIOCSIFADDR, &ifr) == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error trying to configure IPv4 address: %s",
		           strerror(errno));
		return -1;
	}

	return 0;
}
