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

/**
 * This file is heavily based on the tun.c source file of aiccu -
 * Automatic IPv6 Connectivity Client Utility, which is released
 * under three clause BSD-like license. You can download the source
 * code of aiccu from http://www.sixxs.net/tools/aiccu/ website
 */


#define TAP_REGISTRY_KEY    "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define TAP_ADAPTER_KEY     "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

struct tap_reg {
	char *guid;
	struct tap_reg *next;
};

struct panel_reg {
	char *name;
	char *guid;
	struct panel_reg *next;
};

/* Get a working tunnel adapter */
static struct tap_reg *
get_tap_reg(taplog_t *taplog)
{
	struct tap_reg *first = NULL;
	struct tap_reg *last = NULL;
	HKEY adapter_key;
	LONG status;
	int i;

	status = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
	                       TAP_ADAPTER_KEY,
	                       0,
	                       KEY_READ,
	                       &adapter_key);
	if (status != ERROR_SUCCESS) {
		taplog_log(taplog, TAPLOG_ERR,
		           "Error opening registry key: %s",
		           TAP_ADAPTER_KEY);
		return NULL;
	}

	for (i=0; 1; i++) {
		char enum_name[256];
		char unit_string[256];
		HKEY unit_key;
		char component_id[256];
		DWORD data_type;
		DWORD len;

		len = sizeof(enum_name);
		status = RegEnumKeyExA(adapter_key, i, enum_name,
		                       &len, NULL, NULL, NULL, NULL);
		if (status == ERROR_NO_MORE_ITEMS) {
			break;
		} else if (status != ERROR_SUCCESS) {
			taplog_log(taplog, TAPLOG_ERR,
			           "Error enumerating registry subkeys of key: %s (t0)",
			           TAP_ADAPTER_KEY);
			break;
		} else if (!strcmp(enum_name, "Properties")) {
			/* Properties key can not be opened, skip */
			continue;
		}

		unit_string[sizeof(unit_string)-1] = '\0';
		snprintf(unit_string, sizeof(unit_string)-1,
		         "%s\\%s", TAP_ADAPTER_KEY, enum_name);
		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, unit_string,
		                       0, KEY_READ, &unit_key);
		if (status != ERROR_SUCCESS) {
			taplog_log(taplog, TAPLOG_WARNING,
			           "Error opening registry key: %s (t1)",
			           unit_string);
			continue;
		}

		len = sizeof(component_id);
		status = RegQueryValueExA(unit_key, "ComponentId",
					  NULL, &data_type,
					  (LPBYTE) component_id,
		                          &len);
		if (status != ERROR_SUCCESS || data_type != REG_SZ) {
			taplog_log(taplog, TAPLOG_WARNING,
				   "Error opening registry key: %s\\ComponentId (t2)",
				    unit_string);
		} else {
			char net_cfg_instance_id[256];

			len = sizeof(net_cfg_instance_id);
			status = RegQueryValueExA(unit_key, "NetCfgInstanceId",
						  NULL, &data_type,
						  (LPBYTE) net_cfg_instance_id,
						  &len);

			if (status == ERROR_SUCCESS && data_type == REG_SZ) {
				/* The component ID is usually tap0801 or tap0901
				 * depending on the version, for convenience we
				 * accept all tapXXXX components */
				if (strlen(component_id) == 7 &&
				    !strncmp(component_id, "tap", 3)) {
					struct tap_reg *reg;

					reg = calloc(1, sizeof(struct tap_reg));
					if (!reg) { 
						RegCloseKey(unit_key);
						continue;
					}
					reg->guid = strdup(net_cfg_instance_id);

					/* Update the linked list */
					if (!first) first = reg;
					if (last) last->next = reg;
					last = reg;
				}
			}
		}

		RegCloseKey(unit_key);
	}

	RegCloseKey(adapter_key);

	return first;
}

static void
free_tap_reg(struct tap_reg *tap_reg)
{
	while (tap_reg) {
		struct tap_reg *next = tap_reg->next;

		free(tap_reg->guid);
		free(tap_reg);

		tap_reg = next;
	}
}

