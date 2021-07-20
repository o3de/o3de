#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(VKVALIDATION_RUNTIME_DEPENDENCIES $<$<NOT:$<CONFIG:Release>>:${LY_NDK_DIR}/sources/third_party/vulkan/src/build-android/jniLibs/arm64-v8a/libVkLayer_khronos_validation.so>)
