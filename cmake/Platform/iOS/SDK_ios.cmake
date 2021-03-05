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


# Detect the ios SDK Path and set the SYSROOT
find_program(XCRUN_PROG "xcrun")
execute_process(COMMAND ${XCRUN_PROG} --sdk iphoneos --show-sdk-path
                OUTPUT_VARIABLE LY_IOS_SDK_PATH
                RESULTS_VARIABLE GET_IOS_SDK_RESULT)
if (NOT GET_IOS_SDK_RESULT EQUAL 0)
    message(FATAL_ERROR "Unable to determine the iOS SDK path")
endif()
string(STRIP ${LY_IOS_SDK_PATH} LY_IOS_SDK_PATH)

set(CMAKE_SYSROOT ${LY_IOS_SDK_PATH})

set(SDKROOT "iphoneos")

set(DEVROOT "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer")

set(CMAKE_OSX_SYSROOT "${SDKROOT}")

