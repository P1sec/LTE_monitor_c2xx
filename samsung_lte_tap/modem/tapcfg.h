#ifndef TAPCFG_H
#define TAPCFG_H

/* On Windows we explicitly only export the API symbols */
#if defined(_WIN32) && defined(DLL_EXPORT)
# define TAPCFG_API __declspec(dllexport)
#else
# define TAPCFG_API
#endif

/* Current API version number, should be kept up to date */
#define TAPCFG_VERSION_MAJOR  1
#define TAPCFG_VERSION_MINOR  1
#define TAPCFG_VERSION ((TAPCFG_VERSION_MAJOR << 16) | TAPCFG_VERSION_MINOR)

/* Define syslog style log levels */
#define TAPLOG_EMERG       0       /* system is unusable */
#define TAPLOG_ALERT       1       /* action must be taken immediately */
#define TAPLOG_CRIT        2       /* critical conditions */
#define TAPLOG_ERR         3       /* error conditions */
#define TAPLOG_WARNING     4       /* warning conditions */
#define TAPLOG_NOTICE      5       /* normal but significant condition */
#define TAPLOG_INFO        6       /* informational */
#define TAPLOG_DEBUG       7       /* debug-level messages */

#define TAPCFG_STATUS_ALL_DOWN   0x0000
#define TAPCFG_STATUS_ALL_UP     0xffff

#define TAPCFG_STATUS_IPV4_UP    0x0001
#define TAPCFG_STATUS_IPV4_ALL   0x000f

#define TAPCFG_STATUS_IPV6_UP    0x0010
#define TAPCFG_STATUS_IPV6_RADV  0x0020
#define TAPCFG_STATUS_IPV6_ALL   0x00f0

typedef void (*taplog_callback_t)(int level, char *msg);

/**
 * Typedef to the structure used by the library, should never
 * be accessed directly.
 */
typedef struct tapcfg_s tapcfg_t;

/**
 * Get the current version of the library, this number only
 * changes when the API is changed. In general it should be
 * in format ((major << 16) | minor) where binary incompatible
 * changes in API increase the minor version number and source
 * incompatible changes increase the major version number.
 * @return The version number encoded as an integer.
 */
TAPCFG_API int tapcfg_get_version();

/**
 * Set the current logging level, all messages with lower
 * significance than the set level will be ignored.
 * @param tapcfg is a pointer to an inited structure
 * @param level is the new level of logging
 */
TAPCFG_API void tapcfg_set_log_level(tapcfg_t *tapcfg, int level);

/**
 * Set callback to receive all the messages to be logged.
 * If not called at all or called with null parameter, then
 * all messages will be printed to stderr stream.
 * @param tapcfg is a pointer to an inited structure
 * @param callback is the callback function for logging
 */
TAPCFG_API void tapcfg_set_log_callback(tapcfg_t *tapcfg, taplog_callback_t callback);

/**
 * Initializes a new tapcfg_t structure and allocates
 * the required memory for it.
 * @return A pointer to the tapcfg_t structure to be used
 */
TAPCFG_API tapcfg_t *tapcfg_init();

/**
 * Destroys a tapcfg_t structure and frees all resources
 * related to it cleanly. Will also stop the device in
 * case it is started. The handle can't be used any more
 * after calling this function.
 * @param tapcfg is a pointer to an inited structure
 */
TAPCFG_API void tapcfg_destroy(tapcfg_t *tapcfg);


/**
 * Creates a new network interface with the specified name
 * and waits for subsequent calls. This has to be called
 * before any calls to other functions except init, or
 * the other functions will simply fail. If the specified
 * interface name is not available, will fallback to using
 * the system default.
 * @param tapcfg is a pointer to an inited structure
 * @param ifname is a pointer to the suggested name for 
 *        the device in question in UTF-8 encoding, can be
 *        null for system default
 * @param fallback is a flag, if it is set and the defined
 *        interface name is not available, other available
 *        TAP interfaces are searched and used accordingly
 * @return Negative value on error, non-negative on success.
 */
