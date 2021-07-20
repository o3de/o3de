#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! ly_target_include_system_directories: adds a system include to the target. This is the same behavior as target_include_directories
#  passing the SYSTEM parameter. However, this supports doing it in generators that CMake still does not.
#
# To handle platforms that yet not support SYSTEM includes, we pass the includes as part of compilation options
# Once all platforms support it, we can roll back to target_include_directories
# We are using the same variable for the system include that CMake does, so this works in all platforms where cmake supports
# it and by defining the variables, it will support it in platforms where the compiler supports it but CMake still didnt
# add it to the generator (e.g. MSVC: https://gitlab.kitware.com/cmake/cmake/issues/17904)
#
# \arg:TARGET name of the target
# \arg:INTERFACE includes to apply to INTERFACE
# \arg:PUBLIC includes to apply to PUBLIC
# \arg:PRIVATE includes to apply to PRIVATE
#
function(ly_target_include_system_directories)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs INTERFACE PUBLIC PRIVATE)

    cmake_parse_arguments(ly_target_include_system_directories "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ly_target_include_system_directories_TARGET)
        message(FATAL_ERROR "Target not provided")
    endif()

    if(LY_CXX_SYSTEM_INCLUDE_CONFIGURATION_FLAG)
        target_compile_options(${ly_target_include_system_directories_TARGET}
            INTERFACE
                ${LY_CXX_SYSTEM_INCLUDE_CONFIGURATION_FLAG}
        )
    endif()

    foreach(type ITEMS INTERFACE PUBLIC PRIVATE)
        foreach(include_dir ${ly_target_include_system_directories_${type}})
            string(GENEX_STRIP ${include_dir} include_genex_expr)
            # Skip over any include directory value which contains a generator expression
            # It's path cannot be validated at configure
            # The check validates that the string stripped from generation expressions is the same as the original string
            if(include_genex_expr STREQUAL include_dir AND NOT EXISTS ${include_dir})
                message(FATAL_ERROR "Cannot find 3rdParty library ${ly_add_external_target_NAME} include path ${include_dir}")
            endif()
            target_compile_options(${ly_target_include_system_directories_TARGET}
                ${type} ${CMAKE_INCLUDE_SYSTEM_FLAG_CXX}${include_dir}
            )
            # For windows add the includes to the INTERFACE_SYSTEM_INCLUDE_DIRECTORIES directly so that the property
            # is available in dependent targets
            set_property(TARGET ${ly_target_include_system_directories_TARGET} APPEND
                PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${include_dir})
        endforeach()
    endforeach()

endfunction()
