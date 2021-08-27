#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    include(cmake/Platform/Common/Configurations_common.cmake)
    include(cmake/Platform/Common/Clang/Configurations_clang.cmake)

    ly_append_configurations_options(
        DEFINES
            LINUX64
            __ARM_NEON__
            _LINUX
            LINUX
            ANDROID
            MOBILE
            _HAS_C9X
            ENABLE_TYPE_INFO
            __ANDROID_API__=${LY_TOOLCHAIN_NDK_API_LEVEL}
            NDK_REV_MAJOR=${LY_TOOLCHAIN_NDK_PKG_MAJOR}
            NDK_REV_MINOR=${LY_TOOLCHAIN_NDK_PKG_MINOR}

        COMPILATION
            -femulated-tls           # All accesses to TLS variables are converted to calls to __emutls_get_address in the runtime library
            -ffast-math              # Allow aggressive, lossy floating-point optimizations,
            -fno-aligned-allocation  # Disable use of C++17 aligned_alloc for operator new/delete

        COMPILATION_DEBUG
            -gdwarf-2            # DWARF 2 debugging information

        COMPILATION_PROFILE
            -g                  # debugging information
            -gdwarf-2           # DWARF 2 debugging information

        LINK_NON_STATIC
            -rdynamic            # add ALL symbols to the dynamic symbol table
            -Wl,--no-undefined   # tell the gcc linker to fail if it finds undefined references
            -Wl,--gc-sections    # discards unused sections

            -landroid            # Android Library
            -llog                # log library for android
            -lc++_shared
            -ldl                 # Dynamic
            -stdlib=libc++

            -u ANativeActivity_onCreate

            -L${LY_NDK_ABI_ROOT}/usr/lib
            -L${LY_NDK_SRC_ROOT}/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}

        LINK_NON_STATIC_DEBUG
            -Wl,--build-id       # Android Studio needs the libraries to have an id in order to match them with what"s running on the device.
            -shared

        LINK_NON_STATIC_PROFILE
            -Wl,--build-id       # Android Studio needs the libraries to have an id in order to match them with what"s running on the device.
            -shared

    )
    ly_set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions -fno-aligned-allocation -stdlib=libc++")

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

    ly_set(CMAKE_CXX_EXTENSIONS OFF)

    include(cmake/Platform/Common/TargetIncludeSystemDirectories_supported.cmake)
else()

    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")

endif()

