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

set(CMAKE_INSTALL_MESSAGE NEVER) # Simplify messages to reduce output noise

#! ly_install_target: registers the target to be installed by cmake install.
#
# \arg:NAME name of the target
function(ly_install_target ly_install_target_NAME)

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
        LIBRARY DESTINATION lib/$<CONFIG>
        ARCHIVE DESTINATION lib/$<CONFIG>
        RUNTIME DESTINATION bin/$<CONFIG>
        PUBLIC_HEADER DESTINATION ${include_location}
    )

    ly_generate_target_config_file(${ly_install_target_NAME})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${ly_install_target_NAME}_$<CONFIG>.cmake"
        DESTINATION cmake_autogen/${ly_install_target_NAME}
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Find${ly_install_target_NAME}.cmake"
        DESTINATION cmake
    )

endfunction()


#! ly_generate_target_find_file: generates the Find${target}.cmake file which is used when importing installed packages.
#
# \arg:NAME name of the target
# \arg:NAMESPACE namespace declaration for this target. It will be used for IDE and dependencies
# \arg:INCLUDE_DIRECTORIES paths to the include directories
# \arg:BUILD_DEPENDENCIES list of interfaces this target depends on (could be a compilation dependency
#                             if the dependency is only exposing an include path, or could be a linking
#                             dependency is exposing a lib)
# \arg:RUNTIME_DEPENDENCIES list of dependencies this target depends on at runtime
# \arg:COMPILE_DEFINITIONS list of compilation definitions this target will use to compile
function(ly_generate_target_find_file)

    set(options)
    set(oneValueArgs NAME NAMESPACE)
    set(multiValueArgs INCLUDE_DIRECTORIES COMPILE_DEFINITIONS BUILD_DEPENDENCIES RUNTIME_DEPENDENCIES)
    cmake_parse_arguments(ly_generate_target_find_file "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(NAME_PLACEHOLDER ${ly_generate_target_find_file_NAME})
    unset(NAMESPACE_PLACEHOLDER)
    unset(COMPILE_DEFINITIONS_PLACEHOLDER)
    unset(include_directories_interface_props)
    unset(INCLUDE_DIRECTORIES_PLACEHOLDER)
    set(RUNTIME_DEPENDENCIES_PLACEHOLDER ${ly_generate_target_find_file_RUNTIME_DEPENDENCIES})

    # These targets will be imported. We will expose PUBLIC and INTERFACE properties as INTERFACE properties since
    # only INTERFACE properties can be exposed on imported targets
    ly_strip_private_properties(COMPILE_DEFINITIONS_PLACEHOLDER ${ly_generate_target_find_file_COMPILE_DEFINITIONS})
    ly_strip_private_properties(include_directories_interface_props ${ly_generate_target_find_file_INCLUDE_DIRECTORIES})
    ly_strip_private_properties(BUILD_DEPENDENCIES_PLACEHOLDER ${ly_generate_target_find_file_BUILD_DEPENDENCIES})

    if(ly_generate_target_find_file_NAMESPACE)
        set(NAMESPACE_PLACEHOLDER "NAMESPACE ${ly_generate_target_find_file_NAMESPACE}")
    endif()

    string(REPLACE ";" "\n" COMPILE_DEFINITIONS_PLACEHOLDER "${COMPILE_DEFINITIONS_PLACEHOLDER}")

    # Includes need additional processing to add the install root
    foreach(include ${include_directories_interface_props})
        set(installed_include_prefix "\${LY_ROOT_FOLDER}/include/")
        file(RELATIVE_PATH relative_path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${include})
        list(APPEND INCLUDE_DIRECTORIES_PLACEHOLDER "include/${relative_path}")
    endforeach()
    string(REPLACE ";" "\n" INCLUDE_DIRECTORIES_PLACEHOLDER "${INCLUDE_DIRECTORIES_PLACEHOLDER}")

    string(REPLACE ";" "\n" BUILD_DEPENDENCIES_PLACEHOLDER "${BUILD_DEPENDENCIES_PLACEHOLDER}")
    string(REPLACE ";" "\n" RUNTIME_DEPENDENCIES_PLACEHOLDER "${RUNTIME_DEPENDENCIES_PLACEHOLDER}")

    configure_file(${LY_ROOT_FOLDER}/cmake/FindTarget.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/Find${ly_generate_target_find_file_NAME}.cmake @ONLY)

endfunction()


#! ly_generate_target_config_file: generates the ${target}_$<CONFIG>.cmake files for a target
#
# The generated file will set the location of the target binary per configuration
# These per config files will be included by the target's find file to set the location of the binary/
# \arg:NAME name of the target
function(ly_generate_target_config_file NAME)

    get_target_property(target_type ${NAME} TYPE)

    unset(target_file_contents)
    if(NOT target_type STREQUAL INTERFACE_LIBRARY)

        set(BINARY_DIR_OUTPUTS EXECUTABLE APPLICATION)
        set(target_file_contents "")
        if(${target_type} IN_LIST BINARY_DIR_OUTPUTS)
            set(out_file_generator TARGET_FILE_NAME)
            set(out_dir bin)
        else()
            set(out_file_generator TARGET_LINKER_FILE_NAME)
            set(out_dir lib)
        endif()

        string(APPEND target_file_contents
"# Generated by O3DE install

set(target_location \"\${LY_ROOT_FOLDER}/${out_dir}/$<CONFIG>/$<${out_file_generator}:${NAME}>\")
set_target_properties(${NAME}
    PROPERTIES
        $<$<CONFIG:profile>:IMPORTED_LOCATION \"\${target_location}>\"
        IMPORTED_LOCATION_$<UPPER_CASE:$<CONFIG>> \"\${target_location}\"
)
if(EXISTS \"\${target_location}\")
    set(${NAME}_$<CONFIG>_FOUND TRUE)
else()
    set(${NAME}_$<CONFIG>_FOUND FALSE)
endif()
")
    endif()

    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_$<CONFIG>.cmake" CONTENT "${target_file_contents}")

endfunction()


#! ly_strip_private_properties: strips private properties since we're exporting an interface target
#
# \arg:INTERFACE_PROPERTIES list of interface properties to be returned
function(ly_strip_private_properties INTERFACE_PROPERTIES)
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


#! ly_setup_o3de_install: orchestrates the installation of the different parts. This is the entry point from the root CMakeLists.txt
function(ly_setup_o3de_install)

    ly_setup_cmake_install()
    ly_setup_target_generator()
    ly_setup_others()

endfunction()

#! ly_setup_cmake_install: install the "cmake" folder
function(ly_setup_cmake_install)

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/cmake"
        DESTINATION .
        REGEX "Findo3de.cmake" EXCLUDE
        REGEX "Platform\/.*\/BuiltInPackages_.*\.cmake" EXCLUDE
    )
    install(
        FILES
            "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
            "${CMAKE_SOURCE_DIR}/engine.json"
        DESTINATION .
    )

    # Collect all Find files that were added with ly_add_external_target_path
    unset(additional_find_files)
    get_property(additional_module_paths GLOBAL PROPERTY LY_ADDITIONAL_MODULE_PATH)
    foreach(additional_module_path ${additional_module_paths})
        unset(find_files)
        file(GLOB find_files "${additional_module_path}/Find*.cmake")
        list(APPEND additional_find_files "${find_files}")
    endforeach()
    install(FILES ${additional_find_files}
        DESTINATION cmake/3rdParty
    )

    # Findo3de.cmake file: we generate a different Findo3de.camke file than the one we have in cmake. This one is going to expose all
    # targets that are pre-built
    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    unset(FIND_PACKAGES_PLACEHOLDER)
    foreach(target IN LISTS all_targets)
        string(APPEND FIND_PACKAGES_PLACEHOLDER "    find_package(${target})\n")
    endforeach()

    configure_file(${LY_ROOT_FOLDER}/cmake/Findo3de.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/Findo3de.cmake @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/Findo3de.cmake"
        DESTINATION cmake
    )

    # BuiltInPackage_<platform>.cmake: since associations could happen in any cmake file across the engine. We collect
    # all the associations in ly_associate_package and then generate them into BuiltInPackages_<platform>.cmake. This
    # will consolidate all associations in one file
    get_property(all_package_names GLOBAL PROPERTY LY_PACKAGE_NAMES)
    set(builtinpackages "# Generated by O3DE install\n\n")
    foreach(package_name IN LISTS all_package_names)
        get_property(package_hash GLOBAL PROPERTY LY_PACKAGE_HASH_${package_name})
        get_property(targets GLOBAL PROPERTY LY_PACKAGE_TARGETS_${package_name})
        string(APPEND builtinpackages "ly_associate_package(PACKAGE_NAME ${package_name} TARGETS ${targets} PACKAGE_HASH ${package_hash})\n")
    endforeach()

    ly_get_absolute_pal_filename(pal_builtin_file ${CMAKE_CURRENT_BINARY_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
    file(GENERATE OUTPUT ${pal_builtin_file}
        CONTENT ${builtinpackages}
    )
    install(FILES "${pal_builtin_file}"
        DESTINATION cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}
    )

endfunction()

#! ly_setup_others: install directories required by the engine
function(ly_setup_others)

    # List of directories we want to install relative to engine root
    set(DIRECTORIES_TO_INSTALL Tools/LyTestTools Tools/RemoteConsole ctest_scripts scripts)
    foreach(dir ${DIRECTORIES_TO_INSTALL})

        get_filename_component(install_path ${dir} DIRECTORY)
        if (NOT install_path)
            set(install_path .)
        endif()

        install(DIRECTORY "${CMAKE_SOURCE_DIR}/${dir}"
            DESTINATION ${install_path}
        )

    endforeach()

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"
        DESTINATION .
        REGEX "downloaded_packages" EXCLUDE
        REGEX "runtime" EXCLUDE
    )

    # Registry
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/Registry
        DESTINATION ./bin/$<CONFIG>
    )
    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/Registry
        DESTINATION .
    )

    # Engine Source Assets
    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/Assets
        DESTINATION .
    )

    # Gem Source Assets
    # Find all gem directories relative to the CMake Source Dir
    file(GLOB_RECURSE gems_assets_path RELATIVE ${CMAKE_SOURCE_DIR} "Gems/*/Assets")
    foreach (gem_assets_path ${gems_assets_path})

        set(gem_abs_assets_path ${CMAKE_SOURCE_DIR}/${gem_assets_path}/)
        if (EXISTS ${gem_abs_assets_path})
            # The trailing slash is IMPORTANT here as that is needed to prevent
            # the "Assets" folder from being copied underneath the <gem-root>/Assets folder
            install(DIRECTORY ${gem_abs_assets_path}
                DESTINATION ${gem_assets_path}
            )
        endif()
    endforeach()


    # Qt Binaries
    set(QT_BIN_DIRS bearer iconengines imageformats platforms styles translations)
    foreach(qt_dir ${QT_BIN_DIRS})
        install(DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/${qt_dir}
            DESTINATION ./bin/$<CONFIG>
        )
    endforeach()

    # Templates
    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/Templates
        DESTINATION .
    )

    # Misc
    install(FILES
        ${CMAKE_SOURCE_DIR}/ctest_pytest.ini
        ${CMAKE_SOURCE_DIR}/LICENSE.txt
        ${CMAKE_SOURCE_DIR}/README.md
        DESTINATION .
    )

endfunction()


#! ly_setup_target_generator: install source files needed for project launcher generation
function(ly_setup_target_generator)

    install(FILES
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/launcher_generator.cmake
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/launcher_project_files.cmake
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/LauncherProject.cpp
        ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/StaticModules.in
        DESTINATION LauncherGenerator
    )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/Platform
        DESTINATION LauncherGenerator
    )
    install(FILES ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/FindLauncherGenerator.cmake
        DESTINATION cmake
    )

endfunction()