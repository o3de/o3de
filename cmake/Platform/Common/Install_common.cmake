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

set(_default_component "com.o3de.default")

#! ly_install_target: registers the target to be installed by cmake install.
#
# \arg:NAME name of the target
# \arg:NAMESPACE namespace declaration for this target. It will be used for IDE and dependencies
# \arg:INCLUDE_DIRECTORIES paths to the include directories
# \arg:BUILD_DEPENDENCIES list of interfaces this target depends on (could be a compilation dependency
#                             if the dependency is only exposing an include path, or could be a linking
#                             dependency is exposing a lib)
# \arg:RUNTIME_DEPENDENCIES list of dependencies this target depends on at runtime
# \arg:COMPILE_DEFINITIONS list of compilation definitions this target will use to compile
function(ly_install_target ly_install_target_NAME)

    set(options)
    set(oneValueArgs NAMESPACE COMPONENT)
    set(multiValueArgs INCLUDE_DIRECTORIES BUILD_DEPENDENCIES RUNTIME_DEPENDENCIES COMPILE_DEFINITIONS)

    cmake_parse_arguments(ly_install_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # All include directories marked PUBLIC or INTERFACE will be installed
    set(include_location "include")
    get_target_property(include_directories ${ly_install_target_NAME} INTERFACE_INCLUDE_DIRECTORIES)

    if (include_directories)
        set_target_properties(${ly_install_target_NAME} PROPERTIES PUBLIC_HEADER "${include_directories}")
        # The include directories are specified relative to the CMakeLists.txt file that adds the target.
        # We need to install the includes relative to our source tree root because that's where INSTALL_INTERFACE
        # will point CMake when it looks for headers
        file(RELATIVE_PATH relative_path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
        string(APPEND include_location "/${relative_path}")
    endif()

    ly_generate_target_find_file(
        NAME ${ly_install_target_NAME}
        ${ARGN}
    )

    install(
        TARGETS ${ly_install_target_NAME}
        EXPORT ${ly_install_target_NAME}Targets
        LIBRARY
            DESTINATION lib/$<CONFIG>
            COMPONENT ${ly_install_target_COMPONENT}
        ARCHIVE
            DESTINATION lib/$<CONFIG>
            COMPONENT ${ly_install_target_COMPONENT}
        RUNTIME
            DESTINATION bin/$<CONFIG>
            COMPONENT ${ly_install_target_COMPONENT}
        PUBLIC_HEADER
            DESTINATION ${include_location}
            COMPONENT ${ly_install_target_COMPONENT}
    )

    install(EXPORT ${ly_install_target_NAME}Targets
        DESTINATION cmake_autogen/${ly_install_target_NAME}
        COMPONENT ${ly_install_target_COMPONENT}
    )

    # Header only targets(i.e., INTERFACE) don't have outputs
    get_target_property(target_type ${ly_install_target_NAME} TYPE)
    if(NOT ${target_type} STREQUAL "INTERFACE_LIBRARY")
        ly_generate_target_config_file(${ly_install_target_NAME})

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${ly_install_target_NAME}_$<CONFIG>.cmake"
            DESTINATION cmake_autogen/${ly_install_target_NAME}
            COMPONENT ${ly_install_target_COMPONENT}
        )
    endif()

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Find${ly_install_target_NAME}.cmake"
        DESTINATION .
        COMPONENT ${ly_install_target_COMPONENT}
    )

endfunction()


