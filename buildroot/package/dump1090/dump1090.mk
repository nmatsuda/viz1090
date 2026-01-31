################################################################################
#
# dump1090 (FlightAware fork)
#
################################################################################

DUMP1090_VERSION = v9.0
DUMP1090_SITE = https://github.com/flightaware/dump1090.git
DUMP1090_SITE_METHOD = git
DUMP1090_LICENSE = GPL-2.0+
DUMP1090_LICENSE_FILES = COPYING

DUMP1090_DEPENDENCIES = rtl-sdr libusb

# dump1090-fa uses a plain Makefile
define DUMP1090_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) \
		BLADERF=no HACKRF=no LIMESDR=no \
		-C $(@D) dump1090
endef

define DUMP1090_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dump1090 $(TARGET_DIR)/usr/bin/dump1090
endef

$(eval $(generic-package))
