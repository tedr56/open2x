#############################################################
#
# GMenu2X
#
#############################################################

GMENU2X_VERSION:=0.9
GMENU2X_SOURCE:=gmenu2x-$(GMENU2X_VERSION)-o2x.tar.bz2

$(TARGET_DIR)/usr/menu/gmenu2x:
	echo "Installing GMenu2X..."
	$(WGET) -P $(DL_DIR) $(O2X_REPO)/gmenu2x/$(GMENU2X_SOURCE)
	mkdir -p $(TARGET_DIR)/usr/menu
	tar xfvj $(DL_DIR)/$(GMENU2X_SOURCE) -C $(TARGET_DIR)/usr/menu

gmenu2x: $(TARGET_DIR)/usr/menu/gmenu2x
	echo "Installed GMenu2X..."

gmenu2x-clean:
	

gmenu2x-dirclean:
	
#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_GMENU2X)),y)
TARGETS+=gmenu2x
endif
