/*
 * USB network interface driver for Samsung Kalmia based LTE USB modem like the
 * Samsung GT-B3730 and GT-B3710.
 *
 * Copyright (C) 2011 Marius Bjoernstad Kotsbak <marius@kotsbak.com>
 *
 * Sponsored by Quicklink Video Distribution Services Ltd.
 *
 * Based on the cdc_eem module.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ctype.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/crc32.h>
#include <linux/usb/cdc.h>
#include <linux/usb/usbnet.h>
#include <linux/gfp.h>

/*
 * The Samsung Kalmia based LTE USB modems have a CDC ACM port for modem control
 * handled by the "option" module and an ethernet data port handled by this
 * module.
 *
 * The stick must first be switched into modem mode by usb_modeswitch
 * or similar tool. Then the modem gets sent two initialization packets by
 * this module, which gives the MAC address of the device. User space can then
 * connect the modem using AT commands through the ACM port and then use
 * DHCP on the network interface exposed by this module. Network packets are
 * sent to and from the modem in a proprietary format discovered after watching
 * the behavior of the windows driver for the modem.
 *
 * More information about the use of the modem is available in usb_modeswitch
 * forum and the project page:
 *
 * http://www.draisberghof.de/usb_modeswitch/bb/viewtopic.php?t=465
 * https://github.com/mkotsbak/Samsung-GT-B3730-linux-driver
 */

/* #define	DEBUG */
/* #define	VERBOSE */

#define KALMIA_HEADER_LENGTH 6
#define KALMIA_ALIGN_SIZE 4
#define KALMIA_USB_TIMEOUT 10000



// ==============================================================================
#include <linux/kfifo.h>
#define FIFO_SIZE       16384

/* lock for procfs read access */
static DEFINE_MUTEX(read_lock);

static DECLARE_KFIFO(c2xx_fifo, unsigned char, FIFO_SIZE);

// XM - no need to tell c2xx_debug parameter for now
static int c2xx_debug = 1;	
module_param (c2xx_debug, int, 0);
MODULE_PARM_DESC (c2xx_debug, "debug mode on");

static int c2xx_init(struct usbnet *dev);
#define C2XX_IOC_MAGIC 0x92
#define C2XX_IOCX_MFETCH _IOWR(C2XX_IOC_MAGIC, 7, struct c2xx_bin_mfetch)

#define C2XX_START	0x55AA55AA

struct c2xx_bin_mfetch {
        u32 nfetch;
        u32 nlength;
        u8 __user *packet;
};


// ==============================================================================

/*-------------------------------------------------------------------------*/

static int
kalmia_send_init_packet(struct usbnet *dev, u8 *init_msg, u8 init_msg_len,
	u8 *buffer, u8 expected_len)
{
	int act_len;
	int status;

	netdev_dbg(dev->net, "Sending init packet");

	status = usb_bulk_msg(dev->udev, usb_sndbulkpipe(dev->udev, 0x02),
		init_msg, init_msg_len, &act_len, KALMIA_USB_TIMEOUT);
	if (status != 0) {
		netdev_err(dev->net,
			"Error sending init packet. Status %i, length %i\n",
			status, act_len);
		return status;
	}
	else if (act_len != init_msg_len) {
		netdev_err(dev->net,
			"Did not send all of init packet. Bytes sent: %i",
			act_len);
	}
	else {
		netdev_dbg(dev->net, "Successfully sent init packet.");
	}

	status = usb_bulk_msg(dev->udev, usb_rcvbulkpipe(dev->udev, 0x81),
		buffer, expected_len, &act_len, KALMIA_USB_TIMEOUT);

	if (status != 0)
		netdev_err(dev->net,
			"Error receiving init result. Status %i, length %i\n",
			status, act_len);
	else if (act_len != expected_len && !c2xx_debug)
		netdev_err(dev->net, "Unexpected init result length: %i\n",
			act_len);

	return status;
}

