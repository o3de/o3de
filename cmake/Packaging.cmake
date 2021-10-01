#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_CPACK_SUPPORTED)
    return()
endif()

# public facing options will be used for conversion into cpack specific ones below.
set(LY_INSTALLER_LICENSE_URL "" CACHE STRING "Optionally embed a link to the license instead of raw text")

set(LY_INSTALLER_AUTO_GEN_TAG OFF CACHE BOOL
"Automatically generate a build tag based on the git repo and append it to the download/upload URLs.  \
Format: <branch>/<commit_date>-<commit_hash>"
)

set(LY_INSTALLER_DOWNLOAD_URL "" CACHE STRING
"Base URL embedded into the installer to download additional artifacts, the host target and version \
number will automatically appended as '<version>/<host>'.  If LY_INSTALLER_AUTO_GEN_TAG is set, the \
full URL format will be: <base_url>/<build_tag>/<host>"
)

set(LY_INSTALLER_UPLOAD_URL "" CACHE STRING
"Base URL used to upload the installer artifacts after generation, the host target and version number \
will automatically appended as '<version>/<host>'.  If LY_INSTALLER_AUTO_GEN_TAG is set, the full URL \
format will be: <base_url>/<build_tag>/<host>.  Can also be set via LY_INSTALLER_UPLOAD_URL environment \
variable.  Currently only accepts S3 URLs e.g. s3://<bucket>/<prefix>"
)

set(LY_INSTALLER_AWS_PROFILE "" CACHE STRING
"AWS CLI profile for uploading artifacts.  Can also be set via LY_INSTALLER_AWS_PROFILE environment variable."
)

set(CPACK_DESIRED_CMAKE_VERSION 3.20.2)

# set all common cpack variable overrides first so they can be accessible via configure_file
# when the platform specific settings are applied below.  additionally, any variable with
# the "CPACK_" prefix will automatically be cached for use in any phase of cpack namely
# pre/post build
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_FULL_NAME "Open3D Engine")
set(CPACK_PACKAGE_VENDOR "O3DE Binary Project a Series of LF Projects, LLC")
set(CPACK_PACKAGE_VERSION "${LY_VERSION_STRING}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")

string(TOLOWER "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}" CPACK_PACKAGE_FILE_NAME)

set(DEFAULT_LICENSE_NAME "Apache-2.0")
set(DEFAULT_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

set(CPACK_RESOURCE_FILE_LICENSE ${DEFAULT_LICENSE_FILE})
set(CPACK_LICENSE_URL ${LY_INSTALLER_LICENSE_URL})

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}/${CPACK_PACKAGE_VERSION}")

# neither of the SOURCE_DIR variables equate to anything during execution of pre/post build scripts
set(CPACK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CPACK_BINARY_DIR ${CMAKE_BINARY_DIR}/_CPack) # to match other CPack out dirs

# this config file allows the dynamic setting of cpack variables at cpack-time instead of cmake configure
set(CPACK_PROJECT_CONFIG_FILE ${CPACK_SOURCE_DIR}/PackagingConfig.cmake)
set(CPACK_AUTO_GEN_TAG ${LY_INSTALLER_AUTO_GEN_TAG})

# attempt to apply platform specific settings
ly_get_absolute_pal_filename(pal_dir ${CPACK_SOURCE_DIR}/Platform/${PAL_HOST_PLATFORM_NAME})
include(${pal_dir}/Packaging_${PAL_HOST_PLATFORM_NAME_LOWERCASE}.cmake)

# if we get here and the generator hasn't been set, then a non fatal error occurred disabling packaging support
if(NOT CPACK_GENERATOR)
    return()
endif()

if(${CPACK_DESIRED_CMAKE_VERSION} VERSION_LESS ${CMAKE_MINIMUM_REQUIRED_VERSION})
    message(FATAL_ERROR
        "The desired version of CMake to be included in the package is "
        "below the minimum required version of CMake to run")
endif()

# pull down the desired copy of CMake so it can be included in the package
if(NOT (CPACK_CMAKE_PACKAGE_FILE AND CPACK_CMAKE_PACKAGE_HASH))
    message(FATAL_ERROR
        "Packaging is missing one or more following properties required to include CMake: "
        " CPACK_CMAKE_PACKAGE_FILE, CPACK_CMAKE_PACKAGE_HASH")
endif()

set(_cmake_package_dest ${CPACK_BINARY_DIR}/${CPACK_CMAKE_PACKAGE_FILE})

if(EXISTS ${_cmake_package_dest})
    file(SHA256 ${_cmake_package_dest} hash_of_downloaded_file)
    if (NOT "${hash_of_downloaded_file}" STREQUAL "${CPACK_CMAKE_PACKAGE_HASH}")
        message(STATUS "CMake ${CPACK_DESIRED_CMAKE_VERSION} found at ${_cmake_package_dest} but expected hash missmatches, re-downloading...")
        file(REMOVE ${_cmake_package_dest})
    else()
        message(STATUS "CMake ${CPACK_DESIRED_CMAKE_VERSION} found")
    endif()
