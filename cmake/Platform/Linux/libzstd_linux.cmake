#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Use system default zstd library instead of maintaining an O3DE version for Linux

find_package(PkgConfig REQUIRED)
# ask pkg-config to find the libzstd library and prepare an imported target
pkg_check_modules(zstd IMPORTED_TARGET libzstd)

if (NOT TARGET PkgConfig::zstd)
    message(FATAL_ERROR "Compiling on linux requires the zstd development libraries and headers as well as pkg-config.  Try using your package manager to install the libzstd-dev libraries.")
else()
   add_library(3rdParty::zstd ALIAS PkgConfig::zstd)
   set_target_properties(PkgConfig::zstd
                         PROPERTIES
                             LY_SYSTEM_LIBRARY TRUE)

   # include Install.cmake to get access to the ly_install function
   include(cmake/Install.cmake)

   # Copies over the libzstd_linux.cmake to the same location in the SDK layout.
   cmake_path(RELATIVE_PATH CMAKE_CURRENT_LIST_DIR BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE libzstd_linux_cmake_rel_directory)
   ly_install(FILES "${CMAKE_CURRENT_LIST_FILE}"
       DESTINATION "${libzstd_linux_cmake_rel_directory}"
       COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
   )

endif()

