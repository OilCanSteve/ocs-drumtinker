######################################
#
# DRUMTINKER
#
######################################
OCS_DRUMTINKER_VERSION = 3a7de9ad5d7ae74cf21296ff8949f02457a8af7b
OCS_DRUMTINKER_SITE = https://github.com/OilCanSteve/ocs-drumtinker.git
OCS_DRUMTINKER_SITE_METHOD = git
OCS_DRUMTINKER_BUNDLES = ocs-drumtinker.lv2

OCS_DRUMTINKER_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) MOD=1 OPTIMIZATIONS="" PREFIX=/usr -C $(@D)/source

define OCS_DRUMTINKER_BUILD_CMDS
	$(OCS_DRUMTINKER_TARGET_MAKE)
endef

define OCS_DRUMTINKER_INSTALL_TARGET_CMDS
	$(OCS_DRUMTINKER_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef

$(eval $(generic-package))