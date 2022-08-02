#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_ANDROID_VULKAN_VALIDATION_PATH "${LY_NDK_DIR}/sources/third_party/vulkan/src/build-android/jniLibs" CACHE PATH "Path to the Vulkan Validation Layers libs for Android")

if(EXISTS ${LY_ANDROID_VULKAN_VALIDATION_PATH})
    set(VKVALIDATION_RUNTIME_DEPENDENCIES $<$<NOT:$<CONFIG:Release>>:${LY_ANDROID_VULKAN_VALIDATION_PATH}/arm64-v8a/libVkLayer_khronos_validation.so>)
else()
    message(WARNING
        "Android Vulkan validation layer libs not found at ${LY_ANDROID_VULKAN_VALIDATION_PATH}. "
        "If using Android NDK r23 or above, these libs are distributed separately via https://github.com/KhronosGroup/Vulkan-ValidationLayers. "
        "Vulkan validation will not be available.")
endif()
