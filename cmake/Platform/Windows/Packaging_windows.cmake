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

set(CPACK_WIX_ROOT "" CACHE PATH "Path to the WiX install path")

if(CPACK_WIX_ROOT)
    if(NOT EXISTS ${CPACK_WIX_ROOT})
        message(FATAL_ERROR "Invalid path supplied for CPACK_WIX_ROOT argument")
    endif()
else()
    # early out as no path to WiX has been supplied effectively disabling support
    return()
endif()

set(CPACK_GENERATOR WIX)

set(_cmake_package_name "cmake-${CPACK_DESIRED_CMAKE_VERSION}-windows-x86_64")
set(CPACK_CMAKE_PACKAGE_FILE "${_cmake_package_name}.zip")
set(CPACK_CMAKE_PACKAGE_HASH "15a49e2ab81c1822d75b1b1a92f7863f58e31f6d6aac1c4103eef2b071be3112")

# CPack will generate the WiX product/upgrade GUIDs further down the chain if they weren't supplied
# however, they are unique for each run.  instead, let's do the auto generation here and add it to
# the cache for run persistence and have the ability to detect if they are still being used.
set(_wix_guid_namespace "6D43F57A-2917-4AD9-B758-1F13CDB08593")

function(generate_wix_guid out_value seed)
    string(UUID _guid
        NAMESPACE ${_wix_guid_namespace}
        NAME ${seed}
        TYPE SHA1
        UPPER
    )

    set(${out_value} ${_guid} PARENT_SCOPE)
endfunction()

set(_guid_seed_base "${PROJECT_NAME}_${LY_VERSION_STRING}")
generate_wix_guid(_wix_default_product_guid "${_guid_seed_base}_ProductID" )
generate_wix_guid(_wix_default_upgrade_guid "${_guid_seed_base}_UpgradeCode")

set(LY_WIX_PRODUCT_GUID "" CACHE STRING "GUID for the Product ID field. Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")
set(LY_WIX_UPGRADE_GUID "" CACHE STRING "GUID for the Upgrade Code field. Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")

# clear previously cached default values to correct future runs.  this will
# unfortunately only work if the seed properties still haven't changed
if(LY_WIX_PRODUCT_GUID STREQUAL ${_wix_default_product_guid})
    unset(LY_WIX_PRODUCT_GUID CACHE)
endif()
if(LY_WIX_UPGRADE_GUID STREQUAL ${_wix_default_upgrade_guid})
    unset(LY_WIX_UPGRADE_GUID CACHE)
endif()

if(NOT (LY_WIX_PRODUCT_GUID AND LY_WIX_UPGRADE_GUID))
    message(STATUS "One or both WiX GUIDs were auto generated.  It is recommended you supply your own GUIDs through LY_WIX_PRODUCT_GUID and LY_WIX_UPGRADE_GUID.")

    if(NOT LY_WIX_PRODUCT_GUID)
        set(LY_WIX_PRODUCT_GUID ${_wix_default_product_guid})
        message(STATUS "-> Default LY_WIX_PRODUCT_GUID = ${LY_WIX_PRODUCT_GUID}")
    endif()

    if(NOT LY_WIX_UPGRADE_GUID)
        set(LY_WIX_UPGRADE_GUID ${_wix_default_upgrade_guid})
        message(STATUS "-> Default LY_WIX_UPGRADE_GUID = ${LY_WIX_UPGRADE_GUID}")
    endif()
endif()

set(CPACK_WIX_PRODUCT_GUID ${LY_WIX_PRODUCT_GUID})
set(CPACK_WIX_UPGRADE_GUID ${LY_WIX_UPGRADE_GUID})

set(CPACK_WIX_PRODUCT_LOGO ${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/product_logo.png)
set(CPACK_WIX_PRODUCT_ICON ${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/product_icon.ico)

set(CPACK_WIX_TEMPLATE "${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/Template.wxs.in")

set(CPACK_WIX_EXTRA_SOURCES
    "${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/PostInstallSetup.wxs"
    "${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/Shortcuts.wxs"
)

set(CPACK_WIX_EXTENSIONS
    WixUtilExtension
)

set(_embed_artifacts "yes")

if(LY_INSTALLER_DOWNLOAD_URL)
    set(_embed_artifacts "no")

    # the bootstrapper will at the very least need a different upgrade guid
    generate_wix_guid(CPACK_WIX_BOOTSTRAP_UPGRADE_GUID "${_guid_seed_base}_Bootstrap_UpgradeCode")

    set(CPACK_POST_BUILD_SCRIPTS
        ${CPACK_SOURCE_DIR}/Platform/Windows/PackagingPostBuild.cmake
    )
endif()

set(CPACK_WIX_CANDLE_EXTRA_FLAGS
    -dCPACK_EMBED_ARTIFACTS=${_embed_artifacts}
    -dCPACK_CMAKE_PACKAGE_NAME=${_cmake_package_name}
)
