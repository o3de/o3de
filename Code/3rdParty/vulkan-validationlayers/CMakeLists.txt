#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(vulkan-validationlayers
    VERSION "1.4.363.1"
    LICENSE "Apache-2.0"
    URL "https://github.com/KhronosGroup/Vulkan-ValidationLayers/archive/refs/tags/vulkan-sdk-1.4.363.1.tar.gz"
    URL_HASH "e79809a172639518c4a3a0703ec16dfc41f95a3c15e3f390745c51c326819790"
    GIT "https://github.com/KhronosGroup/Vulkan-ValidationLayers.git"
    GIT_HASH "317cff060893783f96d4174f50491e6374eb3714"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(vulkan-validationlayers)

file(
    COPY "${vulkan-validationlayers_SOURCE_DIR}/layers/external/vma/vma.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/vma"
)

o3de_disable_warnings()

ly_add_target(
    NAME vulkan-validationlayers MODULE
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    OUTPUT_NAME VkLayer_khronos_validation
    INCLUDE_DIRECTORIES
        PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}/Include"
            "${vulkan-validationlayers_SOURCE_DIR}/layers"
            "${vulkan-validationlayers_SOURCE_DIR}/layers/vulkan"
            "${vulkan-validationlayers_SOURCE_DIR}/layers/gpuav/spirv"
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::parallel-hashmap
            3rdParty::spirv-tools
            3rdParty::vulkan-memory-allocator
            3rdParty::vulkan-utility-libraries
    COMPILE_DEFINITIONS
        PRIVATE USE_CUSTOM_HASH_MAP
)

set_target_properties(vulkan-validationlayers PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

find_package(Threads REQUIRED)
target_link_libraries(vulkan-validationlayers PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(MSVC)
    target_compile_options(vulkan-validationlayers PRIVATE
        /GR
        /bigobj
    )
else()
    target_compile_options(vulkan-validationlayers PRIVATE -frtti)
endif()

if(WIN32)
    set_target_properties(vulkan-validationlayers PROPERTIES PREFIX "")

    target_compile_definitions(vulkan-validationlayers PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )

    target_sources(vulkan-validationlayers PRIVATE
        "${vulkan-validationlayers_SOURCE_DIR}/layers/VkLayer_khronos_validation.def"
    )

    set(JSON_LIBRARY_PATH ".\\\\VkLayer_khronos_validation.dll")
elseif(APPLE)
    set_target_properties(vulkan-validationlayers PROPERTIES SUFFIX ".dylib")

    target_link_options(vulkan-validationlayers PRIVATE
        "LINKER:-exported_symbols_list,${vulkan-validationlayers_SOURCE_DIR}/layers/VkLayer_khronos_validation.exp"
    )

    set(JSON_LIBRARY_PATH "./libVkLayer_khronos_validation.dylib")
elseif(ANDROID)
    ly_target_link_libraries(vulkan-validationlayers PRIVATE
        log
        android
    )

    target_link_options(vulkan-validationlayers PRIVATE
        "LINKER:-z,max-page-size=16384"
        "LINKER:--version-script=${vulkan-validationlayers_SOURCE_DIR}/layers/VkLayer_khronos_validation-android.map"
    )
else()
    target_link_options(vulkan-validationlayers PRIVATE
        "LINKER:--version-script=${vulkan-validationlayers_SOURCE_DIR}/layers/VkLayer_khronos_validation.map"
    )

    set(JSON_LIBRARY_PATH "./libVkLayer_khronos_validation.so")
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/vulkan-validationlayers
)

if(ANDROID)
    return()
endif()

set(JSON_LAYER_NAME VK_LAYER_KHRONOS_validation)

configure_file(
    "${vulkan-validationlayers_SOURCE_DIR}/layers/VkLayer_khronos_validation.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_khronos_validation.json"
    @ONLY
)

ly_add_target_files(
    TARGETS vulkan-validationlayers
    FILES "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_khronos_validation.json"
)

ly_install_files(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_khronos_validation.json"
    DESTINATION Code/3rdParty/vulkan-validationlayers/Platform/${PAL_PLATFORM_NAME}
)

set_property(DIRECTORY APPEND_STRING PROPERTY O3DE_SUBDIRECTORY_INSTALL_CODE [[
cmake_language(DEFER CALL ly_add_target_files
    TARGETS vulkan-validationlayers
    FILES "${LY_ROOT_FOLDER}/Code/3rdParty/vulkan-validationlayers/Platform/${PAL_PLATFORM_NAME}/VkLayer_khronos_validation.json"
)
]])
