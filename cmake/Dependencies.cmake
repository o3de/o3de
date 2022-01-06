#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
        # Dependencies can only be added on non-alias target
        ly_de_alias_target(${TARGET} de_aliased_target)
        add_dependencies(${de_aliased_target} ${extra_function_args})
    else()
        set_property(GLOBAL APPEND PROPERTY LY_DELAYED_DEPENDENCIES_${TARGET} ${extra_function_args})
    endif()

endfunction()