TAPCFG_API int tapcfg_start(tapcfg_t *tapcfg, const char *ifname, int fallback);

/**
 * Stops the network interface and frees all resources
 * related to it. After this a new interface using the
 * tcptap_start can be started for the same structure.
 * @param tapcfg is a pointer to an inited structure
 */
TAPCFG_API void tapcfg_stop(tapcfg_t *tapcfg);

/**
 * Get the file descriptor attached to the device.
 * @param tapcfg is a pointer to an inited structure
 */
TAPCFG_API int tapcfg_get_fd(tapcfg_t *tapcfg);

/**
 * Wait for data to be available for reading. This can
 * be used for avoiding blocking the thread on read. If
 * a positive value is returned, then the following read
 * will not block in any case.
 * @param tapcfg is a pointer to an inited structure
 * @param msec is the time in milliseconds to wait for the
 *        device to become readable, can be 0 in which case
 *        the function will return immediately
 * @return Non-zero if the device is readable, zero otherwise.
 */
TAPCFG_API int tapcfg_wait_readable(tapcfg_t *tapcfg, int msec);

/**
 * Read data from the device. This function will always
 * block until there is data readable. The blocking can be
 * avoided by making sure there is data available by using
 * the tapcfg_wait_readable function. The buffer should
 * always have enough space for a complete Ethernet frame
 * or the read will simply fail.
 * @param tapcfg is a pointer to an inited structure
 * @param buf is a pointer to the buffer where data is read to
 * @param count is the maximum size of the buffer
 * @return Negative value on error, number of bytes read otherwise.
 */
TAPCFG_API int tapcfg_read(tapcfg_t *tapcfg, void *buf, int count);

/**
 * Wait for data to be available for writing. This can
 * be used for avoiding blocking the thread on write. If
 * a positive value is returned, then the following write
 * will not block in any case.
 * @param tapcfg is a pointer to an inited structure
 * @param msec is the time in milliseconds to wait for the
 *        device to become writable, can be 0 in which case
 *        the function will return immediately
 * @return Non-zero if the device is writable, zero otherwise.
 */
TAPCFG_API int tapcfg_wait_writable(tapcfg_t *tapcfg, int msec);

/**
 * Write data to the device. This function will always
 * block until the device is writable. The blocking can be
 * avoided by making sure the device is writable by using
 * the tapcfg_wait_readable function. However in common case
 * write should be very fast and that is not necessary.
 * The buffer should always contain a complete Ethernet frame
 * to write.
 * @param tapcfg is a pointer to an inited structure
 * @param buf is a pointer to the buffer where data is written from
 * @param count is the number of bytes in the buffer
 * @return Negative value on error, number of bytes written otherwise.
 */
TAPCFG_API int tapcfg_write(tapcfg_t *tapcfg, void *buf, int count);


/**
 * Get the current name of the interface. This can be called
 * after tapcfg_start to see if the suggested interface name
 * was available for use.
 * @param tapcfg is a pointer to an inited structure
 * @return Pointer to a character array containing the interface
 *         name in UTF-8 encoding, should NOT be freed by the caller
 *         application after use! NULL if the device is not started.
 */
TAPCFG_API char *tapcfg_get_ifname(tapcfg_t *tapcfg);

/**
 * Get the current hardware MAC address of the interface. This
 * can be called after tapcfg_start for use in packet construction.
 * @param tapcfg is a pointer to an inited structure
 * @param length is a pointer to an integer that stores the address,
 *        array length, can be NULL in which case it is ignored
 * @return Pointer to a character array containing the interface
 *         hardware address in binary format, should NOT be freed
 *         by the caller application after use! NULL if the device
 *         is not started.
 */
TAPCFG_API const char *tapcfg_iface_get_hwaddr(tapcfg_t *tapcfg, int *length);

