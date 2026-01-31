################################################################################
#
# viz1090
#
################################################################################

VIZ1090_VERSION = 1.0
VIZ1090_SITE = $(BR2_EXTERNAL_VIZ1090_PATH)/../../
VIZ1090_SITE_METHOD = local
VIZ1090_LICENSE = MIT
VIZ1090_LICENSE_FILES = LICENSE

VIZ1090_DEPENDENCIES = sdl2 sdl2_gfx sdl2_ttf freetype

VIZ1090_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_TESTING=OFF

define VIZ1090_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/viz1090 $(TARGET_DIR)/opt/viz1090/viz1090
endef

$(eval $(cmake-package))
