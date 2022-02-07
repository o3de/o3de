#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT INSTALLED_ENGINE)
    # Add all cmake files in a project so they can be handled from within the IDE
    ly_include_cmake_file_list(cmake/cmake_files.cmake)
    add_custom_target(CMakeFiles SOURCES ${ALLFILES})
    ly_source_groups_from_folders("${ALLFILES}")
    unset(ALLFILES)
endif()