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


if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

    include(cmake/Platform/Common/Clang/Configurations_clang.cmake)

    ly_append_configurations_options(
        DEFINES
            APPLE
            IOS
            MOBILE
            APPLE_BUNDLE
        COMPILATION
            -miphoneos-version-min=${LY_IOS_DEPLOYMENT_TARGET}
            -Wno-shorten-64-to-32
            -fno-aligned-allocation
        LINK_NON_STATIC

            -Wl,-dead_strip
            -miphoneos-version-min=${LY_IOS_DEPLOYMENT_TARGET}

            -lpthread
    )
    set(CMAKE_CXX_EXTENSIONS OFF)
else()

    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")

endif()

# Signing
set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS --deep)

# Generate scheme files for Xcode
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Make modules have the dylib extension
set(CMAKE_SHARED_MODULE_SUFFIX .dylib)
