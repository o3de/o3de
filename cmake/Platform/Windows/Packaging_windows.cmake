#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_INSTALLER_WIX_ROOT "" CACHE PATH "Path to the WiX install path")

if(LY_INSTALLER_WIX_ROOT)
    if(NOT EXISTS ${LY_INSTALLER_WIX_ROOT})
        message(FATAL_ERROR "Invalid path supplied for LY_INSTALLER_WIX_ROOT argument")
    endif()
else()
    # early out as no path to WiX has been supplied effectively disabling support
    return()
endif()

# IMPORTANT: CPACK_WIX_ROOT is a built-in variable that is required to propagate the path supplied
# via command line down to the cpack internals
set(CPACK_WIX_ROOT ${LY_INSTALLER_WIX_ROOT})

set(CPACK_GENERATOR WIX)

set(_cmake_package_name "cmake-${CPACK_DESIRED_CMAKE_VERSION}-windows-x86_64")
set(CPACK_CMAKE_PACKAGE_FILE "${_cmake_package_name}.zip")
set(CPACK_CMAKE_PACKAGE_HASH "15a49e2ab81c1822d75b1b1a92f7863f58e31f6d6aac1c4103eef2b071be3112")

# workaround for shortening the path cpack installs to by stripping the platform directory
set(CPACK_TOPLEVEL_TAG "")

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

set(_hyperlink_license [[
        <Hypertext Name="EulaHyperlink" X="42" Y="202" Width="-42" Height="51" TabStop="yes" FontId="1" HideWhenDisabled="yes">#(loc.InstallEulaAcceptance)</Hypertext>
]])

set(_raw_text_license [[
        <Richedit Name="EulaRichedit" X="42" Y="202" Width="-42" Height="-84" TabStop="yes" FontId="2" HexStyle="0x800000" />
        <Text Name="EulaAcceptance" X="42" Y="-56" Width="-42" Height="18" TabStop="yes" FontId="1" HideWhenDisabled="yes">#(loc.InstallEulaAcceptance)</Text>
]])

set(WIX_THEME_WARNING_IMAGE ${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/warning.png)

if(LY_INSTALLER_LICENSE_URL)
    set(WIX_THEME_INSTALL_LICENSE_ELEMENTS ${_hyperlink_license})
    set(WIX_THEME_EULA_ACCEPTANCE_TEXT "&lt;a href=\"#\"&gt;Terms of Use&lt;/a&gt;")
else()
    set(WIX_THEME_INSTALL_LICENSE_ELEMENTS ${_raw_text_license})
    set(WIX_THEME_EULA_ACCEPTANCE_TEXT "Terms of Use above")
endif()

# theme ux file
configure_file(
    "${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/BootstrapperTheme.xml.in"
    "${CPACK_BINARY_DIR}/BootstrapperTheme.xml"
    @ONLY
)

# theme localization file
configure_file(
    "${CPACK_SOURCE_DIR}/Platform/Windows/Packaging/BootstrapperTheme.wxl.in"
    "${CPACK_BINARY_DIR}/BootstrapperTheme.wxl"
    @ONLY
)

set(_embed_artifacts "no")

# the bootstrapper will at the very least need a different upgrade guid
generate_wix_guid(CPACK_WIX_BOOTSTRAP_UPGRADE_GUID "${_guid_seed_base}_Bootstrap_UpgradeCode")

set(CPACK_WIX_CANDLE_EXTRA_FLAGS
    -dCPACK_EMBED_ARTIFACTS=${_embed_artifacts}
    -dCPACK_CMAKE_PACKAGE_NAME=${_cmake_package_name}
)
