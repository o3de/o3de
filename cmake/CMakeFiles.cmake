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

if(NOT INSTALLED_ENGINE)
    # Add all cmake files in a project so they can be handled from within the IDE
    ly_include_cmake_file_list(cmake/cmake_files.cmake)
    add_custom_target(CMakeFiles SOURCES ${ALLFILES})
    ly_source_groups_from_folders("${ALLFILES}")
    unset(ALLFILES)
endif()