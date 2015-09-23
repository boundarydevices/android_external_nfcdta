$(info "NXP-NFC-DTA>Building DTA Application..")
LOCAL_PATH := $(call my-dir)
###########################################
# Build DTA.apk
include $(CLEAR_VARS)
# Build all java files in the java subdirectory
 
# Name of the APK to build
LOCAL_PACKAGE_NAME := NxpDTA
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_TAGS := tests

ifeq ($(PLATFORM_SDK_VERSION),21)
$(warning "Building for L release or Later")
LOCAL_SRC_FILES := $(call all-java-files-under, src/com) $/../JNI/PhNXPJniHelper.java  $/src/l/PhCustomSNEPRTD.java
else
$(warning "Building for KK release or Lower")
LOCAL_SRC_FILES := $(call all-java-files-under, src/com) $/../JNI/PhNXPJniHelper.java  $/src/kk/PhCustomSNEPRTD.java
endif

# Tell it to build an APK
include $(BUILD_PACKAGE)
