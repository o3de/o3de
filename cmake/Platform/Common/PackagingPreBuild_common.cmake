# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

message(STATUS "Executing packaging prebuild...")
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

# We download it to a different location because CMAKE_INSTALL_PREFIX will be removed during
# cpack generation. CPACK_BINARY_DIR persists across cpack invocations
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

cmake_path(APPEND install_prefix $ENV{DESTDIR} ${CMAKE_INSTALL_PREFIX})
file(INSTALL FILES ${_cmake_package_dest}
    DESTINATION "${install_prefix}/Tools/Redistributables/CMake"
    MESSAGE_NEVER
)
