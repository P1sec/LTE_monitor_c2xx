#
# $Id$
#

_CUSTOM_SUBDIRS_ = \
	c2xx	

_CUSTOM_EXTRA_DIST_ = \
	Custom.m4 \
	Custom.make

_CUSTOM_plugin_ldadd_ = \
	-dlopen plugins/c2xx/c2xx.la
