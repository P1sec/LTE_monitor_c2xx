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

#include <net/if_dl.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#ifdef __NetBSD__
#  include <net/if_tap.h>
#endif

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	char buf[128];
	int tap_fd = -1;
	struct ifaddrs *ifa;

	buf[sizeof(buf)-1] = '\0';

	/* If we have a configured interface name, try that first */
	if (ifname && strlen(ifname) <= MAX_IFNAME && !strrchr(ifname, ' ')) {
		snprintf(buf, sizeof(buf)-1, "/dev/%s", ifname);
		tap_fd = open(buf, O_RDWR);

		/* Copy device name into buffer */
		memmove(buf, buf+5, sizeof(buf)-5);
	}
	if (tap_fd < 0 && fallback) {
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Opening device '%s' failed, trying to find another one",
		           ifname);

#ifdef __NetBSD__
		tap_fd = open("/dev/tap", O_RDWR);
		if (tap_fd >= 0) {
			struct ifreq ifr;

			memset(&ifr, 0, sizeof(struct ifreq));
			if (ioctl(tap_fd, TAPGIFNAME, &ifr) == -1) {
				taplog_log(&tapcfg->taplog, TAPLOG_ERR,
					   "Error getting the interface name: %s",
					   strerror(errno));
				return -1;
			}
			/* Copy device name into buffer */
			strncpy(buf, ifr.ifr_name, sizeof(buf)-1);
		}
#else
		{
			int i;

			/* Try all possible devices, because configured name failed */
			for (i=0; i<16; i++) {
				snprintf(buf, sizeof(buf)-1, "/dev/tap%u", i);
				tap_fd = open(buf, O_RDWR);
				if (tap_fd >= 0) {
					/* Found one! Copy device name into buffer */
					memmove(buf, buf+5, sizeof(buf)-5);
					break;
				}
			}
		}
#endif
	}
	if (tap_fd < 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
			   "Couldn't open the tap device \"%s\"", ifname);
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
			   "Check that you are running the program with "
			   "root privileges and have TUN/TAP driver installed");
		return -1;
	}

	/* Set the device name to be the one we found finally */
	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Device name %s", buf);
	strncpy(tapcfg->ifname, buf, sizeof(tapcfg->ifname)-1);

	/* Get MAC address on BSD, slightly trickier than Linux */
	if (getifaddrs(&ifa) == 0) {
		struct ifaddrs *curr;

		for (curr = ifa; curr; curr = curr->ifa_next) {
			if (!strcmp(curr->ifa_name, tapcfg->ifname) &&
			    curr->ifa_addr->sa_family == AF_LINK) {
				struct sockaddr_dl *sdp =
					(struct sockaddr_dl *) curr->ifa_addr;

				memcpy(tapcfg->hwaddr,
				       sdp->sdl_data + sdp->sdl_nlen,
				       HWADDRLEN);
			}
		}

		freeifaddrs(ifa);
	}

	return tap_fd;
}

static void
tapcfg_stop_dev(tapcfg_t *tapcfg)
{
	/* Nothing needed to cleanup here */
}

/* This is to accept router advertisements on KAME stack, these
 * functions are copied from usr.sbin/rtsold/if.c of OpenBSD */
#if defined(IPV6CTL_FORWARDING) && defined(IPV6CTL_ACCEPT_RTADV)
#include <sys/sysctl.h>

static int
getinet6sysctl(int code)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	int value;
	size_t size;

	mib[3] = code;
	size = sizeof(value);
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &value, &size, NULL, 0) < 0)
		return -1;
	else
		return value;
}

static int
setinet6sysctl(int code, int newval)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	int value;
	size_t size;

	mib[3] = code;
	size = sizeof(value);
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &value, &size,
	    &newval, sizeof(newval)) < 0)
		return -1;
	else
		return value;
}
#endif

#if defined(SIOCPROTOATTACH_IN6) && defined(SIOCLL_START)
#include <netinet/in_var.h>

/* This function is based on ip6tool.c of Darwin sources */
static int
tapcfg_attach_ipv6(const char *ifname)
{
	struct in6_aliasreq ifr;
	int s, err;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifra_name, ifname, sizeof(ifr.ifra_name));

	/* Attach IPv6 protocol to the interface */
	if ((err = ioctl(s, SIOCPROTOATTACH_IN6, &ifr)) != 0)
		return -1;

	/* Start acquiring linklocal address on the interface */
	if ((err = ioctl(s, SIOCLL_START, &ifr)) != 0)
		return -1;

	close(s);

	return 0;
}
#else
static int
tapcfg_attach_ipv6(const char *ifname)
{
	return 0;
}
#endif

