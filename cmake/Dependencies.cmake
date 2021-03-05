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

include(FindPackageHandleStandardArgs)

#! ly_add_dependencies: wrapper to add_dependencies, however, this wrapper collaborates with
#  LYWrappers.cmake to delay adding dependencies if the target is not declared yet.
#
# This wrapper will add the dependency to the indicated target immediately if the target was
# already added. If the target was not added at the moment this function is called, the dependency
# will be stored in a global variable which ly_add_target will access and create those dependencies
# after the target is added.
#
# \arg:TARGET name of the target
# \arg:DEPENDENCIES dependencies to add to TARGET
#
function(ly_add_dependencies TARGET)

    if(NOT TARGET)
        message(FATAL_ERROR "You must provide a target")
    endif()
    set (extra_function_args ${ARGN})
    if(num_extra_args)
        message(FATAL_ERROR "You must provide at least a dependency")
    endif()

    if(${TARGET} IN_LIST extra_function_args)
        message(FATAL_ERROR "Cyclic dependency detected ${TARGET} depends on ${extra_function_args}")
    endif()
    if(TARGET ${TARGET})
        # Target already created, add it
        ly_parse_third_party_dependencies("${extra_function_args}")
        add_dependencies(${TARGET} ${extra_function_args})
    else()
        set_property(GLOBAL APPEND PROPERTY LY_DELAYED_DEPENDENCIES_${TARGET} ${extra_function_args})
    endif()

endfunction()