/* Collect GUID's and names of all the Connections that are available */
static struct panel_reg *
get_panel_reg(taplog_t *taplog)
{
	struct panel_reg *first = NULL;
	struct panel_reg *last = NULL;
	HKEY network_connections_key;
	LONG status;
	int i;

	status = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
	                       TAP_REGISTRY_KEY,
	                       0,
	                       KEY_READ,
	                       &network_connections_key);
	if (status != ERROR_SUCCESS) {
		taplog_log(taplog, TAPLOG_ERR,
		           "Error opening registry key: %s (p0)",
		           TAP_REGISTRY_KEY);
		return NULL;
	}

	for (i=0; 1; i++) {
		char enum_name[256];
		char connection_string[256];
		HKEY connection_key;
		BYTE name_data[512];
		DWORD name_type;
		DWORD len;

		len = sizeof(enum_name);
		status = RegEnumKeyExA(network_connections_key, i, enum_name, &len,
		                       NULL, NULL, NULL, NULL);
		if (status == ERROR_NO_MORE_ITEMS) {
			break;
		} else if (status != ERROR_SUCCESS) {
			taplog_log(taplog, TAPLOG_ERR,
			           "Error enumerating registry subkeys of key: %s (p1)",
			           TAP_REGISTRY_KEY);
			break;
		} else if (enum_name[0] != '{') {
			continue;
		}

		connection_string[sizeof(connection_string)-1] = '\0';
		snprintf(connection_string, sizeof(connection_string)-1,
		         "%s\\%s\\Connection", TAP_REGISTRY_KEY, enum_name);
		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, connection_string,
		                       0, KEY_READ, &connection_key);
		if (status != ERROR_SUCCESS) {
			taplog_log(taplog, TAPLOG_WARNING,
			           "Error opening registry key: %s (p2)",
			           connection_string);
			continue;
		}

		len = sizeof(name_data);
		status = RegQueryValueExW(connection_key, L"Name", NULL,
		                          &name_type, name_data, &len);
		if (status != ERROR_SUCCESS || name_type != REG_SZ) {
			taplog_log(taplog, TAPLOG_WARNING,
			           "Error opening registry key: %s\\%s\\Name (p3)",
		                   TAP_REGISTRY_KEY, connection_string);
		} else {
			struct panel_reg *reg;
			int namelen;

			reg = calloc(1, sizeof(struct panel_reg));
			if (!reg) {
				RegCloseKey(connection_key);
				continue;
			}
			reg->guid = strdup(enum_name);

			/* Make the UTF-16 to UTF-8 conversion */
			namelen = WideCharToMultiByte(CP_UTF8, 0,
			                              (LPCWSTR) name_data, -1,
			                              NULL, 0, NULL, NULL);
			reg->name = malloc(namelen);
			WideCharToMultiByte(CP_UTF8, 0,
			                    (LPCWSTR) name_data, -1,
			                    reg->name, namelen,
			                    NULL, NULL);

			/* Update the linked list */
			if (!first) first = reg;
			if (last) last->next = reg;
			last = reg;
		}

		RegCloseKey(connection_key);
	}

	RegCloseKey(network_connections_key);

	return first;
}

static void
free_panel_reg(struct panel_reg *panel_reg)
{
	while (panel_reg) {
		struct panel_reg *next = panel_reg->next;

		free(panel_reg->guid);
		free(panel_reg->name);
		free(panel_reg);

		panel_reg = next;
	}
}

static char *
tapcfg_fixup_adapters(taplog_t *taplog, const char *ifname,
                      char **guid, int fallback)
{
	struct tap_reg *tap_reg = get_tap_reg(taplog), *tr;
	struct panel_reg *panel_reg = get_panel_reg(taplog), *pr;
	struct panel_reg *adapter = NULL;
	unsigned int found=0, valid=0;
	char *ret = NULL;

	taplog_log(taplog, TAPLOG_DEBUG, "Available TAP adapters [name, GUID]:");

	/* loop through each TAP adapter registry entry */
	for (tr=tap_reg; tr != NULL; tr=tr->next) {
		struct tap_reg *iterator;
		int links = 0, valid_adapter = 1;

		/* loop through each network connections entry in the control panel */
		for (pr=panel_reg; pr != NULL; pr=pr->next) {
			if (!strcmp(tr->guid, pr->guid)) {
				taplog_log(taplog, TAPLOG_DEBUG, "'%s' %s", pr->name, tr->guid);
				links++;

				/* If we haven't found adapter with a correct name yet
				 * then we should mark this adapter as a candidate */
				if (!found) {
					adapter = pr;
				}

				if (!strcasecmp(ifname, pr->name)) {
					found++;
				}
			}
		}

		if (!links) {
			taplog_log(taplog, TAPLOG_WARNING,
			           "*** Adapter with GUID %s doesn't have a link from the "
			           "control panel", tr->guid);
			valid_adapter = 0;
		} else if (links > 1) {
			taplog_log(taplog, TAPLOG_WARNING,
			           "*** Adapter with GUID %s has %u links from the Network "
			           "Connections control panel, it should only be 1",
			           tr->guid, links);
			valid_adapter = 0;
		}

		for (iterator=tap_reg; iterator != NULL; iterator=iterator->next) {
			if (tr != iterator && !strcmp(tr->guid, iterator->guid)) {
				taplog_log(taplog, TAPLOG_WARNING, "*** Duplicate Adapter GUID %s", tr->guid);
				valid_adapter = 0;
			}
		}

		if (valid_adapter) {
			valid++;
		}
	}

	if (found == 1 && valid >= 1) {
		taplog_log(taplog, TAPLOG_DEBUG,
		           "Using configured interface %s",
		           ifname);
		ret = strdup(ifname);
		if (guid) {
			*guid = strdup(adapter->guid);
		}
	} else if (found == 0 && valid == 1 && adapter && fallback) {
		taplog_log(taplog, TAPLOG_INFO,
		           "Using adapter '%s' instead of '%s' because it was the only one found",
		           adapter->name, ifname);
		ret = strdup(adapter->name);
		if (guid) {
			*guid = strdup(adapter->guid);
		}
	} else {
		taplog_log(taplog, TAPLOG_WARNING,
		           "Found %u adapters, %u of which were valid, don't know what to use",
		           found, valid);
	}

	free_tap_reg(tap_reg);
	free_panel_reg(panel_reg);

	return ret;
}
