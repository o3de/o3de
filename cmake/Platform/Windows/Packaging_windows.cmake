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

set(LY_WIX_PATH "" CACHE PATH "Path to the WiX install path")

if(LY_WIX_PATH)
    file(TO_CMAKE_PATH ${LY_QTIFW_PATH} CPACK_WIX_ROOT)
elseif(DEFINED ENV{WIX})
    file(TO_CMAKE_PATH $ENV{WIX} CPACK_WIX_ROOT)
endif()

if(CPACK_WIX_ROOT)
    if(NOT EXISTS ${CPACK_WIX_ROOT})
        message(FATAL_ERROR "Invalid path supplied for LY_WIX_PATH argument or WIX environment variable")
    endif()
else()
    # early out as no path to WiX has been supplied effectively disabling support
    return()
endif()

set(CPACK_GENERATOR "WIX")

# CPack will generate the WiX product/upgrade GUIDs further down the chain if they weren't supplied
# however, they are unique for each run.  instead, let's do the auto generation here and add it to
# the cache for run persistence.  an additional cache file will be used to store the information on
# the original generation so we still have the ability to detect if they are still being used.
set(_guid_cache_file "${CMAKE_BINARY_DIR}/installer/wix_guid_cache.cmake")
if(NOT EXISTS ${_guid_cache_file})
    set(_wix_guid_namespace "6D43F57A-2917-4AD9-B758-1F13CDB08593")

    # based the ISO-8601 standard (YYYY-MM-DDTHH-mm-ssTZD) e.g., 20210506145533
    string(TIMESTAMP _guid_gen_timestamp "%Y%m%d%H%M%S")

    file(WRITE ${_guid_cache_file} "set(_wix_guid_gen_timestamp ${_guid_gen_timestamp})\n")

    string(UUID _default_product_guid
        NAMESPACE ${_wix_guid_namespace}
        NAME "ProductID_${_guid_gen_timestamp}"
        TYPE SHA1
        UPPER
    )
    file(APPEND ${_guid_cache_file} "set(_wix_default_product_guid ${_default_product_guid})\n")

    string(UUID _default_upgrade_guid
        NAMESPACE ${_wix_guid_namespace}
        NAME "UpgradeCode_${_guid_gen_timestamp}"
        TYPE SHA1
        UPPER
    )
    file(APPEND ${_guid_cache_file} "set(_wix_default_upgrade_guid ${_default_upgrade_guid})\n")
endif()
include(${_guid_cache_file})

set(LY_WIX_PRODUCT_GUID "${_wix_default_product_guid}" CACHE STRING "GUID for the Product ID field. Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")
set(LY_WIX_UPGRADE_GUID "${_wix_default_upgrade_guid}" CACHE STRING "GUID for the Upgrade Code field. Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")

set(_uses_default_product_guid FALSE)
if(NOT LY_WIX_PRODUCT_GUID OR LY_WIX_PRODUCT_GUID STREQUAL ${_wix_default_product_guid})
    set(_uses_default_product_guid TRUE)
    set(LY_WIX_PRODUCT_GUID ${_wix_default_product_guid})
endif()

set(_uses_default_upgrade_guid FALSE)
if(NOT LY_WIX_UPGRADE_GUID OR LY_WIX_UPGRADE_GUID STREQUAL ${_wix_default_upgrade_guid})
    set(_uses_default_upgrade_guid TRUE)
    set(LY_WIX_UPGRADE_GUID ${_wix_default_upgrade_guid})
endif()

if(_uses_default_product_guid OR _uses_default_upgrade_guid)
    message(STATUS "One or both WiX GUIDs were auto generated.  It is recommended you supply your own GUIDs through LY_WIX_PRODUCT_GUID and LY_WIX_UPGRADE_GUID.")

    if(_uses_default_product_guid)
        message(STATUS "-> Default LY_WIX_PRODUCT_GUID = ${LY_WIX_PRODUCT_GUID}")
    endif()

    if(_uses_default_upgrade_guid)
        message(STATUS "-> Default LY_WIX_UPGRADE_GUID = ${LY_WIX_UPGRADE_GUID}")
    endif()
endif()

set(CPACK_WIX_PRODUCT_GUID ${LY_WIX_PRODUCT_GUID})
set(CPACK_WIX_UPGRADE_GUID ${LY_WIX_UPGRADE_GUID})

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Platform/Windows/PackagingTemplate.wxs.in")
