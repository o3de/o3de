#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(vulkan-utility-libraries
    VERSION "1.4.363.0"
    LICENSE "Apache-2.0"
    URL "https://github.com/KhronosGroup/Vulkan-Utility-Libraries/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz"
    URL_HASH "a5308b62e3bac84cf45c4d7dd5b830aabace4847fdb71f4fd7db3be034c2bebf"
    GIT "https://github.com/KhronosGroup/Vulkan-Utility-Libraries.git"
    GIT_HASH "57f01541b3959f2528f769043396e882851a7e75"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(vulkan-utility-libraries)

file(
    COPY "${vulkan-utility-libraries_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME vulkan-utility-libraries STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    BUILD_DEPENDENCIES
        PUBLIC 3rdParty::vulkan-headers
    COMPILE_DEFINITIONS
        PUBLIC VK_ENABLE_BETA_EXTENSIONS
)

if(WIN32)
    target_compile_definitions(vulkan-utility-libraries PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif(ANDROID)
    target_compile_definitions(vulkan-utility-libraries PUBLIC VK_USE_PLATFORM_ANDROID_KHR)
elseif(APPLE)
    target_compile_definitions(vulkan-utility-libraries PUBLIC VK_USE_PLATFORM_METAL_EXT)

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        target_compile_definitions(vulkan-utility-libraries PUBLIC VK_USE_PLATFORM_IOS_MVK)
    else()
        target_compile_definitions(vulkan-utility-libraries PUBLIC VK_USE_PLATFORM_MACOS_MVK)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    target_include_directories(vulkan-utility-libraries PUBLIC ${X11_INCLUDE_DIR})
    target_compile_definitions(vulkan-utility-libraries PUBLIC
        VK_USE_PLATFORM_XCB_KHR
        VK_USE_PLATFORM_XLIB_KHR
        VK_USE_PLATFORM_XLIB_XRANDR_EXT
        VK_USE_PLATFORM_WAYLAND_KHR
    )
endif()

set_target_properties(vulkan-utility-libraries PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/vulkan-utility-libraries
)
