#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(openxr-sdk
    VERSION "1.1.65"
    LICENSE "Apache-2.0 AND MIT AND BSL-1.0"
    URL "https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.1.65.tar.gz"
    URL_HASH "27aa797baf680c5f836b6138e78e59e2f72d6becfef6d4587ed9a57756d6d3b3"
    GIT "https://github.com/KhronosGroup/OpenXR-SDK.git"
    GIT_HASH "24a1f2b92d212501a2b1b8d30ffd53abf55fcfa8"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(openxr-sdk)

file(COPY
    "${openxr-sdk_SOURCE_DIR}/include/openxr"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
    PATTERN "CMakeLists.txt" EXCLUDE
)

include(CheckFunctionExists)
check_function_exists(secure_getenv HAVE_SECURE_GETENV)
check_function_exists(__secure_getenv HAVE___SECURE_GETENV)
configure_file(
    "${openxr-sdk_SOURCE_DIR}/src/common_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/common_config.h"
)

find_package(Threads REQUIRED GLOBAL)

o3de_disable_warnings()

ly_add_target(
    NAME openxr-sdk STATIC
    NAMESPACE 3rdParty
    OUTPUT_NAME openxr_loader
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/Include"
        PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${openxr-sdk_SOURCE_DIR}/src"
            "${openxr-sdk_SOURCE_DIR}/src/common"
    COMPILE_DEFINITIONS
        PRIVATE
            OPENXR_HAVE_COMMON_CONFIG
            XR_USE_GRAPHICS_API_VULKAN
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::jsoncpp
            3rdParty::vulkan-headers
            Threads::Threads
            ${CMAKE_DL_LIBS}
)

set_target_properties(openxr-sdk PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)
target_compile_features(openxr-sdk PRIVATE cxx_std_17)
target_compile_options(openxr-sdk PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

if(WIN32)
    target_compile_definitions(openxr-sdk PRIVATE
        NOMINMAX
        XR_OS_WINDOWS
        XR_USE_PLATFORM_WIN32
        XR_USE_GRAPHICS_API_D3D11
        XR_USE_GRAPHICS_API_D3D12
    )
    ly_target_link_libraries(openxr-sdk PRIVATE advapi32)
elseif(ANDROID)
    target_compile_definitions(openxr-sdk PRIVATE XR_OS_ANDROID XR_USE_PLATFORM_ANDROID)
    target_include_directories(openxr-sdk PRIVATE
        "${openxr-sdk_SOURCE_DIR}/src/external/android-jni-wrappers"
        "${openxr-sdk_SOURCE_DIR}/src/external/jnipp"
    )
    ly_target_link_libraries(openxr-sdk PRIVATE android log)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(openxr-sdk PRIVATE
        XR_OS_LINUX
        SYSCONFDIR="/usr/local/etc"
        EXTRASYSCONFDIR="/etc"
    )
elseif(APPLE)
    target_compile_definitions(openxr-sdk PRIVATE
        XR_OS_APPLE
        $<$<PLATFORM_ID:Darwin>:XR_USE_PLATFORM_MACOS>
        $<$<PLATFORM_ID:iOS>:XR_USE_PLATFORM_IOS>
    )
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/openxr-sdk
)
