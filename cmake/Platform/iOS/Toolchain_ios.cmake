#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#


set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_OSX_ARCHITECTURES arm64)


set(LY_IOS_CODE_SIGNING_IDENTITY "iPhone Developer" CACHE STRING "iPhone Developer")
set(LY_IOS_DEPLOYMENT_TARGET "13.0" CACHE STRING "iOS Deployment Target")
set(LY_IOS_DEVELOPMENT_TEAM "CF9TGN983S" CACHE STRING "The development team ID")


# PAL variables
set(PAL_PLATFORM_NAME iOS)
string(TOLOWER ${PAL_PLATFORM_NAME} PAL_PLATFORM_NAME_LOWERCASE)

include(${CMAKE_CURRENT_LIST_DIR}/SDK_ios.cmake)

set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${LY_IOS_DEPLOYMENT_TARGET})
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE NO)

set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${LY_IOS_CODE_SIGNING_IDENTITY})
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${LY_IOS_DEVELOPMENT_TEAM})