static int
kalmia_init_and_get_ethernet_addr(struct usbnet *dev, u8 *ethernet_addr)
{
	static const char init_msg_1[] =
		{ 0x57, 0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
		0x00, 0x00 };
	static const char init_msg_2[] =
		{ 0x57, 0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xf4,
		0x00, 0x00 };
	static const int buflen = 28;
	char *usb_buf;
	int status;

	printk(KERN_INFO "kalmia_init_and_get_ethernet_addr\n");

	usb_buf = kmalloc(buflen, GFP_DMA | GFP_KERNEL);
	if (!usb_buf)
		return -ENOMEM;

	memcpy(usb_buf, init_msg_1, 12);
	status = kalmia_send_init_packet(dev, usb_buf, sizeof(init_msg_1)
		/ sizeof(init_msg_1[0]), usb_buf, 24);
	if (status != 0)
		return status;

	memcpy(usb_buf, init_msg_2, 12);
	status = kalmia_send_init_packet(dev, usb_buf, sizeof(init_msg_2)
		/ sizeof(init_msg_2[0]), usb_buf, 28);
	if (status != 0)
		return status;

	memcpy(ethernet_addr, usb_buf + 10, ETH_ALEN);

	kfree(usb_buf);

	if(c2xx_debug)
		c2xx_init(dev);

	return status;
}

static int
kalmia_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int status;
	u8 ethernet_addr[ETH_ALEN];

	/* Don't bind to AT command interface */
	if (intf->cur_altsetting->desc.bInterfaceClass != USB_CLASS_VENDOR_SPEC)
		return -EINVAL;

	dev->in = usb_rcvbulkpipe(dev->udev, 0x81 & USB_ENDPOINT_NUMBER_MASK);
	dev->out = usb_sndbulkpipe(dev->udev, 0x02 & USB_ENDPOINT_NUMBER_MASK);
	dev->status = NULL;

	dev->net->hard_header_len += KALMIA_HEADER_LENGTH;
	dev->hard_mtu = 1400;
	dev->rx_urb_size = dev->hard_mtu * 10; // Found as optimal after testing

	status = kalmia_init_and_get_ethernet_addr(dev, ethernet_addr);

	if (status < 0) {
		usb_set_intfdata(intf, NULL);
		usb_driver_release_interface(driver_of(intf), intf);
		return status;
	}

	memcpy(dev->net->dev_addr, ethernet_addr, ETH_ALEN);
	memcpy(dev->net->perm_addr, ethernet_addr, ETH_ALEN);

	return status;
}

static struct sk_buff *
kalmia_tx_fixup(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	struct sk_buff *skb2 = NULL;
	u16 content_len;
	unsigned char *header_start;
	unsigned char ether_type_1, ether_type_2;
	u8 remainder, padlen = 0;

	if (!skb_cloned(skb)) {
		int headroom = skb_headroom(skb);
		int tailroom = skb_tailroom(skb);

		if ((tailroom >= KALMIA_ALIGN_SIZE) && (headroom
			>= KALMIA_HEADER_LENGTH))
			goto done;

		if ((headroom + tailroom) > (KALMIA_HEADER_LENGTH
			+ KALMIA_ALIGN_SIZE)) {
			skb->data = memmove(skb->head + KALMIA_HEADER_LENGTH,
				skb->data, skb->len);
			skb_set_tail_pointer(skb, skb->len);
			goto done;
		}
	}

	skb2 = skb_copy_expand(skb, KALMIA_HEADER_LENGTH,
		KALMIA_ALIGN_SIZE, flags);
	if (!skb2)
		return NULL;

	dev_kfree_skb_any(skb);
	skb = skb2;

done:
	header_start = skb_push(skb, KALMIA_HEADER_LENGTH);
	ether_type_1 = header_start[KALMIA_HEADER_LENGTH + 12];
	ether_type_2 = header_start[KALMIA_HEADER_LENGTH + 13];

	netdev_dbg(dev->net, "Sending etherType: %02x%02x", ether_type_1,
		ether_type_2);

	/* According to empiric data for data packages */
	header_start[0] = 0x57;
	header_start[1] = 0x44;
	content_len = skb->len - KALMIA_HEADER_LENGTH;

	put_unaligned_le16(content_len, &header_start[2]);
	header_start[4] = ether_type_1;
	header_start[5] = ether_type_2;

	/* Align to 4 bytes by padding with zeros */
	remainder = skb->len % KALMIA_ALIGN_SIZE;
	if (remainder > 0) {
		padlen = KALMIA_ALIGN_SIZE - remainder;
		memset(skb_put(skb, padlen), 0, padlen);
	}

	netdev_dbg(
		dev->net,
		"Sending package with length %i and padding %i. Header: %02x:%02x:%02x:%02x:%02x:%02x.",
		content_len, padlen, header_start[0], header_start[1],
		header_start[2], header_start[3], header_start[4],
		header_start[5]);

	return skb;
}

