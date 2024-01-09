#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # With Android Studio, the CMAKE_RUNTIME_OUTPUT_DIRECTORY, CMAKE_LIBRARY_OUTPUT_DIRECTORY and CMAKE_RUNTIME_OUTPUT_DIRECTORY are
    # already different per configuration. There's no need to do "CMAKE_RUNTIME_OUTPUT_DIRECTORY\Debug" as the output folder.
    # Having this extra configuration folder creates issues when copying the "runtime dependencies" files into the APK.
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${UCONF} CACHE)    # Just use the CMAKE_ARCHIVE_OUTPUT_DIRECTORY for all configurations
        unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UCONF} CACHE)    # Just use the CMAKE_LIBRARY_OUTPUT_DIRECTORY for all configurations
        unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF} CACHE)    # Just use the CMAKE_ARCHIVE_OUTPUT_DIRECTORY for all configurations
    endforeach()

    include(cmake/Platform/Common/Configurations_common.cmake)
    include(cmake/Platform/Common/Clang/Configurations_clang.cmake)

    set(_android_api_define)
    if(${LY_TOOLCHAIN_NDK_PKG_MAJOR} VERSION_LESS "23")
        set(_android_api_define __ANDROID_API__=${LY_TOOLCHAIN_NDK_API_LEVEL})
    endif()

    ly_append_configurations_options(
        DEFINES
            LINUX64
            _LINUX
            LINUX
            ANDROID
            MOBILE
            _HAS_C9X
            ENABLE_TYPE_INFO
            NDK_REV_MAJOR=${LY_TOOLCHAIN_NDK_PKG_MAJOR}
            NDK_REV_MINOR=${LY_TOOLCHAIN_NDK_PKG_MINOR}
            ${_android_api_define}

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

