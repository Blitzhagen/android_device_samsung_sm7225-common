#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH := $(call my-dir)

ifneq ($(filter gts7xllite,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

# Kernel-Header aus dem Original-Quellcode fuer Legacy-Make-Module
# (qcom-caf Module setzen KERNEL_OBJ/usr als Dependency; bei
#  TARGET_FORCE_PREBUILT_KERNEL wird es sonst nicht erzeugt)
KERNEL_HEADERS_USR := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
# "make" ist im Android-Build ein disallowed PATH tool -> absoluter Pfad
# (command -v liefert den out/.path-Shim, daher hart /usr/bin/make)
KERNEL_HEADERS_MAKE := /usr/bin/make
$(KERNEL_HEADERS_USR):
	@echo "Installing kernel headers -> $@"
	@mkdir -p $@
	$(hide) $(KERNEL_HEADERS_MAKE) -C $(TARGET_KERNEL_SOURCE) O=$(abspath $(KERNEL_HEADERS_USR)/..) \
	    HOSTCC=$(BUILD_TOP)/prebuilts/clang/host/$(HOST_PREBUILT_TAG)/clang-r487747c/bin/clang \
	    HOSTCXX=$(BUILD_TOP)/prebuilts/clang/host/$(HOST_PREBUILT_TAG)/clang-r487747c/bin/clang++ \
	    ARCH=arm64 headers_install
	@# asm*/signal.h kollidiert mit Bionics sigaction (bionic nennt es
	@# __kernel_sigaction); linux/ion.h (Upstream-UAPI) kollidiert mit
	@# libions Legacy-UAPI (ion_user_handle_t) -> beides aus dem
	@# Include-Pfad entfernen
	$(hide) rm -f $@/include/asm/signal.h $@/include/asm-generic/signal.h \
	    $@/include/linux/ion.h

include $(CLEAR_VARS)

FIRMWARE_MOUNT_POINT := $(TARGET_OUT_VENDOR)/firmware_mnt
$(FIRMWARE_MOUNT_POINT): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating $(FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/firmware_mnt

FIRMWARE_MODEM_MOUNT_POINT := $(TARGET_OUT_VENDOR)/firmware-modem
$(FIRMWARE_MODEM_MOUNT_POINT): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating $(FIRMWARE_MODEM_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/firmware-modem

DSP_MOUNT_POINT := $(TARGET_OUT_VENDOR)/dsp
$(DSP_MOUNT_POINT): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating $(DSP_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/dsp

ALL_DEFAULT_INSTALLED_MODULES += $(FIRMWARE_MOUNT_POINT) $(FIRMWARE_MODEM_MOUNT_POINT) $(DSP_MOUNT_POINT)

define rfs-tree
RFS_$(1)_$(2)_STAMP := $$(TARGET_OUT_INTERMEDIATES)/rfs_$(1)_$(2)_links_done
$$(RFS_$(1)_$(2)_STAMP): $$(LOCAL_INSTALLED_MODULE)
	@echo "Creating RFS $(1)/$(2) structure"
	@mkdir -p $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/readonly/vendor
	$$(hide) ln -sf /data/vendor/tombstones/rfs/$(3) $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/ramdumps
	$$(hide) ln -sf /mnt/vendor/persist/rfs/$(1)/$(2) $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/readwrite
	$$(hide) ln -sf /mnt/vendor/persist/rfs/shared $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/shared
	$$(hide) ln -sf /mnt/vendor/persist/hlos_rfs/shared $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/hlos
	$$(hide) ln -sf /vendor/$(4) $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/readonly/firmware
	$$(hide) ln -sf /vendor/firmware $$(TARGET_OUT_VENDOR)/rfs/$(1)/$(2)/readonly/vendor/firmware
	$$(hide) mkdir -p $$(dir $$@) && touch $$@

ALL_DEFAULT_INSTALLED_MODULES += $$(RFS_$(1)_$(2)_STAMP)
endef

$(eval $(call rfs-tree,apq,gnss,modem,firmware_mnt))
$(eval $(call rfs-tree,mdm,adsp,lpass,firmware_mnt))
$(eval $(call rfs-tree,mdm,cdsp,cdsp,firmware_mnt))
$(eval $(call rfs-tree,mdm,mpss,modem,firmware-modem))
$(eval $(call rfs-tree,mdm,slpi,slpi,firmware_mnt))
$(eval $(call rfs-tree,mdm,tn,tn,firmware_mnt))
$(eval $(call rfs-tree,msm,adsp,lpass,firmware_mnt))
$(eval $(call rfs-tree,msm,cdsp,cdsp,firmware_mnt))
$(eval $(call rfs-tree,msm,mpss,modem,firmware-modem))
$(eval $(call rfs-tree,msm,slpi,slpi,firmware_mnt))

# GPU library symlinks (adreno -> egl/)
EGL_SYMLINKS := \
    $(TARGET_OUT_VENDOR)/lib64/libEGL_adreno.so \
    $(TARGET_OUT_VENDOR)/lib64/libGLESv2_adreno.so \
    $(TARGET_OUT_VENDOR)/lib64/libq3dtools_adreno.so \
    $(TARGET_OUT_VENDOR)/lib/libEGL_adreno.so \
    $(TARGET_OUT_VENDOR)/lib/libGLESv2_adreno.so \
    $(TARGET_OUT_VENDOR)/lib/libq3dtools_adreno.so

$(EGL_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating adreno symlink: $@"
	$(hide) ln -sf egl/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(EGL_SYMLINKS)

CNE_APP_SYMLINKS := $(TARGET_OUT_VENDOR)/app/CneApp/lib/arm64/.cne_links_done
$(CNE_APP_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating CneApp symlinks"
	@mkdir -p $(dir $@)
	$(hide) ln -sf /vendor/lib64/libvndfwk_detect_jni.qti.so $(dir $@)libvndfwk_detect_jni.qti.so
	$(hide) touch $@

ALL_DEFAULT_INSTALLED_MODULES += $(CNE_APP_SYMLINKS)

# vendor/bin toybox_vendor/toolbox Applets (Symlinks aus Stock)
SM7225_COMMON_DIR := $(LOCAL_PATH)
TOYBOX_BIN_LINKS := $(TARGET_OUT_VENDOR)/bin/.toybox_links_done
$(TOYBOX_BIN_LINKS): $(LOCAL_INSTALLED_MODULE) $(SM7225_COMMON_DIR)/vendor_bin_symlinks.txt
	@echo "Creating vendor/bin applet symlinks"
	$(hide) while IFS=: read -r name target; do \
	    ln -sf $$target $(TARGET_OUT_VENDOR)/bin/$$name; \
	done < $(SM7225_COMMON_DIR)/vendor_bin_symlinks.txt
	$(hide) touch $@

ALL_DEFAULT_INSTALLED_MODULES += $(TOYBOX_BIN_LINKS)

# Prebuilt Vendor-Libs als linkbare Make-Module (qcom-caf Android.mk-Deps)
VND_PREB := ../../../vendor/samsung/sm7225-common/proprietary

include $(CLEAR_VARS)
LOCAL_MODULE        := libthermalclient
LOCAL_MODULE_CLASS  := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB      := both
LOCAL_SRC_FILES_32  := $(VND_PREB)/vendor/lib/libthermalclient.so
LOCAL_SRC_FILES_64  := $(VND_PREB)/vendor/lib64/libthermalclient.so
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE        := libskeymaster4device
LOCAL_MODULE_CLASS  := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB      := 64
LOCAL_SRC_FILES_64  := $(VND_PREB)/vendor/lib64/libskeymaster4device.so
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE        := libfastcvopt
LOCAL_MODULE_CLASS  := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB      := both
LOCAL_SRC_FILES_32  := $(VND_PREB)/vendor/lib/libfastcvopt.so
LOCAL_SRC_FILES_64  := $(VND_PREB)/vendor/lib64/libfastcvopt.so
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

endif