static int
kalmia_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	int ret;

	/*
	 * Our task here is to strip off framing, leaving skb with one
	 * data frame for the usbnet framework code to process.
	 */
	static const u8 HEADER_END_OF_USB_PACKET[] =
		{ 0x57, 0x5a, 0x00, 0x00, 0x08, 0x00 };
	static const u8 EXPECTED_UNKNOWN_HEADER_1[] =
		{ 0x57, 0x43, 0x1e, 0x00, 0x15, 0x02 };
	static const u8 EXPECTED_UNKNOWN_HEADER_2[] =
		{ 0x57, 0x50, 0x0e, 0x00, 0x00, 0x00 };
	int i = 0;
	unsigned int c2xx_start = C2XX_START;

	/* incomplete header? */
	if (skb->len < KALMIA_HEADER_LENGTH)
		return 0;

	do {
		struct sk_buff *skb2 = NULL;
		u8 *header_start;
		u16 usb_packet_length, ether_packet_length;
		int is_last;

		header_start = skb->data;

		if (unlikely(header_start[0] != 0x57 || header_start[1] != 0x44)) {
			if (!memcmp(header_start, EXPECTED_UNKNOWN_HEADER_1,
				sizeof(EXPECTED_UNKNOWN_HEADER_1)) || !memcmp(
				header_start, EXPECTED_UNKNOWN_HEADER_2,
				sizeof(EXPECTED_UNKNOWN_HEADER_2))) {
				netdev_dbg(
					dev->net,
					"Received expected unknown frame header: %02x:%02x:%02x:%02x:%02x:%02x. Package length: %i\n",
					header_start[0], header_start[1],
					header_start[2], header_start[3],
					header_start[4], header_start[5],
					skb->len - KALMIA_HEADER_LENGTH);
			}
			else {
				if(kfifo_avail(&c2xx_fifo) >= ( sizeof(c2xx_start) + sizeof(skb->len) + skb->len ) ){
					ret = kfifo_in(&c2xx_fifo, (u8*)&c2xx_start, sizeof(c2xx_start));
					if(ret != sizeof(skb->len)) {
						printk(KERN_INFO "c2xx unable to kfifo_in c2xx_start\n");
						return 0;
					}
					ret = kfifo_in(&c2xx_fifo, (u8*)&(skb->len), sizeof(skb->len));
					// when kfifo is full...
					if(ret != sizeof(skb->len)) {
						printk(KERN_INFO "c2xx unable to kfifo_in len\n");
						return 0;
					}
					ret = kfifo_in(&c2xx_fifo, skb->data, skb->len);
					if(ret != skb->len) {
						printk(KERN_INFO "c2xx unable to kfifo_in skb->len\n");
						return 0;
					}
					return 1;
				} else {
					return 0;
				}
			}
		}
		else
			netdev_dbg(
				dev->net,
				"Received header: %02x:%02x:%02x:%02x:%02x:%02x. Package length: %i\n",
				header_start[0], header_start[1], header_start[2],
				header_start[3], header_start[4], header_start[5],
				skb->len - KALMIA_HEADER_LENGTH);

		/* subtract start header and end header */
		usb_packet_length = skb->len - (2 * KALMIA_HEADER_LENGTH);
		ether_packet_length = get_unaligned_le16(&header_start[2]);
		skb_pull(skb, KALMIA_HEADER_LENGTH);

		/* Some small packets misses end marker */
		if (usb_packet_length < ether_packet_length) {
			ether_packet_length = usb_packet_length
				+ KALMIA_HEADER_LENGTH;
			is_last = true;
		}
		else {
			netdev_dbg(dev->net, "Correct package length #%i", i
				+ 1);

			is_last = (memcmp(skb->data + ether_packet_length,
				HEADER_END_OF_USB_PACKET,
				sizeof(HEADER_END_OF_USB_PACKET)) == 0);
			if (!is_last) {
				header_start = skb->data + ether_packet_length;
				netdev_dbg(
					dev->net,
					"End header: %02x:%02x:%02x:%02x:%02x:%02x. Package length: %i\n",
					header_start[0], header_start[1],
					header_start[2], header_start[3],
					header_start[4], header_start[5],
					skb->len - KALMIA_HEADER_LENGTH);
			}
		}

		if (is_last) {
			skb2 = skb;
		}
		else {
			skb2 = skb_clone(skb, GFP_ATOMIC);
			if (unlikely(!skb2))
				return 0;
		}

		skb_trim(skb2, ether_packet_length);

		if (is_last) {
			return 1;
		}
		else {
			usbnet_skb_return(dev, skb2);
			skb_pull(skb, ether_packet_length);
		}

		i++;
	}
	while (skb->len);

	return 1;
}

