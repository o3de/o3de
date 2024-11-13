#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    ly_set(CMAKE_RC_FLAGS /nologo)   
    
    include(cmake/Platform/Common/MSVC/Configurations_msvc.cmake)

    ly_append_configurations_options(
        DEFINES
            _WIN32
            WIN32
            _WIN64
            WIN64
            NOMINMAX
        LINK
            /MACHINE:X64
    )

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    if(MSVC)
        include(cmake/Platform/Common/MSVC/Configurations_clang.cmake)
    else()
        include(cmake/Platform/Common/Clang/Configurations_clang.cmake)
    endif()

    ly_append_configurations_options(
        DEFINES
            _ENABLE_EXTENDED_ALIGNED_STORAGE #Enables support for extended alignment for the MSVC std::aligned_storage class
            _WIN32
            WIN32
            _WIN64
            WIN64
            NOMINMAX
        COMPILATION
            -msse3
            -mf16c
            -Wno-deprecated-declarations
    )
    ly_set(CMAKE_CXX_EXTENSIONS OFF)

else()

    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")

endif()

get_property(O3DE_SCRIPT_ONLY GLOBAL PROPERTY "O3DE_SCRIPT_ONLY")

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio" AND NOT O3DE_SCRIPT_ONLY)

    if(DEFINED ENV{CMAKE_WINDOWS_KITS_10_DIR})
        set(win10_root $ENV{CMAKE_WINDOWS_KITS_10_DIR})
        file(TO_CMAKE_PATH "${win10_root}" win10_root)
        list(APPEND win10_roots "${win10_root}")
    endif()

    # This logic is taken from cmGlobalVisualStudio14Generator.cxx, which
    # itself is taken from the vcvarsqueryregistry.bat file from VS2015.
    # Try HKLM and then HKCU.
    list(APPEND win10_roots "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]")
    list(APPEND win10_roots "[HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]")

    foreach(win10_root IN LISTS win10_roots)
        get_filename_component(_win10_root "${win10_root}" ABSOLUTE CACHE)

        # Grab the paths of the different SDKs that are installed
        file(GLOB candidates "${_win10_root}/Include/*")
        foreach(sdk_dir IN LISTS candidates)

            # Skip SDKs that do not contain <um/windows.h> because that
            # indicates that only the UCRT MSIs were installed for them.
            if(EXISTS "${sdk_dir}/um/Windows.h")
                list(APPEND sdks "${sdk_dir}")
            endif()
        endforeach()
        if(sdks)
            break()
        endif()
    endforeach()

    if(NOT sdks)
        return()
    endif()

    list(GET sdks 0 max_sdk)
    foreach(sdk IN LISTS sdks)
        # Only use the filename, which will be the SDK version.
        get_filename_component(version "${sdk}" NAME)

        # Look for a SDK exactly matching the requested target version
        if(version VERSION_EQUAL CMAKE_SYSTEM_VERSION)
            set(max_sdk "${version}")
            set(sdk_root "${sdk}")
            break()
        elseif(version VERSION_GREATER max_sdk)
            # Use the latest Windows 10 SDK since the exact version is not
            # available
            set(max_sdk "${version}")
            set(sdk_root "${sdk}")
        endif()
    endforeach()

    if(NOT version VERSION_EQUAL CMAKE_SYSTEM_VERSION)
        message(STATUS "Using Windows SDK version ${version} to target Windows ${CMAKE_SYSTEM_VERSION}")
    endif()

    ly_set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "${version}")

endif()

if(NOT O3DE_SCRIPT_ONLY AND NOT CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION MATCHES "10.0")
    message(FATAL_ERROR "Unsupported version of Windows SDK ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}, specify \"-DCMAKE_SYSTEM_VERSION=10.0\" when invoking cmake")
endif()
