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

if(NOT PAL_TRAIT_BUILD_CPACK_SUPPORTED)
    return()
endif()

set(LY_QTIFW_PATH "" CACHE PATH "Path to the Qt Installer Framework install path")

if(LY_QTIFW_PATH)
    file(TO_CMAKE_PATH ${LY_QTIFW_PATH} CPACK_IFW_ROOT)
elseif(DEFINED ENV{QTIFWDIR})
    file(TO_CMAKE_PATH $ENV{QTIFWDIR} CPACK_IFW_ROOT)
endif()
if(NOT EXISTS ${CPACK_IFW_ROOT})
    message(STATUS "WARN: A valid LY_QTIFW_PATH argument or QTIFWDIR environment variable is required to enable cpack support")
    return()
endif()

set(CPACK_GENERATOR "IFW")

set(CPACK_PACKAGE_VENDOR "O3DE")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")

set(CPACK_PACKAGE_FILE_NAME "o3de_installer")

set(DEFAULT_LICENSE_NAME "Apache 2.0")
set(DEFAULT_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

set(CPACK_RESOURCE_FILE_LICENSE ${DEFAULT_LICENSE_FILE})

set(CPACK_IFW_PACKAGE_TITLE "O3DE Installer")
set(CPACK_IFW_PACKAGE_PUBLISHER "O3DE")

set(CPACK_IFW_TARGET_DIRECTORY "@ApplicationsDir@/O3DE/${LY_VERSION_STRING}")
set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY "O3DE")

# IMPORTANT: required to be included AFTER setting all property overrides
include(CPack REQUIRED)
include(CPackIFW REQUIRED)

function(ly_configure_cpack_component ly_configure_cpack_component_NAME)

    set(options REQUIRED)
    set(oneValueArgs DISPLAY_NAME DESCRIPTION LICENSE_NAME LICENSE_FILE)
    set(multiValueArgs)

    cmake_parse_arguments(ly_configure_cpack_component "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # default to optional
    set(component_type DISABLED)
    set(ifw_component_type)

    if(ly_configure_cpack_component_REQUIRED)
        set(component_type REQUIRED)
        set(ifw_component_type FORCED_INSTALLATION)
    endif()

    set(license_name ${DEFAULT_LICENSE_NAME})
    set(license_file ${DEFAULT_LICENSE_FILE})

    if(ly_configure_cpack_component_LICENSE_NAME AND ly_configure_cpack_component_LICENSE_FILE)
        set(license_name ${ly_configure_cpack_component_LICENSE_NAME})
        set(license_file ${ly_configure_cpack_component_LICENSE_FILE})
    elseif(ly_configure_cpack_component_LICENSE_NAME OR ly_configure_cpack_component_LICENSE_FILE)
        message(WARNING "Invalid argument configuration. Both LICENSE_NAME and LICENSE_FILE must be set for ly_configure_cpack_component")
    endif()

    cpack_add_component(
        ${ly_configure_cpack_component_NAME} ${component_type}
        DISPLAY_NAME ${ly_configure_cpack_component_DISPLAY_NAME}
        DESCRIPTION ${ly_configure_cpack_component_DESCRIPTION}
    )

    cpack_ifw_configure_component(
        ${ly_configure_cpack_component_NAME} ${ifw_component_type}
        LICENSES
            ${license_name}
            ${license_file}
    )
endfunction()

# configure ALL components here
ly_configure_cpack_component(
    ${LY_DEFAULT_INSTALL_COMPONENT} REQUIRED
    DISPLAY_NAME "O3DE Core"
    DESCRIPTION "O3DE Headers and Libraries"
)