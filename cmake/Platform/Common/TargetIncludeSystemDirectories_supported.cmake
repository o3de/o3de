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

# ly_target_include_system_directories: adds a system include to the target.
# This allows for platform-specific handling of how system includes are added
# to the target. This common implementation just calls
#
#     target_include_directories(<TARGET> SYSTEM <ARGS>)
#
# \arg:TARGET name of the target
# All other unrecognized arguments are passed unchanged to
# target_include_directories
#
function(ly_target_include_system_directories)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs)

    cmake_parse_arguments(ly_target_include_system_directories "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ly_target_include_system_directories_TARGET)
        message(FATAL_ERROR "Target not provided")
    endif()

    target_compile_options(${ly_target_include_system_directories_TARGET}
        INTERFACE
            ${LY_CXX_SYSTEM_INCLUDE_CONFIGURATION_FLAG}
    )

    target_include_directories(${ly_target_include_system_directories_TARGET} SYSTEM ${ly_target_include_system_directories_UNPARSED_ARGUMENTS})

endfunction()
