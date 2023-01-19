#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Use system default unwind library instead of maintaining an O3DE version for Linux

find_package(PkgConfig REQUIRED)
# ask pkg-config to find the libunwind library and prepare an imported target
pkg_check_modules(libunwind IMPORTED_TARGET libunwind)
if (NOT TARGET PkgConfig::libunwind)
    message(FATAL_ERROR "Compiling on linux requires the unwind development libraries and headers as well as pkg-config.  Try using your package manager to install the libunwind-dev libraries.")
else()
    add_library(3rdParty::unwind ALIAS PkgConfig::libunwind)
    set_target_properties(PkgConfig::libunwind 
        PROPERTIES 
            LY_SYSTEM_LIBRARY TRUE)

    # include Install.cmake to get access to the ly_install function
    include(cmake/Install.cmake)

    # Copies over the libunwind_linux.cmake to the same location in the SDK layout.
    cmake_path(RELATIVE_PATH CMAKE_CURRENT_LIST_DIR BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE libunwind_linux_cmake_rel_directory)
    ly_install(FILES "${CMAKE_CURRENT_LIST_FILE}"
        DESTINATION "${libunwind_linux_cmake_rel_directory}"
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

endif()