#! ly_generate_target_find_file: generates the Find${target}.cmake file which is used when importing installed packages.
#
# \arg:NAME name of the target
# \arg:INCLUDE_DIRECTORIES paths to the include directories
# \arg:NAMESPACE namespace declaration for this target. It will be used for IDE and dependencies
# \arg:BUILD_DEPENDENCIES list of interfaces this target depends on (could be a compilation dependency
#                             if the dependency is only exposing an include path, or could be a linking
#                             dependency is exposing a lib)
# \arg:RUNTIME_DEPENDENCIES list of dependencies this target depends on at runtime
# \arg:COMPILE_DEFINITIONS list of compilation definitions this target will use to compile
function(ly_generate_target_find_file)

    set(options)
    set(oneValueArgs NAME NAMESPACE)
    set(multiValueArgs COMPILE_DEFINITIONS BUILD_DEPENDENCIES RUNTIME_DEPENDENCIES INCLUDE_DIRECTORIES)
    cmake_parse_arguments(ly_generate_target_find_file "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # These targets will be imported. So we strip PRIVATE properties.
    # We can only set INTERFACE properties on imported targets
    unset(build_dependencies_interface_props)
    unset(compile_definitions_interface_props)
    unset(include_directories_interface_props)
    unset(installed_include_directories_interface_props)
    ly_strip_non_interface_properties(build_dependencies_interface_props ${ly_generate_target_find_file_BUILD_DEPENDENCIES})
    ly_strip_non_interface_properties(compile_definitions_interface_props ${ly_generate_target_find_file_COMPILE_DEFINITIONS})
    ly_strip_non_interface_properties(include_directories_interface_props ${ly_generate_target_find_file_INCLUDE_DIRECTORIES})

    set(NAME_PLACEHOLDER ${ly_generate_target_find_file_NAME})

    # Includes need additional processing to add the install root
    foreach(include ${include_directories_interface_props})
        set(installed_include_prefix "\${LY_ROOT_FOLDER}/include/")
        file(RELATIVE_PATH relative_path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${include})
        string(APPEND installed_include_prefix ${relative_path})
        list(APPEND installed_include_directories_interface_props ${installed_include_prefix})
    endforeach()

    if(ly_generate_target_find_file_NAMESPACE)
        set(NAMESPACE_PLACEHOLDER "NAMESPACE ${ly_generate_target_find_file_NAMESPACE}")
    endif()
    if(installed_include_directories_interface_props)
        string(REPLACE ";" "\n" include_dirs "${installed_include_directories_interface_props}")
        set(INCLUDE_DIRECTORIES_PLACEHOLDER "INCLUDE_DIRECTORIES\nINTERFACE\n${include_dirs}")
    endif()
    if(build_dependencies_interface_props)
        string(REPLACE ";" "\n" build_deps "${build_dependencies_interface_props}")
        set(BUILD_DEPENDENCIES_PLACEHOLDER "BUILD_DEPENDENCIES\nINTERFACE\n${build_deps}")
    endif()
    if(ly_generate_target_find_file_RUNTIME_DEPENDENCIES)
        string(REPLACE ";" "\n" runtime_deps "${ly_generate_target_find_file_RUNTIME_DEPENDENCIES}")
        set(RUNTIME_DEPENDENCIES_PLACEHOLDER "RUNTIME_DEPENDENCIES\n${runtime_deps}")
    endif()
    if(compile_definitions_interface_props)
        string(REPLACE ";" "\n" compile_defs "${compile_definitions_interface_props}")
        set(COMPILE_DEFINITIONS_PLACEHOLDER "COMPILE_DEFINITIONS\nINTERFACE\n${compile_defs}")
    endif()

    string(REPLACE ";" " " ALL_CONFIGS "${CMAKE_CONFIGURATION_TYPES}")

    set(target_config_found_vars "")
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        string(APPEND target_config_found_vars "\n${ly_generate_target_find_file_NAME}_${config}_FOUND")
    endforeach()
    set(TARGET_CONFIG_FOUND_VARS_PLACEHOLDER "${target_config_found_vars}")

    # Interface libs aren't built so they don't generate a library. These are our HEADER_ONLY targets.
    get_target_property(target_type ${ly_generate_target_find_file_NAME} TYPE)
    if(NOT ${target_type} STREQUAL "INTERFACE_LIBRARY")
        set(HEADER_ONLY_PLACEHOLDER FALSE)
    else()
        set(HEADER_ONLY_PLACEHOLDER TRUE)
    endif()

    configure_file(${LY_ROOT_FOLDER}/cmake/FindTargetTemplate.cmake ${CMAKE_CURRENT_BINARY_DIR}/Find${ly_generate_target_find_file_NAME}.cmake @ONLY)

endfunction()


#! ly_generate_target_config_file: generates the ${target}_$<CONFIG>.cmake files for a target
#
# The generated file will set the location of the target binary per configuration
# These per config files will be included by the target's find file to set the location of the binary/
# \arg:NAME name of the target
function(ly_generate_target_config_file NAME)

    # SHARED_LIBRARY is omitted from this list because we link to the implib on Windows
    set(BINARY_DIR_OUTPUTS EXECUTABLE APPLICATION)
    set(target_file_contents "")
    if(${target_type} IN_LIST BINARY_DIR_OUTPUTS)
        set(out_file_generator TARGET_FILE_NAME)
        set(out_dir bin)
    else()
        set(out_file_generator TARGET_LINKER_FILE_NAME)
        set(out_dir lib)
    endif()

    string(APPEND target_file_contents "
# Generated by O3DE

set_target_properties(${NAME} PROPERTIES IMPORTED_LOCATION \"\${LY_ROOT_FOLDER}/${out_dir}/$<CONFIG>/$<${out_file_generator}:${NAME}>\")

if(EXISTS \"\${LY_ROOT_FOLDER}/${out_dir}/$<CONFIG>/$<${out_file_generator}:${NAME}>\")
    set(${NAME}_$<CONFIG>_FOUND TRUE)
else()
    set(${NAME}_$<CONFIG>_FOUND FALSE)
endif()")

    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_$<CONFIG>.cmake" CONTENT ${target_file_contents})

endfunction()


#! ly_strip_non_interface_properties: strips private properties since we're exporting an interface target
#
# \arg:INTERFACE_PROPERTIES list of interface properties to be returned
function(ly_strip_non_interface_properties INTERFACE_PROPERTIES)
    set(reserved_keywords PRIVATE PUBLIC INTERFACE)
    unset(last_keyword)
    unset(stripped_props)
    foreach(prop ${ARGN})
        if(${prop} IN_LIST reserved_keywords)
            set(last_keyword ${prop})
        else()
            if (NOT last_keyword STREQUAL "PRIVATE")
                list(APPEND stripped_props ${prop})
            endif()
        endif()
    endforeach()

    set(${INTERFACE_PROPERTIES} ${stripped_props} PARENT_SCOPE)
endfunction()


#! ly_setup_o3de_install: generates the Findo3de.cmake file and setup install locations for scripts, tools, assets etc.,
function(ly_setup_o3de_install)

    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    unset(find_package_list)
    foreach(target IN LISTS all_targets)
        list(APPEND find_package_list "find_package(${target})")
    endforeach()

    string(REPLACE ";" "\n" FIND_PACKAGES_PLACEHOLDER "${find_package_list}")

    configure_file(${LY_ROOT_FOLDER}/cmake/Findo3deTemplate.cmake ${CMAKE_CURRENT_BINARY_DIR}/Findo3de.cmake @ONLY)

    ly_install_launcher_target_generator()

    ly_install_o3de_directories()

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Findo3de.cmake"
        DESTINATION cmake
        COMPONENT ${_default_component}
    )

    install(FILES "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
        DESTINATION .
        COMPONENT ${_default_component}
    )

endfunction()


#! ly_install_o3de_directories: install directories required by the engine
function(ly_install_o3de_directories)

    # List of directories we want to install relative to engine root
    set(DIRECTORIES_TO_INSTALL Tools/LyTestTools Tools/RemoteConsole ctest_scripts scripts)
    foreach(dir ${DIRECTORIES_TO_INSTALL})

        get_filename_component(install_path ${dir} DIRECTORY)
        if (NOT install_path)
            set(install_path .)
        endif()

        install(DIRECTORY "${CMAKE_SOURCE_DIR}/${dir}"
            DESTINATION ${install_path}
            COMPONENT ${_default_component}
        )

    endforeach()

    # Directories which have excludes
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/cmake"
        DESTINATION .
        COMPONENT ${_default_component}
        REGEX "Findo3de.cmake" EXCLUDE
    )

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"
        DESTINATION .
        COMPONENT ${_default_component}
        REGEX "downloaded_packages" EXCLUDE
        REGEX "runtime" EXCLUDE
    )

endfunction()


#! ly_install_launcher_target_generator: install source files needed for project launcher generation
function(ly_install_launcher_target_generator)

    install(FILES
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/launcher_generator.cmake
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/launcher_project_files.cmake
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/LauncherProject.cpp
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/StaticModules.in
        DESTINATION LauncherGenerator
        COMPONENT ${_default_component}
    )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/Platform
        DESTINATION LauncherGenerator
        COMPONENT ${_default_component}
    )
    install(FILES ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/FindLauncherGenerator.cmake
        DESTINATION cmake
        COMPONENT ${_default_component}
    )

endfunction()