endif()
if(NOT EXISTS ${_cmake_package_dest})
    # download it
    string(REPLACE "." ";" _version_componets "${CPACK_DESIRED_CMAKE_VERSION}")
    list(GET _version_componets 0 _major_version)
    list(GET _version_componets 1 _minor_version)

    set(_url_version_tag "v${_major_version}.${_minor_version}")
    set(_package_url "https://cmake.org/files/${_url_version_tag}/${CPACK_CMAKE_PACKAGE_FILE}")

    message(STATUS "Downloading CMake ${CPACK_DESIRED_CMAKE_VERSION} for packaging...")
    download_file(
        URL ${_package_url}
        TARGET_FILE ${_cmake_package_dest}
        EXPECTED_HASH ${CPACK_CMAKE_PACKAGE_HASH}
        RESULTS _results
    )
    list(GET _results 0 _status_code)

    if (${_status_code} EQUAL 0 AND EXISTS ${_cmake_package_dest})
        message(STATUS "CMake ${CPACK_DESIRED_CMAKE_VERSION} found")
    else()
        file(REMOVE ${_cmake_package_dest})
        list(REMOVE_AT _results 0)

        set(_error_message "An error occurred, code ${_status_code}.  URL ${_package_url} - ${_results}")

        if(${_status_code} EQUAL 1)
            string(APPEND _error_message
                "  Please double check the CPACK_CMAKE_PACKAGE_FILE and "
                "CPACK_CMAKE_PACKAGE_HASH properties before trying again.")
        endif()

        message(FATAL_ERROR ${_error_message})
    endif()
endif()

install(FILES ${_cmake_package_dest}
    DESTINATION ./Tools/Redistributables/CMake
)

# the version string and git tags are intended to be synchronized so it should be safe to use that instead
# of directly calling into git which could get messy in certain scenarios
if(${CPACK_PACKAGE_VERSION} VERSION_GREATER "0.0.0.0")
    set(_3rd_party_license_filename NOTICES.txt)

    set(_3rd_party_license_url "https://raw.githubusercontent.com/o3de/3p-package-source/${CPACK_PACKAGE_VERSION}/${_3rd_party_license_filename}")
    set(_3rd_party_license_dest ${CPACK_BINARY_DIR}/${_3rd_party_license_filename})

    # use the plain file downloader as we don't have the file hash available and using a dummy will
    # delete the file once it fails hash verification
    file(DOWNLOAD
        ${_3rd_party_license_url}
        ${_3rd_party_license_dest}
        STATUS _status
        TLS_VERIFY ON
    )
    list(POP_FRONT _status _status_code)

    if (${_status_code} EQUAL 0 AND EXISTS ${_3rd_party_license_dest})
        install(FILES ${_3rd_party_license_dest}
            DESTINATION .
        )
    else()
        file(REMOVE ${_3rd_party_license_dest})
        message(FATAL_ERROR "Failed to acquire the 3rd Party license manifest file at ${_3rd_party_license_url}.  Error: ${_status}")
    endif()
endif()

# checks for and removes trailing slash
function(strip_trailing_slash in_url out_url)
    string(LENGTH ${in_url} _url_length)
    MATH(EXPR _url_length "${_url_length}-1")

    string(SUBSTRING ${in_url} 0 ${_url_length} _clean_url)
    if("${in_url}" STREQUAL "${_clean_url}/")
        set(${out_url} ${_clean_url} PARENT_SCOPE)
    else()
        set(${out_url} ${in_url} PARENT_SCOPE)
    endif()
endfunction()

if(NOT LY_INSTALLER_UPLOAD_URL AND DEFINED ENV{LY_INSTALLER_UPLOAD_URL})
    set(LY_INSTALLER_UPLOAD_URL $ENV{LY_INSTALLER_UPLOAD_URL})
endif()

if(LY_INSTALLER_UPLOAD_URL)
    ly_is_s3_url(${LY_INSTALLER_UPLOAD_URL} _is_s3_bucket)
    if(NOT _is_s3_bucket)
        message(FATAL_ERROR "Only S3 installer uploading is supported at this time")
    endif()

    if (LY_INSTALLER_AWS_PROFILE)
        set(CPACK_AWS_PROFILE ${LY_INSTALLER_AWS_PROFILE})
    elseif (DEFINED ENV{LY_INSTALLER_AWS_PROFILE})
        set(CPACK_AWS_PROFILE $ENV{LY_INSTALLER_AWS_PROFILE})
    endif()

    strip_trailing_slash(${LY_INSTALLER_UPLOAD_URL} LY_INSTALLER_UPLOAD_URL)
    set(CPACK_UPLOAD_URL ${LY_INSTALLER_UPLOAD_URL})
endif()

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
    ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME} REQUIRED
    DISPLAY_NAME "${PROJECT_NAME} Core"
    DESCRIPTION "${PROJECT_NAME} Headers, Libraries and Tools"
)

if(LY_INSTALLER_DOWNLOAD_URL)
    strip_trailing_slash(${LY_INSTALLER_DOWNLOAD_URL} LY_INSTALLER_DOWNLOAD_URL)

    # this will set the following variables: CPACK_DOWNLOAD_SITE, CPACK_DOWNLOAD_ALL, and CPACK_UPLOAD_DIRECTORY (local)
    cpack_configure_downloads(
        ${LY_INSTALLER_DOWNLOAD_URL}
        UPLOAD_DIRECTORY ${CMAKE_BINARY_DIR}/_CPack_Uploads # to match the _CPack_Packages directory
        ALL
    )
endif()
