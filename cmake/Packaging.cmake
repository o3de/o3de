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

set(CPACK_SNAP_DISTRO "" CACHE STRING
  "Sets the base snap OS distro (core20, 22, etc) for the snap build"
)

set(CPACK_THREADS 0)
set(CPACK_DESIRED_CMAKE_VERSION 3.22.0)
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
# prefer the display engine version if available.
# during development, the display version will be "00.00" or "" in which case we want
# to use the actual engine version  
if(NOT ((${O3DE_INSTALL_DISPLAY_VERSION_STRING} STREQUAL "00.00") OR (${O3DE_INSTALL_DISPLAY_VERSION_STRING} STREQUAL "")))
    set(CPACK_PACKAGE_VERSION "${O3DE_INSTALL_DISPLAY_VERSION_STRING}")
else()
    set(CPACK_PACKAGE_VERSION "${O3DE_INSTALL_VERSION_STRING}")
endif()
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")

string(TOLOWER "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}" CPACK_PACKAGE_FILE_NAME)

set(DEFAULT_LICENSE_NAME "Apache-2.0")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
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
o3de_pal_dir(pal_dir ${CPACK_SOURCE_DIR}/Platform/${PAL_HOST_PLATFORM_NAME} "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")
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

# Scan the source and 3p packages for licenses, then add the generated license results to the binary output folder.
# These results will be installed to the root of the install folder and copied to the S3 bucket specific to each platform
set(CPACK_3P_LICENSE_FILE "${CPACK_BINARY_DIR}/NOTICES.txt")
set(CPACK_3P_MANIFEST_FILE "${CPACK_BINARY_DIR}/SPDX-License.json")

configure_file(${LY_ROOT_FOLDER}/cmake/Packaging/LicenseScan.cmake.in
    ${CPACK_BINARY_DIR}/LicenseScan.cmake
    @ONLY
)
ly_install(SCRIPT ${CPACK_BINARY_DIR}/LicenseScan.cmake
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)
ly_install(FILES ${CPACK_3P_LICENSE_FILE} ${CPACK_3P_MANIFEST_FILE}
    DESTINATION .
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)

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

# Set common CPACK variables to all platforms/generators
set(CPACK_STRIP_FILES TRUE) # always strip symbols on packaging
set(CPACK_PACKAGE_CHECKSUM SHA256) # Generate checksum file
set(CPACK_PRE_BUILD_SCRIPTS ${pal_dir}/PackagingPreBuild_${PAL_HOST_PLATFORM_NAME_LOWERCASE}.cmake)
set(CPACK_POST_BUILD_SCRIPTS ${pal_dir}/PackagingPostBuild_${PAL_HOST_PLATFORM_NAME_LOWERCASE}.cmake)
set(CPACK_CODESIGN_SCRIPT ${pal_dir}/PackagingCodeSign_${PAL_HOST_PLATFORM_NAME_LOWERCASE}.cmake)
set(CPACK_LY_PYTHON_CMD ${LY_PYTHON_CMD})

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
