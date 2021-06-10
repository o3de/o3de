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

ly_set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Core)

file(RELATIVE_PATH runtime_output_directory ${CMAKE_BINARY_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(RELATIVE_PATH library_output_directory ${CMAKE_BINARY_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
# Anywhere CMAKE_INSTALL_PREFIX is used, it has to be escaped so it is baked into the cmake_install.cmake script instead
# of baking the path. This is needed so `cmake --install --prefix <someprefix>` works regardless of the CMAKE_INSTALL_PREFIX
# used to generate the solution.
# CMAKE_INSTALL_PREFIX is still used when building the INSTALL target
set(install_output_folder "\${CMAKE_INSTALL_PREFIX}/${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>")


#! ly_setup_target: Setup the data needed to re-create the cmake target commands for a single target
function(ly_setup_target OUTPUT_CONFIGURED_TARGET ALIAS_TARGET_NAME)
    # De-alias target name
    ly_de_alias_target(${ALIAS_TARGET_NAME} TARGET_NAME)

    # get the component ID.  if the property isn't set for the target, it will auto fallback to use CMAKE_INSTALL_DEFAULT_COMPONENT_NAME
    get_property(install_component TARGET ${TARGET_NAME} PROPERTY INSTALL_COMPONENT)

    # All include directories marked PUBLIC or INTERFACE will be installed. We dont use PUBLIC_HEADER because in order to do that
    # we need to set the PUBLIC_HEADER property of the target for all the headers we are exporting. After doing that, installing the
    # headers end up in one folder instead of duplicating the folder structure of the public/interface include directory.
    # Instead, we install them with install(DIRECTORY)
    set(include_location "include")
    get_target_property(include_directories ${TARGET_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    if (include_directories)
        unset(public_headers)
        foreach(include_directory ${include_directories})
            string(GENEX_STRIP ${include_directory} include_genex_expr)
            if(include_genex_expr STREQUAL include_directory) # only for cases where there are no generation expressions
                unset(current_public_headers)
                install(DIRECTORY ${include_directory}
                    DESTINATION ${include_location}/${target_source_dir}
                    COMPONENT ${install_component}
                    FILES_MATCHING
                        PATTERN *.h
                        PATTERN *.hpp
                        PATTERN *.inl
                )
            endif()
        endforeach()
    endif()

    # Get the output folders, archive is always the same, but runtime/library can be in subfolders defined per target
    file(RELATIVE_PATH archive_output_directory ${CMAKE_BINARY_DIR} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

    get_target_property(target_runtime_output_directory ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if(target_runtime_output_directory)
        file(RELATIVE_PATH target_runtime_output_subdirectory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${target_runtime_output_directory})
    endif()

    get_target_property(target_library_output_directory ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)
    if(target_library_output_directory)
        file(RELATIVE_PATH target_library_output_subdirectory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${target_library_output_directory})
    endif()

    install(
        TARGETS ${TARGET_NAME}
        ARCHIVE
            DESTINATION ${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>
            COMPONENT ${install_component}
        LIBRARY
            DESTINATION ${library_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_library_output_subdirectory}
            COMPONENT ${install_component}
        RUNTIME
            DESTINATION ${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_runtime_output_subdirectory}
            COMPONENT ${install_component}
    )

    # CMakeLists.txt file
    string(REGEX MATCH "(.*)::(.*)$" match ${ALIAS_TARGET_NAME})
    if(match)
        set(NAMESPACE_PLACEHOLDER "NAMESPACE ${CMAKE_MATCH_1}")
        set(NAME_PLACEHOLDER ${CMAKE_MATCH_2})
    else()
        set(NAMESPACE_PLACEHOLDER "")
        set(NAME_PLACEHOLDER ${TARGET_NAME})
    endif()

    set(TARGET_TYPE_PLACEHOLDER "")
    get_target_property(target_type ${NAME_PLACEHOLDER} TYPE)
    # Remove the _LIBRARY since we dont need to pass that to ly_add_targets
    string(REPLACE "_LIBRARY" "" TARGET_TYPE_PLACEHOLDER ${target_type})
    # For HEADER_ONLY libs we end up generating "INTERFACE" libraries, need to specify HEADERONLY instead
    string(REPLACE "INTERFACE" "HEADERONLY" TARGET_TYPE_PLACEHOLDER ${TARGET_TYPE_PLACEHOLDER})
    if(TARGET_TYPE_PLACEHOLDER STREQUAL "MODULE")
        get_target_property(gem_module ${NAME_PLACEHOLDER} GEM_MODULE)
        if(gem_module)
            set(TARGET_TYPE_PLACEHOLDER "GEM_MODULE")
        endif()
    endif()

    get_target_property(COMPILE_DEFINITIONS_PLACEHOLDER ${TARGET_NAME} INTERFACE_COMPILE_DEFINITIONS)
    if(COMPILE_DEFINITIONS_PLACEHOLDER)
        string(REPLACE ";" "\n" COMPILE_DEFINITIONS_PLACEHOLDER "${COMPILE_DEFINITIONS_PLACEHOLDER}")
    else()
        unset(COMPILE_DEFINITIONS_PLACEHOLDER)
    endif()

    # Includes need additional processing to add the install root
    if(include_directories)
        foreach(include ${include_directories})
            string(GENEX_STRIP ${include} include_genex_expr)
            if(include_genex_expr STREQUAL include) # only for cases where there are no generation expressions
                file(RELATIVE_PATH relative_include ${absolute_target_source_dir} ${include})
                string(APPEND INCLUDE_DIRECTORIES_PLACEHOLDER "\${LY_ROOT_FOLDER}/include/${target_source_dir}/${relative_include}\n")
            endif()
        endforeach()
    endif()

    get_target_property(RUNTIME_DEPENDENCIES_PLACEHOLDER ${TARGET_NAME} MANUALLY_ADDED_DEPENDENCIES)
    if(RUNTIME_DEPENDENCIES_PLACEHOLDER) # not found properties return the name of the variable with a "-NOTFOUND" at the end, here we set it to empty if not found
        string(REPLACE ";" "\n" RUNTIME_DEPENDENCIES_PLACEHOLDER "${RUNTIME_DEPENDENCIES_PLACEHOLDER}")
    else()
        unset(RUNTIME_DEPENDENCIES_PLACEHOLDER)
    endif()

    get_target_property(inteface_build_dependencies_props ${TARGET_NAME} INTERFACE_LINK_LIBRARIES)
    unset(INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER)
    if(inteface_build_dependencies_props)
        foreach(build_dependency ${inteface_build_dependencies_props})
            # Skip wrapping produced when targets are not created in the same directory
            if(NOT ${build_dependency} MATCHES "^::@")
                list(APPEND INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER "${build_dependency}")
            endif()
        endforeach()
    endif()
    # We also need to pass the private link libraries since we will use that to generate the runtime dependencies
    get_target_property(private_build_dependencies_props ${TARGET_NAME} LINK_LIBRARIES)
    if(private_build_dependencies_props)
        foreach(build_dependency ${private_build_dependencies_props})
            # Skip wrapping produced when targets are not created in the same directory
            if(NOT ${build_dependency} MATCHES "^::@")
                list(APPEND INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER "${build_dependency}")
            endif()
        endforeach()
    endif()
    list(REMOVE_DUPLICATES INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER)
    string(REPLACE ";" "\n" INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER "${INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER}")

    # Config file
    set(target_file_contents "# Generated by O3DE install\n\n")
    if(NOT target_type STREQUAL INTERFACE_LIBRARY)

        unset(target_location)
        set(runtime_types EXECUTABLE APPLICATION)
        if(target_type IN_LIST runtime_types)
            set(target_location "\${LY_ROOT_FOLDER}/${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_runtime_output_subdirectory}/$<TARGET_FILE_NAME:${TARGET_NAME}>")
        elseif(target_type STREQUAL MODULE_LIBRARY)
            set(target_location "\${LY_ROOT_FOLDER}/${library_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_library_output_subdirectory}/$<TARGET_FILE_NAME:${TARGET_NAME}>")
        elseif(target_type STREQUAL SHARED_LIBRARY)
            string(APPEND target_file_contents 
"set_property(TARGET ${TARGET_NAME} 
    APPEND_STRING PROPERTY IMPORTED_IMPLIB
        $<$<CONFIG:$<CONFIG>$<ANGLE-R>:\"\${LY_ROOT_FOLDER}/${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}>\"$<ANGLE-R>
)
")
            string(APPEND target_file_contents 
"set_property(TARGET ${TARGET_NAME} 
    PROPERTY IMPORTED_IMPLIB_$<UPPER_CASE:$<CONFIG>> 
        \"\${LY_ROOT_FOLDER}/${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}>\"
)
")
            set(target_location "\${LY_ROOT_FOLDER}/${library_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/${target_library_output_subdirectory}/$<TARGET_FILE_NAME:${TARGET_NAME}>")
        else() # STATIC_LIBRARY, OBJECT_LIBRARY, INTERFACE_LIBRARY
            set(target_location "\${LY_ROOT_FOLDER}/${archive_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>/$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}>")
        endif()

        if(target_location)
            string(APPEND target_file_contents
"set_property(TARGET ${TARGET_NAME}
    APPEND_STRING PROPERTY IMPORTED_LOCATION
        $<$<CONFIG:$<CONFIG>$<ANGLE-R>:${target_location}$<ANGLE-R>
)
set_property(TARGET ${TARGET_NAME}
    PROPERTY IMPORTED_LOCATION_$<UPPER_CASE:$<CONFIG>>
        ${target_location}
)
")
        endif()
    endif()

    if(IS_ABSOLUTE ${target_source_dir})
        # This normally applies the target_source_dir is outside of the engine root
        # such as when invoking ly_setup_subdirectory from the project
        # Therefore the final directory component of the target source directory is used first 8 characters
        # of a SHA256 hash
        string(SHA256 target_source_hash ${target_source_dir})
        string(SUBSTRING ${target_source_hash} 0 8 target_source_hash)
        get_filename_component(target_source_folder_name ${target_source_dir} NAME)
        set(target_source_dir "${target_source_folder_name}-${target_source_hash}")
    endif()

    set(target_install_source_dir ${CMAKE_CURRENT_BINARY_DIR}/install/${target_source_dir})
    file(GENERATE OUTPUT "${target_install_source_dir}/${NAME_PLACEHOLDER}_$<CONFIG>.cmake" CONTENT "${target_file_contents}")
    install(FILES "${target_install_source_dir}/${NAME_PLACEHOLDER}_$<CONFIG>.cmake"
        DESTINATION ${target_source_dir}
        COMPONENT ${install_component}
    )

    # Since a CMakeLists.txt could contain multiple targets, we generate it in a folder per target
    file(READ ${LY_ROOT_FOLDER}/cmake/install/InstalledTarget.in target_cmakelists_template)
    string(CONFIGURE ${target_cmakelists_template} output_cmakelists_data @ONLY)
    set(${OUTPUT_CONFIGURED_TARGET} ${output_cmakelists_data} PARENT_SCOPE)
endfunction()

#! ly_setup_subdirectories: setups all targets on a per directory basis
function(ly_setup_subdirectories)
    get_property(all_subdirectories GLOBAL PROPERTY LY_ALL_TARGET_DIRECTORIES)
    foreach(target IN LISTS all_subdirectories)
        ly_setup_subdirectory(${target})
    endforeach()
endfunction()


#! ly_setup_subdirectory: setup  all targets in the subdirectory
function(ly_setup_subdirectory absolute_target_source_dir)

    # The builtin BUILDSYSTEM_TARGETS property isn't being used here as that returns the de-alised
    # TARGET and we need the alias namespace for recreating the CMakeLists.txt in the install layout
    get_property(ALIAS_TARGETS_NAME DIRECTORY ${absolute_target_source_dir} PROPERTY LY_DIRECTORY_TARGETS)
    file(RELATIVE_PATH target_source_dir ${LY_ROOT_FOLDER} ${absolute_target_source_dir})
    foreach(ALIAS_TARGET_NAME IN LISTS ALIAS_TARGETS_NAME)
        ly_setup_target(configured_target ${ALIAS_TARGET_NAME})
        string(APPEND all_configured_targets "${configured_target}")
    endforeach()

    # Replicate the ly_create_alias() calls based on the SOURCE_DIR for each target that generates an installed CMakeLists.txt
    string(JOIN "\n" create_alias_template
        "if(NOT TARGET @ALIAS_NAME@)"
        "   ly_create_alias(NAME @ALIAS_NAME@ NAMESPACE @ALIAS_NAMESPACE@ TARGETS @ALIAS_TARGETS@)"
        "endif()"
        ""
    )
    get_property(create_alias_commands_arg_list DIRECTORY ${absolute_target_source_dir} PROPERTY LY_CREATE_ALIAS_ARGUMENTS)
    foreach(create_alias_single_command_arg_list ${create_alias_commands_arg_list})
        # Split the ly_create_alias arguments back out based on commas
        string(REPLACE "," ";" create_alias_single_command_arg_list "${create_alias_single_command_arg_list}")
        list(POP_FRONT create_alias_single_command_arg_list ALIAS_NAME)
        list(POP_FRONT create_alias_single_command_arg_list ALIAS_NAMESPACE)
        # The rest of the list are the target dependencies
        set(ALIAS_TARGETS ${create_alias_single_command_arg_list})
        string(CONFIGURE "${create_alias_template}" create_alias_command @ONLY)
        string(APPEND CREATE_ALIASES_PLACEHOLDER ${create_alias_command})
    endforeach()

    file(READ ${LY_ROOT_FOLDER}/cmake/install/Copyright.in cmake_copyright_comment)

    if(IS_ABSOLUTE ${target_source_dir})
        # This normally applies the target_source_dir is outside of the engine root
        # such as when invoking ly_setup_subdirectory from the project
        # Therefore the final directory component of the target source directory is used first 8 characters
        # of a SHA256 hash
        string(SHA256 target_source_hash ${target_source_dir})
        string(SUBSTRING ${target_source_hash} 0 8 target_source_hash)
        get_filename_component(target_source_folder_name ${target_source_dir} NAME)
        set(target_source_dir "${target_source_folder_name}-${target_source_hash}")
    endif()

    # Initialize the target install source directory to path underneath the current binary directory
    set(target_install_source_dir ${CMAKE_CURRENT_BINARY_DIR}/install/${target_source_dir})
    # Write out all the aggregated ly_add_target function calls and the final ly_create_alias() calls to the target CMakeLists.txt
    file(WRITE ${target_install_source_dir}/CMakeLists.txt
        "${cmake_copyright_comment}"
        "${all_configured_targets}"
        "\n"
        "${CREATE_ALIASES_PLACEHOLDER}"
    )

    # get the component ID. if the property isn't set for the directory, it will auto fallback to use CMAKE_INSTALL_DEFAULT_COMPONENT_NAME
    get_property(install_component DIRECTORY ${absolute_target_source_dir} PROPERTY INSTALL_COMPONENT)

    install(FILES "${target_install_source_dir}/CMakeLists.txt"
        DESTINATION ${target_source_dir}
        COMPONENT ${install_component}
    )

endfunction()

#! ly_setup_o3de_install: orchestrates the installation of the different parts. This is the entry point from the root CMakeLists.txt
function(ly_setup_o3de_install)

    ly_setup_subdirectories()
    ly_setup_cmake_install()
    ly_setup_target_generator()
    ly_setup_runtime_dependencies()
    ly_setup_others()

endfunction()

#! ly_setup_cmake_install: install the "cmake" folder
function(ly_setup_cmake_install)

    install(DIRECTORY "${LY_ROOT_FOLDER}/cmake"
        DESTINATION .
        PATTERN "__pycache__" EXCLUDE
        REGEX "Findo3de.cmake" EXCLUDE
        REGEX "Platform\/.*\/BuiltInPackages_.*\.cmake" EXCLUDE
    )

    # Transform the LY_EXTERNAL_SUBDIRS list into a json array
    set(indent "        ")
    foreach(external_subdir ${LY_EXTERNAL_SUBDIRS})
        file(RELATIVE_PATH engine_rel_external_subdir ${LY_ROOT_FOLDER} ${external_subdir})
        list(APPEND relative_external_subdirs "\"${engine_rel_external_subdir}\"")
    endforeach()
    list(JOIN relative_external_subdirs ",\n${indent}" LY_INSTALL_EXTERNAL_SUBDIRS)

    # Read the "templates" key from the source engine.json
    o3de_read_json_array(engine_templates ${LY_ROOT_FOLDER}/engine.json "templates")
    foreach(template_path ${engine_templates})
        list(APPEND relative_templates "\"${template_path}\"")
    endforeach()
    list(JOIN relative_templates ",\n${indent}" LY_INSTALL_TEMPLATES)

    configure_file(${LY_ROOT_FOLDER}/cmake/install/engine.json.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/engine.json @ONLY)

    install(
        FILES
            "${LY_ROOT_FOLDER}/CMakeLists.txt"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake/engine.json"
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
    unset(FIND_PACKAGES_PLACEHOLDER)

    # Add to the FIND_PACKAGES_PLACEHOLDER all directories in which ly_add_target were called in
    get_property(all_subdirectories GLOBAL PROPERTY LY_ALL_TARGET_DIRECTORIES)
    foreach(target_subdirectory IN LISTS all_subdirectories)
        file(RELATIVE_PATH target_source_dir_relative ${LY_ROOT_FOLDER} ${target_subdirectory})
        string(APPEND FIND_PACKAGES_PLACEHOLDER "    add_subdirectory(${target_source_dir_relative})\n")
    endforeach()

    configure_file(${LY_ROOT_FOLDER}/cmake/install/Findo3de.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/Findo3de.cmake @ONLY)
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
    )

    unset(runtime_commands)
    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    foreach(alias_target IN LISTS all_targets)
        ly_de_alias_target(${alias_target} target)

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

        install(DIRECTORY "${LY_ROOT_FOLDER}/${dir}"
            DESTINATION ${install_path}
            PATTERN "__pycache__" EXCLUDE
        )

    endforeach()

    # Scripts
    file(GLOB o3de_scripts "${LY_ROOT_FOLDER}/scripts/o3de.*")
    install(FILES
        ${o3de_scripts}
        DESTINATION ./scripts
    )

    install(DIRECTORY
        ${LY_ROOT_FOLDER}/scripts/bundler
        ${LY_ROOT_FOLDER}/scripts/o3de
        DESTINATION ./scripts
        PATTERN "__pycache__" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "tests" EXCLUDE
    )

    install(DIRECTORY "${LY_ROOT_FOLDER}/python"
        DESTINATION .
        REGEX "downloaded_packages" EXCLUDE
        REGEX "runtime" EXCLUDE
    )

    # Registry
    install(DIRECTORY
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Registry
        DESTINATION ./${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>
    )
    install(DIRECTORY
        ${LY_ROOT_FOLDER}/Registry
        DESTINATION .
    )

    # Engine Source Assets
    install(DIRECTORY
        ${LY_ROOT_FOLDER}/Assets
        DESTINATION .
    )

    # Gem Source Assets and Registry
    # Find all gem directories relative to the CMake Source Dir
    file(GLOB_RECURSE
        gems_assets_path
        LIST_DIRECTORIES TRUE
        RELATIVE "${LY_ROOT_FOLDER}/"
        "Gems/*"
    )
    list(FILTER gems_assets_path INCLUDE REGEX "/(Assets|Registry)$")

    foreach (gem_assets_path ${gems_assets_path})
        set(gem_abs_assets_path ${LY_ROOT_FOLDER}/${gem_assets_path}/)
        if (EXISTS ${gem_abs_assets_path})
            # The trailing slash is IMPORTANT here as that is needed to prevent
            # the "Assets" folder from being copied underneath the <gem-root>/Assets folder
            install(DIRECTORY ${gem_abs_assets_path}
                DESTINATION ${gem_assets_path}
            )
        endif()
    endforeach()

    # gem.json files
    file(GLOB_RECURSE
        gems_json_path
        LIST_DIRECTORIES FALSE
        RELATIVE "${LY_ROOT_FOLDER}"
        "Gems/*/gem.json"
    )
    foreach(gem_json_path ${gems_json_path})
        get_filename_component(gem_relative_path ${gem_json_path} DIRECTORY)
        install(FILES ${gem_json_path}
            DESTINATION ${gem_relative_path}
        )
    endforeach()

    # Additional files needed by gems
    install(DIRECTORY
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/ImageProcessingAtom/Config
        DESTINATION Gems/Atom/Asset/ImageProcessingAtom
    )

    # Templates
    install(DIRECTORY
        ${LY_ROOT_FOLDER}/Templates
        DESTINATION .
    )

    # Misc
    install(FILES
        ${LY_ROOT_FOLDER}/ctest_pytest.ini
        ${LY_ROOT_FOLDER}/LICENSE.txt
        ${LY_ROOT_FOLDER}/README.md
        DESTINATION .
    )

endfunction()

#! ly_setup_target_generator: install source files needed for project launcher generation
function(ly_setup_target_generator)

    install(FILES
        ${LY_ROOT_FOLDER}/Code/LauncherUnified/launcher_generator.cmake
        ${LY_ROOT_FOLDER}/Code/LauncherUnified/launcher_project_files.cmake
        ${LY_ROOT_FOLDER}/Code/LauncherUnified/LauncherProject.cpp
        ${LY_ROOT_FOLDER}/Code/LauncherUnified/StaticModules.in
        DESTINATION LauncherGenerator
    )
    install(DIRECTORY ${LY_ROOT_FOLDER}/Code/LauncherUnified/Platform
        DESTINATION LauncherGenerator
    )
    install(FILES ${LY_ROOT_FOLDER}/Code/LauncherUnified/FindLauncherGenerator.cmake
        DESTINATION cmake
    )

endfunction()
