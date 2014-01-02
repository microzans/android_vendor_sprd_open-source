# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-----------------------------------
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_COMMON_LIB_SRC := $(shell cd $(LOCAL_PATH); ls lib/*.c)
LOCAL_SRC_FILES := $(LOCAL_C_COMMON_LIB_SRC)
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional
#-----------------------------------
LOCAL_SRC_FILES += main.c
LOCAL_MODULE := atd
include $(BUILD_EXECUTABLE)
include $(LOCAL_PATH)/marvell/Android.mk