static const struct driver_info kalmia_info = {
	.description = "Samsung Kalmia LTE USB dongle",
	.flags = FLAG_WWAN,
	.bind = kalmia_bind,
	.rx_fixup = kalmia_rx_fixup,
	.tx_fixup = kalmia_tx_fixup
};

/******************************************************************************/
static long c2xx_ioctl(struct file *file,
		 unsigned int ioctl_num,
		 unsigned long ioctl_param)
{
	unsigned int len;
	int ret;
	int c2xx_start;

	struct c2xx_bin_mfetch mfetch;
	struct c2xx_bin_mfetch __user *uptr;
	unsigned int copied;


	switch(ioctl_num) {
		case C2XX_IOCX_MFETCH:
			uptr = (struct c2xx_bin_mfetch __user *)ioctl_param;
			if (copy_from_user(&mfetch, uptr, sizeof(mfetch))) {
				return -EFAULT;
			}
			if (mutex_lock_interruptible(&read_lock))
				return -ERESTARTSYS;
//			while (kfifo_avail(&c2xx_fifo) > sizeof(len)) {
			while (1) {
				//printk(KERN_INFO "c2xx available %d", kfifo_avail(&c2xx_fifo));
				//ret = kfifo_out_peek(&c2xx_fifo, &len, sizeof(len) );
				ret = kfifo_out(&c2xx_fifo, &c2xx_start, sizeof(c2xx_start) );
				if (ret != sizeof(c2xx_start)) {
					mutex_unlock(&read_lock);
					return -EINVAL;
				}
				// bad start
				if(c2xx_start != C2XX_START) {
					//printk(KERN_INFO "c2xx start %04x\n", c2xx_start);
					mutex_unlock(&read_lock);
					return -EINVAL;
				}
				ret = kfifo_out(&c2xx_fifo, &len, sizeof(len) );
				if (ret != sizeof(len)) {
					mutex_unlock(&read_lock);
					return -EINVAL;
				}
				ret = kfifo_to_user(&c2xx_fifo, uptr->packet, len, &copied);
				if(ret != 0) {
					printk(KERN_INFO "c2xx kfifo_to_user failed\n");
					mutex_unlock(&read_lock);
					return -EFAULT;					
				}
				if (put_user(1, &uptr->nfetch)) {
					mutex_unlock(&read_lock);
					return -EFAULT;					
				}
				if( put_user(len, &uptr->nlength)) {
					mutex_unlock(&read_lock);
					return -EFAULT;
				}
				break;
			}
			mutex_unlock(&read_lock);
			break;
		default:
			return -ENOTTY;
	}
	return 0;
}