/**
 * Set the current hardware MAC address of the interface. This
 * can be called after tapcfg_start to change the source MAC of
 * outgoing packets. This function will fail on some systems like
 * Windows 2000 or Windows XP and that is ok. The system APIs on
 * those platforms don't support dynamic MAC address, however it
 * can be changed manually from Windows TAP driver settings.
 * @param tapcfg is a pointer to an inited structure
 * @param hwaddr is a pointer to an array containing the address
 * @param length is the length of the array, should be always 6 for now
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_hwaddr(tapcfg_t *tapcfg, const char *hwaddr, int length);

/**
 * Get the status of the interface. In Unix systems this means
 * if the interface is up or down and in Windows it means if the
 * network cable of the virtual interface is connected or not.
 * @param tapcfg is a pointer to an inited structure
 * @return Non-zero if the interface is enabled, zero otherwise.
 */
TAPCFG_API int tapcfg_iface_get_status(tapcfg_t *tapcfg);

/**
 * Set the status of the interface. In Unix systems this means
 * if the interface is up or down and in Windows it means if the
 * network cable of the virtual interface is connected or not.
 * @param tapcfg is a pointer to an inited structure
 * @param flags can be TAPCFG_STATUS_IPV4_UP or TAPCFG_STATUS_IPV6_UP
 *        or TAPCFG_STATUS_ALL_UP, which is the most common use case
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_status(tapcfg_t *tapcfg, int flags);

/**
 * Get the maximum transfer unit for the device if possible.
 * @param tapcfg is a pointer to an inited structure
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_get_mtu(tapcfg_t *tapcfg);

/**
 * Set the maximum transfer unit for the device if possible, this function
 * will fail on some systems like Windows 2000 or Windows XP and that is ok.
 * The IP stack on those platforms doesn't support dynamic MTU and it should
 * not cause trouble in any other functionality.
 * @param tapcfg is a pointer to an inited structure
 * @param mtu is the new maximum transfer unit after calling the function
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_mtu(tapcfg_t *tapcfg, int mtu);

/**
 * Set the IPv4 address and netmask of the interface and update
 * the routing tables accordingly.
 * @param tapcfg is a pointer to an inited structure
 * @param addr is a string containing the address in standard numeric format
 * @param netbits is the number of bits in the netmask for this subnet,
 *        must be between [1, 32]
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_ipv4(tapcfg_t *tapcfg, const char *addr, unsigned char netbits);

/**
 * Set the IPv6 address and netmask of the interface and update
 * the routing tables accordingly.
 * @param tapcfg is a pointer to an inited structure
 * @param addr is a string containing the address in standard numeric format
 * @param netbits is the number of bits in the netmask for this subnet,
 *        must be between [1, 128]
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_ipv6(tapcfg_t *tapcfg, const char *addr, unsigned char netbits);

/**
 * Set a DHCP options if the IPv4 address of the interface is configured by
 * using DHCP instead of basic IPv4 address setting. Basically this function
 * works only on Windows platform and should return -1 on all other systems.
 * It can be used for example to add DNS servers to the interface cleanly on
 * Windows. The buffer should include DHCP option data as defined by RFC2123.
 * Notice that each call to this function will overwrite the values defined
 * earlier!
 * @param tapcfg is a pointer to an inited structure
 * @param buffer is a pointer to the DHCP option data buffer
 * @param buflen is the length of the option data buffer
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_dhcp_options(tapcfg_t *tapcfg, unsigned char *buffer, int buflen);

/**
 * Set a DHCPv6 options if the IPv6 address of the interface is configured by
 * using DHCPv6 instead of basic IPv6 address setting. Basically this function
 * works only on Windows platform and should return -1 on all other systems.
 * It can be used for example to add DNS servers to the interface cleanly on
 * Windows. The buffer should include DHCPv6 option data as defined by RFC3315.
 * Notice that each call to this function will overwrite the values defined
 * earlier!
 * @param tapcfg is a pointer to an inited structure
 * @param buffer is a pointer to the DHCPv6 option data buffer
 * @param buflen is the length of the option data buffer
 * @return Negative value if an error happened, non-negative otherwise.
 */
TAPCFG_API int tapcfg_iface_set_dhcpv6_options(tapcfg_t *tapcfg, unsigned char *buffer, int buflen);

#endif /* TAPCFG_H */

