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

file(READ "${CMAKE_CURRENT_LIST_DIR}/project.json" project_json)

string(JSON project_target_name ERROR_VARIABLE json_error GET ${project_json} "project_name")
if(${json_error})
    message(FATAL_ERROR "Unable to read key 'project_name' from 'project.json'")
endif()

set_property(GLOBAL APPEND PROPERTY LY_PROJECTS_TARGET_NAME ${project_target_name})
add_subdirectory(Gem)
