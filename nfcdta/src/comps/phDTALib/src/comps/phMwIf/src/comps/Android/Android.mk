$(info "NXP-NFC-DTA>Building Middleware Infterface..")

# function to find all *.cpp files under a directory
define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_PATH:= $(call my-dir)/../../../../
D_CFLAGS := -DANDROID -DBUILDCFG=1
#NXP PN547 Enable
D_CFLAGS += -DNFC_NXP_NOT_OPEN_INCLUDED=TRUE
######################################
# Build shared library system/lib/libmwif.so for stack code.
include $(CLEAR_VARS)
MW_IF := phMwIf
OSAL := phDTAOSAL
LIBNFC_NCI_PATH:= external/libnfc-nci
NFA := src/nfa
NFC := src/nfc
HAL := src/hal
UDRV := src/udrv

LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libmwif
LOCAL_MODULE_TAGS := optional
#LOCAL_SHARED_LIBRARIES := libhardware_legacy libcutils liblog libdl libstlport libhardware

LOCAL_CFLAGS := $(D_CFLAGS)

#Handle DATA chaining in DTA for L Release
#For lower versions Data chaining is already handled by Middleware
ifeq ($(PLATFORM_SDK_VERSION), 21)
    LOCAL_CFLAGS += -DAPP_HANDLE_DATA_CHAINING
endif

LOCAL_C_INCLUDES := external/stlport/stlport bionic/ \
    $(LOCAL_PATH)/$(MW_IF)/inc \
    $(LOCAL_PATH)/$(OSAL)/inc \
    $(LOCAL_PATH)/$(MW_IF)/src/comps/Android/inc \
    $(LIBNFC_NCI_PATH)/src/include \
    $(LIBNFC_NCI_PATH)/src/gki/ulinux \
    $(LIBNFC_NCI_PATH)/src/gki/common \
    $(LIBNFC_NCI_PATH)/$(NFA)/include \
    $(LIBNFC_NCI_PATH)/$(NFA)/int \
    $(LIBNFC_NCI_PATH)/$(NFC)/include \
    $(LIBNFC_NCI_PATH)/$(NFC)/int \
    $(LIBNFC_NCI_PATH)/src/hal/include \
    $(LIBNFC_NCI_PATH)/src/hal/int \
    $(LOCAL_PATH)/phDTAOSAL/src/comps/Android/inc
    
LOCAL_SRC_FILES := \
    $(call all-c-files-under, $(MW_IF)/src/comps/Android/src) \
    $(call all-cpp-files-under, $(MW_IF)/src/comps/Android/src) 

LOCAL_SHARED_LIBRARIES := \
    libnativehelper \
    libcutils \
    libutils \
    libnfc-nci \
    libosal\
    libhardware \
    libc \
    libdl

include $(BUILD_SHARED_LIBRARY)

