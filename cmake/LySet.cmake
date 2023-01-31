#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_directory_property(LY_PARENT_SCOPE PARENT_DIRECTORY)
if(LY_PARENT_SCOPE)
    set(LY_PARENT_SCOPE ${LY_PARENT_SCOPE} PARENT_SCOPE)
endif()

#! ly_set: convenient function to set and export variable to the parent scope in scenarios
#          where CMAKE_SOURCE_DIR != CMAKE_CURRENT_LIST_DIR (e.g. when the engine's cmake 
#          files are included from a project)
#
macro(ly_set name)
    set(${name} "${ARGN}")
    if(LY_PARENT_SCOPE)
        set(${name} "${ARGN}" PARENT_SCOPE)
    endif()
endmacro()

#! o3de_set_from_env_with_default: convenient function to set a variable
# from an environment variable or a default if the environment var is empty
# and then run CONFIGURE on the result to replace all @sign variable references
#
# Example usage:
# set(default "example")
# o3de_set_from_env_with_default(var ENVVAR "@default@" CACHE STRING "Example string")
# message(INFO "Result is ${var}")
# Prints "Result is example" if no environment var named "ENVVAR" is set or is empty
# 
# \arg:name - name of output variable to set 
# \arg:env_name - name of environment variable to use 
# \argn - remaining args are passed to the set() command and should at least contain the value 
macro(o3de_set_from_env_with_default name env_name)
    set(${name} ${ARGN})
    if(NOT "$ENV{${env_name}}" STREQUAL "")
        set(${name} "$ENV{${env_name}}")
    endif()
    string(CONFIGURE ${${name}} ${name} @ONLY)
endmacro()
