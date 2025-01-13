#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH := $(call my-dir)

ifneq ($(filter gts7xllite,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

# Kernel headers from the original source for legacy make modules
# (qcom-caf modules depend on KERNEL_OBJ/usr; it is not generated
#  when TARGET_FORCE_PREBUILT_KERNEL is used)
KERNEL_HEADERS_USR := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
# "make" is a disallowed PATH tool in the Android build -> absolute path
# (command -v would return the out/.path shim, hence hard /usr/bin/make)
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

ALL_DEFAULT_INSTALLED_MODULES += $(FIRMWARE_MOUNT_POINT) $(BT_FIRMWARE_MOUNT_POINT) $(DSP_MOUNT_POINT) $(FIRMWARE_MODEM_MOUNT_POINT)

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

# vendor/bin toybox_vendor/toolbox applets (symlinks from stock)
SM7225_COMMON_DIR := $(LOCAL_PATH)
TOYBOX_BIN_LINKS := $(TARGET_OUT_VENDOR)/bin/.toybox_links_done
$(TOYBOX_BIN_LINKS): $(LOCAL_INSTALLED_MODULE) $(SM7225_COMMON_DIR)/vendor_bin_symlinks.txt
	@echo "Creating vendor/bin applet symlinks"
	$(hide) while IFS=: read -r name target; do \
	    ln -sf $$target $(TARGET_OUT_VENDOR)/bin/$$name; \
	done < $(SM7225_COMMON_DIR)/vendor_bin_symlinks.txt
	$(hide) touch $@

ALL_DEFAULT_INSTALLED_MODULES += $(TOYBOX_BIN_LINKS)

# Camera sensor/tuning blobs: the 64-bit camera provider resolves these
# under /vendor/lib64/camera, but PRODUCT_COPY_FILES cannot install the
# same file to both vendor/lib/camera and vendor/lib64/camera on 22.2
# (generated prebuilt_rs_bitcode modules claim the lib64 path for their
# arm64 variant -> fsgen "packaging conflict"). Copy them directly.
CAM64_BINS := com.qti.tuned.default.bin \
    com.samsung.sensormodule.0_0_sony_imx355.bin \
    com.samsung.sensormodule.13_lsi_gc5035.bin \
    com.samsung.sensormodule.1_lsi_gc5035.bin \
    com.samsung.tuned.lsi_gc5035.bin \
    com.samsung.tuned.sony_imx355.bin \
    ois_mcu_stm32g_fw.bin

CAM64_SRC_DIR := vendor/samsung/sm7225-common/proprietary/vendor/lib64/camera
CAM64_STAMP := $(TARGET_OUT_INTERMEDIATES)/cam64_bins_done
$(CAM64_STAMP): $(LOCAL_INSTALLED_MODULE) $(addprefix $(CAM64_SRC_DIR)/,$(CAM64_BINS))
	@echo "Installing lib64 camera data blobs"
	@mkdir -p $(TARGET_OUT_VENDOR)/lib64/camera
	$(hide) for f in $(CAM64_BINS); do \
	    cp $(CAM64_SRC_DIR)/$$f $(TARGET_OUT_VENDOR)/lib64/camera/$$f; \
	done
	$(hide) touch $@

ALL_DEFAULT_INSTALLED_MODULES += $(CAM64_STAMP)

endif
