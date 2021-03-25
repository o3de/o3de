# {BEGIN_LICENSE}
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
# {END_LICENSE}

if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.19)
    project(${Name}
        LANGUAGES C CXX
        VERSION 1.0.0.0
    )
    include(EngineFinder.cmake OPTIONAL)
    find_package(o3de REQUIRED)
    o3de_initialize()
else()
    # Currently we are in the ${Name} folder: ${CMAKE_CURRENT_LIST_DIR}
    # Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
    # Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
    #       in which case it will see if that platform is present here or in the restricted folder.
    #       i.e. It could here: ${Name}/Platform/<platorm_name>  or
    #            <restricted_folder>/<platform_name>/${Name}
    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

    # Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
    # project cmake for this platform.
    include(${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_project.cmake)

    add_subdirectory(Code)
endif()
