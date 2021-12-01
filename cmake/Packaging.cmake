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

set(CPACK_UPLOAD_URL "" CACHE STRING
"URL used to upload the installer artifacts after generation, the host target and version number \
will automatically appended as '<version>/<host>'. If LY_INSTALLER_AUTO_GEN_TAG is set, the full URL \
format will be: <base_url>/<build_tag>/<host>. Currently only accepts S3 URLs e.g. s3://<bucket>/<prefix>"
)

set(CPACK_AWS_PROFILE "" CACHE STRING
"AWS CLI profile for uploading artifacts."
)

set(CPACK_THREADS 0)
set(CPACK_DESIRED_CMAKE_VERSION 3.20.2)
if(${CPACK_DESIRED_CMAKE_VERSION} VERSION_LESS ${CMAKE_MINIMUM_REQUIRED_VERSION})
    message(FATAL_ERROR
        "The desired version of CMake to be included in the package is "
        "below the minimum required version of CMake to run")
endif()

# set all common cpack variable overrides first so they can be accessible via configure_file
# when the platform specific settings are applied below.  additionally, any variable with
# the "CPACK_" prefix will automatically be cached for use in any phase of cpack namely
# pre/post build
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_FULL_NAME "Open3D Engine")
set(CPACK_PACKAGE_VENDOR "O3DE Binary Project a Series of LF Projects, LLC")
set(CPACK_PACKAGE_CONTACT "info@o3debinaries.org")
set(CPACK_PACKAGE_VERSION "${LY_VERSION_STRING}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")

string(TOLOWER "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}" CPACK_PACKAGE_FILE_NAME)

set(DEFAULT_LICENSE_NAME "Apache-2.0")
<<<<<<< HEAD
set(DEFAULT_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_LICENSE ${DEFAULT_LICENSE_FILE})
=======

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
>>>>>>> development
set(CPACK_LICENSE_URL ${LY_INSTALLER_LICENSE_URL})

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}/${CPACK_PACKAGE_VERSION}")

# neither of the SOURCE_DIR variables equate to anything during execution of pre/post build scripts
set(CPACK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CPACK_BINARY_DIR ${CMAKE_BINARY_DIR}/_CPack) # to match other CPack out dirs
set(CPACK_OUTPUT_FILE_PREFIX CPackUploads)

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

# We will download the desired copy of CMake so it can be included in the package, we defer the downloading
# to the install process, to do so we generate a script that will perform the download and execute such script
# during the install process (before packaging)
if(NOT (CPACK_CMAKE_PACKAGE_FILE AND CPACK_CMAKE_PACKAGE_HASH))
    message(FATAL_ERROR
        "Packaging is missing one or more following properties required to include CMake: "
        " CPACK_CMAKE_PACKAGE_FILE, CPACK_CMAKE_PACKAGE_HASH")
endif()

# We download it to a different location because CPACK_PACKAGING_INSTALL_PREFIX will be removed during
# cpack generation. CPACK_BINARY_DIR persists across cpack invocations
set(LY_CMAKE_PACKAGE_DOWNLOAD_PATH ${CPACK_BINARY_DIR}/${CPACK_CMAKE_PACKAGE_FILE})

configure_file(${LY_ROOT_FOLDER}/cmake/Packaging/CMakeDownload.cmake.in
    ${CPACK_BINARY_DIR}/CMakeDownload.cmake
    @ONLY
)
ly_install(SCRIPT ${CPACK_BINARY_DIR}/CMakeDownload.cmake
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)
ly_install(FILES ${LY_CMAKE_PACKAGE_DOWNLOAD_PATH}
    DESTINATION Tools/Redistributables/CMake
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)

<<<<<<< HEAD
# Do inital check on workspace path in the event that LY_3RDPARTY_PATH is misconfigured
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." _root_path)
if (EXISTS "${_root_path}/3rdParty/packages")
    set(CPACK_LY_3P_PACKAGE_DIRECTORY "${_root_path}/3rdParty/packages")
else()
    set(CPACK_LY_3P_PACKAGE_DIRECTORY ${LY_3RDPARTY_PATH}/packages)
endif()

# Scan the engine and 3rd Party folders for licenses
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/python/python.cmd" _python_cmd)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/scripts/license_scanner" _license_script_path)

set(_license_script "${_license_script_path}/license_scanner.py")
set(_license_config "${_license_script_path}/scanner_config.json")

set(_license_scan_path "${CMAKE_CURRENT_SOURCE_DIR}" "${CPACK_LY_3P_PACKAGE_DIRECTORY}")
set(CPACK_3P_LICENSE_FILE "${CPACK_BINARY_DIR}/NOTICES.txt")
set(CPACK_3P_MANIFEST_FILE "${CPACK_BINARY_DIR}/SPDX-License.json")

set(_license_command
    ${_python_cmd} -s
    -u ${_license_script}
    --config-file ${_license_config}
    --license-file-path ${CPACK_3P_LICENSE_FILE}
    --package-file-path ${CPACK_3P_MANIFEST_FILE}
)