static void
tapcfg_iface_prepare_ipv6(tapcfg_t *tapcfg, int flags)
{
	/* Do nothing if IPv6 is not enabled */
	if (!(flags & TAPCFG_STATUS_IPV6_UP))
		return;

#if defined(IPV6CTL_AUTO_LINKLOCAL)
	if (getinet6sysctl(IPV6CTL_AUTO_LINKLOCAL) == 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Setting sysctl net.inet6.ip6.auto_linklocal: 0 -> 1");
		setinet6sysctl(IPV6CTL_AUTO_LINKLOCAL, 1);
	}
#endif
	tapcfg_attach_ipv6(tapcfg->ifname);

	/* Return if route advertisements are not requested */
	if (!(flags & TAPCFG_STATUS_IPV6_RADV))
		return;

#if defined(IPV6CTL_FORWARDING) && defined(IPV6CTL_ACCEPT_RTADV)
	if (getinet6sysctl(IPV6CTL_FORWARDING) == 1) {
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Setting sysctl net.inet6.ip6.forwarding: 1 -> 0");
		setinet6sysctl(IPV6CTL_FORWARDING, 0);
	}
	if (getinet6sysctl(IPV6CTL_ACCEPT_RTADV) == 0) {
		taplog_log(&tapcfg->taplog, TAPLOG_INFO,
		           "Setting sysctl net.inet6.ip6.accept_rtadv: 0 -> 1");
		setinet6sysctl(IPV6CTL_ACCEPT_RTADV, 1);
	}
#endif
}

static int
tapcfg_hwaddr_ioctl(tapcfg_t *tapcfg,
                    const char *hwaddr)
{
	int ret = -1;
#if defined(SIOCSIFLLADDR)
	/* SIOCSIFLLADDR used in FreeBSD and OSX */
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ifr.ifr_addr.sa_len = HWADDRLEN;
	ifr.ifr_addr.sa_family = AF_LINK;
	memcpy(ifr.ifr_addr.sa_data, hwaddr, HWADDRLEN);

	ret = ioctl(tapcfg->ctrl_fd, SIOCSIFLLADDR, &ifr);
#elif defined(SIOCSIFPHYADDR)
	/* SIOCSIFPHYADDR used in NetBSD */
	struct ifaliasreq ifra;
	struct sockaddr_dl *sdl;

	memset(&ifra, 0, sizeof(struct ifaliasreq));
	strcpy(ifra.ifra_name, tapcfg->ifname);

	sdl = (struct sockaddr_dl *) &ifra.ifra_addr;
	sdl->sdl_family = AF_LINK;
	sdl->sdl_len = sizeof(struct sockaddr_dl);
	sdl->sdl_alen = HWADDRLEN;
	memcpy(LLADDR(sdl), hwaddr, HWADDRLEN);

	ret = ioctl(tapcfg->ctrl_fd, SIOCSIFPHYADDR, &ifra);
#endif
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
	struct ifreq ridreq;
	struct ifaliasreq addreq;
	struct sockaddr_in *sin;
	int ret;

	memset(&ridreq, 0, sizeof(struct ifreq));
	strcpy(ridreq.ifr_name, tapcfg->ifname);
	ret = ioctl(tapcfg->ctrl_fd, SIOCDIFADDR, &ridreq);
	if (ret == -1 && errno != EADDRNOTAVAIL) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		          "Error calling SIOCDIFADDR: %s",
		           strerror(errno));
		return -1;
	}

	memset(&addreq, 0, sizeof(struct ifaliasreq));
	strcpy(addreq.ifra_name, tapcfg->ifname);
	sin = (struct sockaddr_in *) &addreq.ifra_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin = (struct sockaddr_in *) &addreq.ifra_mask;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mask;
	sin->sin_len = sizeof(struct sockaddr_in);
	ret = ioctl(tapcfg->ctrl_fd, SIOCAIFADDR, &addreq);
	if (ret == -1) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error calling SIOCAIFADDR: %s",
		           strerror(errno));
		return -1;
	}

	return 0;
}
