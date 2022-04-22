#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Use system default OpenSSL library instead of maintaining an O3DE version for Linux
find_package(OpenSSL)
if (NOT OpenSSL_FOUND)
   message(FATAL_ERROR "Compiling on linux requires the development headers for OpenSSL.  Try using your package manager to install the OpenSSL development libraries following https://wiki.openssl.org/index.php/Libssl_API")
else()
    # OpenSSL targets should be considered as provided by the system
    set_target_properties(OpenSSL::SSL OpenSSL::Crypto PROPERTIES LY_SYSTEM_LIBRARY TRUE)

    # Alias the O3DE name to the official name
    add_library(3rdParty::OpenSSL ALIAS OpenSSL::SSL)
endif()

# include Install.cmake to get access to the ly_install function
include(cmake/Install.cmake)

# Copies over the OpenSSL_linux.cmake to the same location in the SDK layout.
cmake_path(RELATIVE_PATH CMAKE_CURRENT_LIST_DIR BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE openssl_cmake_rel_directory)
ly_install(FILES "${CMAKE_CURRENT_LIST_FILE}"
    DESTINATION "${openssl_cmake_rel_directory}"
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)