message(STATUS "Scanning for license files in ${_license_scan_path}")
execute_process(
    COMMAND ${_license_command} --scan-path ${_license_scan_path}
    RESULT_VARIABLE _license_result
    ERROR_VARIABLE _license_errors
    OUTPUT_VARIABLE _license_output
    ECHO_OUTPUT_VARIABLE
)

if(NOT ${_license_result} EQUAL 0)
    message(FATAL_ERROR "An error occurred during license scan.  ${_license_errors}")
else()
    set(CPACK_RESOURCE_FILE_LICENSE ${CPACK_3P_LICENSE_FILE})
endif()

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
        ly_install(FILES ${_3rd_party_license_dest}
            DESTINATION .
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
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
=======
# Set common CPACK variables to all platforms/generators
set(CPACK_STRIP_FILES TRUE) # always strip symbols on packaging
set(CPACK_PACKAGE_CHECKSUM SHA256) # Generate checksum file
set(CPACK_PRE_BUILD_SCRIPTS ${pal_dir}/PackagingPreBuild_${PAL_HOST_PLATFORM_NAME_LOWERCASE}.cmake)
set(CPACK_POST_BUILD_SCRIPTS ${pal_dir}/PackagingPostBuild_${PAL_HOST_PLATFORM_NAME_LOWERCASE}.cmake)
set(CPACK_LY_PYTHON_CMD ${LY_PYTHON_CMD})
>>>>>>> development

# IMPORTANT: required to be included AFTER setting all property overrides
include(CPack REQUIRED)

# configure ALL components here
file(APPEND "${CPACK_OUTPUT_CONFIG_FILE}" "
set(CPACK_COMPONENTS_ALL ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
set(CPACK_COMPONENT_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}_DISPLAY_NAME \"Common files\")
set(CPACK_COMPONENT_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}_DESCRIPTION \"${PROJECT_NAME} Headers, scripts and common files\")
set(CPACK_COMPONENT_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}_REQUIRED TRUE)
set(CPACK_COMPONENT_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}_DISABLED FALSE)

include(CPackComponents.cmake)
")

# Generate a file (CPackComponents.config) that we will include that defines the components 
# for this build permutation. This way we can get components for other permutations being passed
# through LY_INSTALL_EXTERNAL_BUILD_DIRS
unset(cpack_components_contents)

set(required "FALSE")
set(disabled "FALSE")
if(${LY_INSTALL_PERMUTATION_COMPONENT} STREQUAL DEFAULT)
    set(required "TRUE")
else()
    set(disabled "TRUE")
endif()
string(APPEND cpack_components_contents "
list(APPEND CPACK_COMPONENTS_ALL ${LY_INSTALL_PERMUTATION_COMPONENT})
set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_DISPLAY_NAME \"${LY_BUILD_PERMUTATION} common files\")
set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_DESCRIPTION \"${PROJECT_NAME} scripts and common files for ${LY_BUILD_PERMUTATION}\")
set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_DEPENDS ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_REQUIRED ${required})
set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_DISABLED ${disabled})
")

foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${conf} UCONF)
    set(required "FALSE")
    set(disabled "FALSE")
    if(${conf} STREQUAL profile AND ${LY_INSTALL_PERMUTATION_COMPONENT} STREQUAL DEFAULT)
        set(required "TRUE")
    else()
        set(disabled "TRUE")
    endif()

    # Inject a check to not declare components that have not been built. We are using AzCore since that is a
    # common target that will always be build, in every permutation and configuration
    string(APPEND cpack_components_contents "
if(EXISTS \"${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${conf}/${CMAKE_STATIC_LIBRARY_PREFIX}AzCore${CMAKE_STATIC_LIBRARY_SUFFIX}\")
    list(APPEND CPACK_COMPONENTS_ALL ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF})
    set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}_DISPLAY_NAME \"Binaries for ${LY_BUILD_PERMUTATION} ${conf}\")
    set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}_DESCRIPTION \"${PROJECT_NAME} libraries and applications for ${LY_BUILD_PERMUTATION} ${conf}\")
    set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}_DEPENDS ${LY_INSTALL_PERMUTATION_COMPONENT})
    set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}_REQUIRED ${required})
    set(CPACK_COMPONENT_${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}_DISABLED ${disabled})
endif()
")
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/CPackComponents.cmake" ${cpack_components_contents})

# Inject other build directories 
foreach(external_dir ${LY_INSTALL_EXTERNAL_BUILD_DIRS})
    file(APPEND "${CPACK_OUTPUT_CONFIG_FILE}"
        "include(${external_dir}/CPackComponents.cmake)\n"
    )
endforeach()

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

if(LY_INSTALLER_DOWNLOAD_URL)
    strip_trailing_slash(${LY_INSTALLER_DOWNLOAD_URL} LY_INSTALLER_DOWNLOAD_URL)

    # this will set the following variables: CPACK_DOWNLOAD_SITE, CPACK_DOWNLOAD_ALL, and CPACK_UPLOAD_DIRECTORY (local)
    cpack_configure_downloads(
        ${LY_INSTALLER_DOWNLOAD_URL}
        UPLOAD_DIRECTORY ${CMAKE_BINARY_DIR}/CPackUploads # to match the _CPack_Packages directory
        ALL
    )
endif()
