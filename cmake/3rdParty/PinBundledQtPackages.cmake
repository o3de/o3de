#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Prevent Qt's nested find_package(Qt6*Tools) calls from mixing the O3DE Qt
# package with another Qt installation. Unlike a CMAKE_PREFIX_PATH hint,
# <Package>_DIR variables take precedence over the standard package searches,
# including stale entries in CMakeCache.txt. Keep the overrides local to the
# caller so they do not overwrite the user's CMake cache settings.
function(o3de_pin_bundled_qt_package_configs package_folder)
    set(qt_cmake_dir "${package_folder}/qt/lib/cmake")
    if(NOT EXISTS "${qt_cmake_dir}/Qt6/Qt6Config.cmake")
        return()
    endif()

    file(GLOB qt_package_dirs LIST_DIRECTORIES true "${qt_cmake_dir}/Qt6*")
    foreach(qt_package_dir IN LISTS qt_package_dirs)
        if(NOT IS_DIRECTORY "${qt_package_dir}")
            continue()
        endif()
        get_filename_component(qt_package_name "${qt_package_dir}" NAME)
        if(EXISTS "${qt_package_dir}/${qt_package_name}Config.cmake")
            set("${qt_package_name}_DIR" "${qt_package_dir}" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()
