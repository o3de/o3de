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

ly_set(LY_DEFAULT_INSTALL_COMPONENT "Core")

file(RELATIVE_PATH runtime_output_directory ${CMAKE_BINARY_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(RELATIVE_PATH library_output_directory ${CMAKE_BINARY_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(install_output_folder "${CMAKE_INSTALL_PREFIX}/${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>")


#! ly_install_target: registers the target to be installed by cmake install.
#
# \arg:NAME name of the target
# \arg:COMPONENT the grouping string of the target used for splitting up the install
#                    into smaller packages.
# All other parameters are forwarded to ly_generate_target_find_file
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

    # Get the output folders, archive is always the same, but runtime/library can be in subfolders defined per target
    file(RELATIVE_PATH archive_output_directory ${CMAKE_BINARY_DIR} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

    get_target_property(target_runtime_output_directory ${ly_install_target_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if(target_runtime_output_directory)
        file(RELATIVE_PATH target_runtime_output_subdirectory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${target_runtime_output_directory})
    endif()

    get_target_property(target_library_output_directory ${ly_install_target_NAME} LIBRARY_OUTPUT_DIRECTORY)
    if(target_library_output_directory)
        file(RELATIVE_PATH target_library_output_subdirectory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${target_library_output_directory})
    endif()

    install(
        TARGETS ${ly_install_target_NAME}
        ARCHIVE
            DESTINATION ${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>
            COMPONENT ${ly_install_target_COMPONENT}
        LIBRARY
            DESTINATION ${library_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_library_output_subdirectory}
            COMPONENT ${ly_install_target_COMPONENT}
        RUNTIME
            DESTINATION ${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_runtime_output_subdirectory}
            COMPONENT ${ly_install_target_COMPONENT}
        PUBLIC_HEADER
            DESTINATION ${include_location}
            COMPONENT ${ly_install_target_COMPONENT}
    )

    ly_generate_target_find_file(
        NAME ${ly_install_target_NAME}
        ${ARGN}
    )
    ly_generate_target_config_file(${ly_install_target_NAME})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${ly_install_target_NAME}_$<CONFIG>.cmake"
        DESTINATION cmake_autogen/${ly_install_target_NAME}
        COMPONENT ${ly_install_target_COMPONENT}
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Find${ly_install_target_NAME}.cmake"
        DESTINATION cmake
        COMPONENT ${ly_install_target_COMPONENT}
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

    set(target_file_contents "# Generated by O3DE install\n\n")
    if(NOT target_type STREQUAL INTERFACE_LIBRARY)

        unset(target_location)
        set(runtime_types EXECUTABLE APPLICATION)
        if(target_type IN_LIST runtime_types)
            string(APPEND target_location "\"\${LY_ROOT_FOLDER}/${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_runtime_output_subdirectory}/$<TARGET_FILE_NAME:${NAME}>\"")
        elseif(target_type STREQUAL MODULE_LIBRARY)
            string(APPEND target_location "\"\${LY_ROOT_FOLDER}/${library_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_library_output_subdirectory}/$<TARGET_FILE_NAME:${NAME}>\"")
        elseif(target_type STREQUAL SHARED_LIBRARY)
            string(APPEND target_location "\"\${LY_ROOT_FOLDER}/${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/$<TARGET_LINKER_FILE_NAME:${NAME}>\"")
            string(APPEND target_file_contents "ly_add_dependencies(${NAME} \"\${LY_ROOT_FOLDER}/${library_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_library_output_subdirectory}/$<TARGET_FILE_NAME:${NAME}>\")\n")
        else() # STATIC_LIBRARY, OBJECT_LIBRARY, INTERFACE_LIBRARY
            string(APPEND target_location "\"\${LY_ROOT_FOLDER}/${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/$<TARGET_LINKER_FILE_NAME:${NAME}>\"")
        endif()

        string(APPEND target_file_contents
"set(target_location ${target_location})
set_target_properties(${NAME}
    PROPERTIES
        $<$<CONFIG:profile>:IMPORTED_LOCATION \"\${target_location}\">
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
    ly_setup_runtime_dependencies()
    ly_setup_others()

endfunction()

#! ly_setup_cmake_install: install the "cmake" folder
function(ly_setup_cmake_install)

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/cmake"
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
        REGEX "Findo3de.cmake" EXCLUDE
        REGEX "Platform\/.*\/BuiltInPackages_.*\.cmake" EXCLUDE
    )
    install(
        FILES
            "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
            "${CMAKE_SOURCE_DIR}/engine.json"
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
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
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
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
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
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
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )

endfunction()

#! ly_setup_runtime_dependencies: install runtime dependencies
function(ly_setup_runtime_dependencies)

    # Common functions used by the bellow code
    install(CODE
"function(ly_deploy_qt_install target_output)
    execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --verbose 0 --no-compiler-runtime \"\${target_output}\" ERROR_VARIABLE deploy_error RESULT_VARIABLE deploy_result)
    if (NOT \${deploy_result} EQUAL 0)
        if(NOT deploy_error MATCHES \"does not seem to be a Qt executable\" )
            message(SEND_ERROR \"Deploying qt for \${target_output} returned \${deploy_result}: \${deploy_error}\")
        endif()
    endif()
endfunction()

function(ly_copy source_file target_directory)
    file(COPY \"\${source_file}\" DESTINATION \"\${target_directory}\" FILE_PERMISSIONS ${LY_COPY_PERMISSIONS})
endfunction()"
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )
    
    unset(runtime_commands)
    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    foreach(target IN LISTS all_targets)

        # Exclude targets that dont produce runtime outputs
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
            continue()
        endif()
    
        get_target_property(target_runtime_output_directory ${target} RUNTIME_OUTPUT_DIRECTORY)
        if(target_runtime_output_directory)
            file(RELATIVE_PATH target_runtime_output_subdirectory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${target_runtime_output_directory})
        endif()
        
        # Qt
        get_property(has_qt_dependency GLOBAL PROPERTY LY_DETECT_QT_DEPENDENCY_${target})
        if(has_qt_dependency)
            # Qt deploy needs to be done after the binary is copied to the output, so we do a install(CODE) which effectively
            # puts it as a postbuild step of the "install" target. Binaries are copied at that point.
            if(NOT EXISTS ${WINDEPLOYQT_EXECUTABLE})
                message(FATAL_ERROR "Qt deploy executable not found: ${WINDEPLOYQT_EXECUTABLE}")
            endif()
            set(target_output "${install_output_folder}/${target_runtime_output_subdirectory}/$<TARGET_FILE_NAME:${target}>")
            list(APPEND runtime_commands "ly_deploy_qt_install(\"${target_output}\")\n")
        endif()

        # runtime dependencies that need to be copied to the output
        set(target_file_dir "${install_output_folder}/${target_runtime_output_subdirectory}")
        ly_get_runtime_dependencies(runtime_dependencies ${target})
        foreach(runtime_dependency ${runtime_dependencies})
            unset(runtime_command)
            ly_get_runtime_dependency_command(runtime_command ${runtime_dependency})
            string(CONFIGURE "${runtime_command}" runtime_command @ONLY)            
            list(APPEND runtime_commands ${runtime_command})
        endforeach()

    endforeach()

    list(REMOVE_DUPLICATES runtime_commands)
    list(JOIN runtime_commands "    " runtime_commands_str) # the spaces are just to see the right identation in the cmake_install.cmake file
    install(CODE "${runtime_commands_str}" 
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )
    
endfunction()

#! ly_setup_others: install directories required by the engine
function(ly_setup_others)

    # List of directories we want to install relative to engine root
    set(DIRECTORIES_TO_INSTALL Tools/LyTestTools Tools/RemoteConsole)
    foreach(dir ${DIRECTORIES_TO_INSTALL})

        get_filename_component(install_path ${dir} DIRECTORY)
        if (NOT install_path)
            set(install_path .)
        endif()

        install(DIRECTORY "${CMAKE_SOURCE_DIR}/${dir}"
            DESTINATION ${install_path}
            COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
        )

    endforeach()

    # Scripts
    file(GLOB o3de_scripts "${CMAKE_SOURCE_DIR}/scripts/o3de.*")
    install(FILES
        ${o3de_scripts}
        DESTINATION ./scripts
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )

    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts/bundler
        ${CMAKE_SOURCE_DIR}/scripts/project_manager
        DESTINATION ./scripts
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
        PATTERN "__pycache__" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "tests" EXCLUDE
    )

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
        REGEX "downloaded_packages" EXCLUDE
        REGEX "runtime" EXCLUDE
    )

    # Registry
    install(DIRECTORY
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Registry
        DESTINATION ./${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )
    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/Registry
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )

    # Engine Source Assets
    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/Assets
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )

    # Gem Source Assets and Registry
    # Find all gem directories relative to the CMake Source Dir
    file(GLOB_RECURSE
        gems_assets_path
        LIST_DIRECTORIES TRUE
        RELATIVE "${CMAKE_SOURCE_DIR}/"
        "Gems/*"
    )
    list(FILTER gems_assets_path INCLUDE REGEX "/(Assets|Registry)$")

    foreach (gem_assets_path ${gems_assets_path})
        set(gem_abs_assets_path ${CMAKE_SOURCE_DIR}/${gem_assets_path}/)
        if (EXISTS ${gem_abs_assets_path})
            # The trailing slash is IMPORTANT here as that is needed to prevent
            # the "Assets" folder from being copied underneath the <gem-root>/Assets folder
            install(DIRECTORY ${gem_abs_assets_path}
                DESTINATION ${gem_assets_path}
                COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
            )
        endif()
    endforeach()

    # Additional files needed by gems
    install(FILES
        ${CMAKE_SOURCE_DIR}/Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings
        DESTINATION Gems/ImageProcessing/Code/Source
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )  

    # Templates
    install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/Templates
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )

    # Misc
    install(FILES
        ${CMAKE_SOURCE_DIR}/ctest_pytest.ini
        ${CMAKE_SOURCE_DIR}/LICENSE.txt
        ${CMAKE_SOURCE_DIR}/README.md
        DESTINATION .
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
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
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/Platform
        DESTINATION LauncherGenerator
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )
    install(FILES ${CMAKE_SOURCE_DIR}/Code/LauncherUnified/FindLauncherGenerator.cmake
        DESTINATION cmake
        COMPONENT ${LY_DEFAULT_INSTALL_COMPONENT}
    )

endfunction()