static const struct file_operations c2xx_fops = {
	.owner				= THIS_MODULE,
//	.read				= c2xx_read,
	.unlocked_ioctl		= c2xx_ioctl
};

static struct miscdevice c2xx_dev = {
	MISC_DYNAMIC_MINOR,
	"c2xx",
	&c2xx_fops
};

static int c2xx_init(struct usbnet *dev)
{
	int ret;
	static const char init_msg_1[] = 
		{  0x57, 0x43, 0x1f, 0x00, 0x15, 0x02, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x15, 0x02, 0x7f, 0x0f, 0x00, 0x00,
		   0x0c, 0x00, 0x00, 0xff, 0xa0, 0x00, 0x10, 0x14,
		   0x10, 0xc9, 0xa6, 0xff, 0x7e, 0x2b, 0x00, 0x00};
	static const char init_msg_2[] = 
		{  0x57, 0x43, 0x1f, 0x00, 0x15, 0x02, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x15, 0x02, 0x7f, 0x0f, 0x00, 0x00,
		   0x0c, 0x00, 0x01, 0xff, 0xa0, 0x00, 0x20, 0x22,
		   0xb2, 0xd9, 0xa6, 0xff, 0x7e, 0x72, 0x42, 0x00};
	static const char init_msg_3[] = 
		{  0x57, 0x43, 0x1a, 0x00, 0x15, 0x02, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x15, 0x02, 0x7f, 0x0a, 0x00, 0x00,
		   0x07, 0x00, 0x02, 0xff, 0xa0, 0x00, 0x00, 0x7e};

	static const int buflen = 256;

	char *usb_buf;
	int status;

	usb_buf = kmalloc(buflen, GFP_DMA | GFP_KERNEL);
	if (!usb_buf)
		return -ENOMEM;

	memcpy(usb_buf, init_msg_1, 40);
	status = kalmia_send_init_packet(dev, usb_buf, sizeof(init_msg_1)
		/ sizeof(init_msg_1[0]), usb_buf, 255);
	memcpy(usb_buf, init_msg_2, 40);
	status = kalmia_send_init_packet(dev, usb_buf, sizeof(init_msg_2)
		/ sizeof(init_msg_2[0]), usb_buf, 255);
	memcpy(usb_buf, init_msg_3, 32);
	status = kalmia_send_init_packet(dev, usb_buf, sizeof(init_msg_3)
		/ sizeof(init_msg_3[0]), usb_buf, 255);
	printk(KERN_INFO "c2xx debug ON\n");
	kfree(usb_buf);
	ret = misc_register(&c2xx_dev);
	if (ret) {
		// FIXME
		printk(KERN_ERR
		       "Unable to register c2xx misc device\n");		
	}

	INIT_KFIFO(c2xx_fifo);

	return ret;
}

static void c2xx_disconnect(struct usb_interface *interface)
{
	misc_deregister(&c2xx_dev);
	usbnet_disconnect(interface);
}

/*-------------------------------------------------------------------------*/

static const struct usb_device_id products[] = {
	/* The unswitched USB ID, to get the module auto loaded: */
	{ USB_DEVICE(0x04e8, 0x689a) },
	/* The stick swithed into modem (by e.g. usb_modeswitch): */
	{ USB_DEVICE(0x04e8, 0x6889),
		.driver_info = (unsigned long) &kalmia_info, },
	{ /* EMPTY == end of list */} };
MODULE_DEVICE_TABLE( usb, products);

static struct usb_driver kalmia_driver = {
	.name = "kalmia",
	.id_table = products,
	.probe = usbnet_probe,
//	.disconnect = usbnet_disconnect,
	.disconnect = c2xx_disconnect,
	.suspend = usbnet_suspend,
	.resume = usbnet_resume,
	.disable_hub_initiated_lpm = 1,
};

module_usb_driver(kalmia_driver);

MODULE_AUTHOR("Marius Bjoernstad Kotsbak <marius@kotsbak.com>");
MODULE_DESCRIPTION("Samsung Kalmia USB network driver");
MODULE_LICENSE("GPL");
