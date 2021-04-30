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

set(CPACK_GENERATOR "ZIP")

set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${LY_VERSION_STRING}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")

string(TOLOWER ${PROJECT_NAME} _project_name_lower)
set(CPACK_PACKAGE_FILE_NAME "${_project_name_lower}_installer")

set(DEFAULT_LICENSE_NAME "Apache-2.0")
set(DEFAULT_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

set(CPACK_RESOURCE_FILE_LICENSE ${DEFAULT_LICENSE_FILE})

# IMPORTANT: required to be included AFTER setting all property overrides
include(CPack REQUIRED)

function(ly_configure_cpack_component ly_configure_cpack_component_NAME)

    set(options REQUIRED)
    set(oneValueArgs DISPLAY_NAME DESCRIPTION LICENSE_NAME LICENSE_FILE)
    set(multiValueArgs)

    cmake_parse_arguments(ly_configure_cpack_component "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # default to optional
    set(component_type DISABLED)

    if(ly_configure_cpack_component_REQUIRED)
        set(component_type REQUIRED)
    endif()

    set(license_name ${DEFAULT_LICENSE_NAME})
    set(license_file ${DEFAULT_LICENSE_FILE})

    if(ly_configure_cpack_component_LICENSE_NAME AND ly_configure_cpack_component_LICENSE_FILE)
        set(license_name ${ly_configure_cpack_component_LICENSE_NAME})
        set(license_file ${ly_configure_cpack_component_LICENSE_FILE})
    elseif(ly_configure_cpack_component_LICENSE_NAME OR ly_configure_cpack_component_LICENSE_FILE)
        message(FATAL_ERROR "Invalid argument configuration. Both LICENSE_NAME and LICENSE_FILE must be set for ly_configure_cpack_component")
    endif()

    cpack_add_component(
        ${ly_configure_cpack_component_NAME} ${component_type}
        DISPLAY_NAME ${ly_configure_cpack_component_DISPLAY_NAME}
        DESCRIPTION ${ly_configure_cpack_component_DESCRIPTION}
    )
endfunction()

# configure ALL components here
ly_configure_cpack_component(
    ${LY_DEFAULT_INSTALL_COMPONENT} REQUIRED
    DISPLAY_NAME "${PROJECT_NAME} Core"
    DESCRIPTION "${PROJECT_NAME} Headers, Libraries and Tools"
)
