#
# Example for user-defined program
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gobi_loader
PKG_VERSION:=1.0
PKG_BUILD_DIR:=$(BUILD_DIR)/gobi_loader-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

# SECTION currently unused 
define Package/$(PKG_NAME)
 SECTION:=utils
 CATEGORY:=Utilities
 TITLE:=Firmware loader for Qualcomm Gobi USB chipsets
 SUBMENU:=iPkg
 MAINTAINER:=Matthew Garrett, Kicer Jiao
 URL:=
 DEPENDS:=
endef

define Package/$(PKG_NAME)/description
  Firmware loader for Qualcomm Gobi USB chipsets
endef

# backup this files when upgrade
define Package/$(PKG_NAME)/conffiles
endef

# install script
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin
endef

# pre-install script
define Package/$(PKG_NAME)/preinst
endef

# post-install script
define Package/$(PKG_NAME)/postinst
endef

# pre-remove script
define Package/$(PKG_NAME)/prerm
endef

# post-remove script
define Package/$(PKG_NAME)/postrm
endef

define Build/Prepare
	[ -d $(PKG_BUILD_DIR) ] || mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Configure
endef

#define Build/Compile
#endef

$(eval $(call BuildPackage,$(PKG_NAME)))
