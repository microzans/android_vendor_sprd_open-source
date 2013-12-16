# This makefile is used to generate device tree image

#----------------------------------------------------------------------
# Generate device tree image (dt.img)
#----------------------------------------------------------------------
ifeq ($(strip $(BOARD_KERNEL_SEPARATED_DT)),true)
ifeq ($(strip $(BUILD_TINY_ANDROID)),true)
include vendor/sprd/open-source/tools/dt/dtbtool/Android.mk
endif


DTBTOOL := $(HOST_OUT_EXECUTABLES)/dtbTool$(HOST_EXECUTABLE_SUFFIX)

INSTALLED_DTIMAGE_TARGET := $(PRODUCT_OUT)/dt.img

define build-dtimage-target
    $(call pretty,"Target dt image: $(INSTALLED_DTIMAGE_TARGET)")
    $(DTBTOOL) -o $@ -s $(BOARD_KERNEL_PAGESIZE) -p $(KERNEL_OUT)/scripts/dtc/ $(KERNEL_OUT)/arch/arm/boot/dts/
    $(hide) chmod a+r $@
endef

$(INSTALLED_DTIMAGE_TARGET): $(DTBTOOL) $(INSTALLED_KERNEL_TARGET)
	$(build-dtimage-target)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_DTIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_DTIMAGE_TARGET)
endif

