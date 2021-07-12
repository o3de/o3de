#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(CMAKE_INSTALL_MESSAGE NEVER) # Simplify messages to reduce output noise

ly_set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Core)

cmake_path(RELATIVE_PATH CMAKE_RUNTIME_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE runtime_output_directory)
cmake_path(RELATIVE_PATH CMAKE_LIBRARY_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE library_output_directory)
# Anywhere CMAKE_INSTALL_PREFIX is used, it has to be escaped so it is baked into the cmake_install.cmake script instead
# of baking the path. This is needed so `cmake --install --prefix <someprefix>` works regardless of the CMAKE_INSTALL_PREFIX
# used to generate the solution.
# CMAKE_INSTALL_PREFIX is still used when building the INSTALL target
set(install_output_folder "\${CMAKE_INSTALL_PREFIX}/${runtime_output_directory}/${PAL_PLATFORM_NAME}/$<CONFIG>")


function(ly_get_engine_relative_source_dir absolute_target_source_dir output_source_dir)
    # Get a relative target source directory to the LY root folder if possible
    # Otherwise use the final component name
    cmake_path(IS_PREFIX LY_ROOT_FOLDER ${absolute_target_source_dir} is_target_prefix_of_engine_root)
    if(is_target_prefix_of_engine_root)
        cmake_path(RELATIVE_PATH absolute_target_source_dir BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE relative_target_source_dir)
    else()
        # In this case the target source directory is outside of the engine root of the target source directory and concatenate the first
        # is used first 8 characters of the absolute path SHA256 hash to make a unique relative directory
        # that can be used to install the generated CMakeLists.txt
        # of a SHA256 hash
        string(SHA256 target_source_hash ${absolute_target_source_dir})
        string(SUBSTRING ${target_source_hash} 0 8 target_source_hash)
        cmake_path(GET absolute_target_source_dir FILENAME target_source_dirname)
        cmake_path(SET relative_target_source_dir "${target_source_dirname}-${target_source_hash}")
    endif()

    set(${output_source_dir} ${relative_target_source_dir} PARENT_SCOPE)
endfunction()

#! ly_setup_target: Setup the data needed to re-create the cmake target commands for a single target
function(ly_setup_target OUTPUT_CONFIGURED_TARGET ALIAS_TARGET_NAME absolute_target_source_dir)
    # De-alias target name
    ly_de_alias_target(${ALIAS_TARGET_NAME} TARGET_NAME)

    # Get the target source directory relative to the LY root folder
    ly_get_engine_relative_source_dir(${absolute_target_source_dir} relative_target_source_dir)

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

                cmake_path(NORMAL_PATH include_directory)
                string(REGEX REPLACE "/$" "" include_directory "${include_directory}")
                cmake_path(IS_PREFIX LY_ROOT_FOLDER ${absolute_target_source_dir} NORMALIZE include_directory_child_of_o3de_root)
                if(NOT include_directory_child_of_o3de_root)
                    # Include directory is outside of the O3DE root folder ${LY_ROOT_FOLDER}.
                    # For the INSTALL step, the O3DE root folder must be a prefix of all include directories.
                    continue()
                endif()

                cmake_path(RELATIVE_PATH include_directory BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE rel_include_dir)
                cmake_path(APPEND include_location "${rel_include_dir}" ".." OUTPUT_VARIABLE destination_dir)
                cmake_path(NORMAL_PATH destination_dir)

                install(DIRECTORY ${include_directory}
                    DESTINATION ${destination_dir}
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
    cmake_path(RELATIVE_PATH CMAKE_ARCHIVE_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE archive_output_directory)

    get_target_property(target_runtime_output_directory ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if(target_runtime_output_directory)
        cmake_path(RELATIVE_PATH target_runtime_output_directory BASE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} OUTPUT_VARIABLE target_runtime_output_subdirectory)
    endif()

    get_target_property(target_library_output_directory ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)
    if(target_library_output_directory)
        cmake_path(RELATIVE_PATH target_library_output_directory BASE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} OUTPUT_VARIABLE target_library_output_subdirectory)
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
                cmake_path(RELATIVE_PATH include BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE target_include)
                cmake_path(NORMAL_PATH target_include)
                # Escape the LY_ROOT_FOLDER variable so that it isn't resolved during the install step
                string(APPEND INCLUDE_DIRECTORIES_PLACEHOLDER "\${LY_ROOT_FOLDER}/${include_location}/${target_include}\n")
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

    set(target_install_source_dir ${CMAKE_CURRENT_BINARY_DIR}/install/${relative_target_source_dir})
    file(GENERATE OUTPUT "${target_install_source_dir}/${NAME_PLACEHOLDER}_$<CONFIG>.cmake" CONTENT "${target_file_contents}")
    install(FILES "${target_install_source_dir}/${NAME_PLACEHOLDER}_$<CONFIG>.cmake"
        DESTINATION ${relative_target_source_dir}
        COMPONENT ${install_component}
    )

    # Since a CMakeLists.txt could contain multiple targets, we generate it in a folder per target
    ly_file_read(${LY_ROOT_FOLDER}/cmake/install/InstalledTarget.in target_cmakelists_template)
    string(CONFIGURE ${target_cmakelists_template} output_cmakelists_data @ONLY)
    set(${OUTPUT_CONFIGURED_TARGET} ${output_cmakelists_data} PARENT_SCOPE)
endfunction()


#! ly_setup_subdirectories: setups all targets on a per directory basis
function(ly_setup_subdirectories)
    get_property(all_subdirectories GLOBAL PROPERTY LY_ALL_TARGET_DIRECTORIES)
    foreach(target_subdirectory IN LISTS all_subdirectories)
        ly_setup_subdirectory(${target_subdirectory})
    endforeach()
endfunction()


#! ly_setup_subdirectory: setup all targets in the subdirectory
function(ly_setup_subdirectory absolute_target_source_dir)
    # Get the target source directory relative to the LY roo folder
    ly_get_engine_relative_source_dir(${absolute_target_source_dir} relative_target_source_dir)

    # The builtin BUILDSYSTEM_TARGETS property isn't being used here as that returns the de-alised
    # TARGET and we need the alias namespace for recreating the CMakeLists.txt in the install layout
    get_property(ALIAS_TARGETS_NAME DIRECTORY ${absolute_target_source_dir} PROPERTY LY_DIRECTORY_TARGETS)
    foreach(ALIAS_TARGET_NAME IN LISTS ALIAS_TARGETS_NAME)
        ly_setup_target(configured_target ${ALIAS_TARGET_NAME} ${absolute_target_source_dir})
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


    # Reproduce the ly_enable_gems() calls made in the the SOURCE_DIR for this target into the CMakeLists.txt that
    # is about to be generated
    set(enable_gems_template "ly_enable_gems(@enable_gem_PROJECT_NAME@ @enable_gem_GEMS@ @enable_gem_GEM_FILE@ @enable_gem_VARIANTS@ @enable_gem_TARGETS@)\n")
    get_property(enable_gems_commands_arg_list DIRECTORY ${absolute_target_source_dir} PROPERTY LY_ENABLE_GEMS_ARGUMENTS)
    foreach(enable_gems_single_command_arg_list ${enable_gems_commands_arg_list})
        # Split the ly_enable_gems arguments back out based on commas
        string(REPLACE "," ";" enable_gems_single_command_arg_list "${enable_gems_single_command_arg_list}")
        foreach(enable_gem_arg_kw IN ITEMS PROJECT_NAME GEMS GEM_FILE VARIANTS TARGETS)
            list(POP_FRONT enable_gems_single_command_arg_list enable_gem_${enable_gem_arg_kw})
            if(enable_gem_${enable_gem_arg_kw})
                # if the argument exist append to argument keyword to the front
                string(PREPEND enable_gem_${enable_gem_arg_kw} "${enable_gem_arg_kw} ")
            endif()
        endforeach()

        string(CONFIGURE "${enable_gems_template}" enable_gems_command @ONLY)
        string(APPEND ENABLE_GEMS_PLACEHOLDER ${enable_gems_command})
    endforeach()

    ly_file_read(${LY_ROOT_FOLDER}/cmake/install/Copyright.in cmake_copyright_comment)

    # Initialize the target install source directory to path underneath the current binary directory
    set(target_install_source_dir ${CMAKE_CURRENT_BINARY_DIR}/install/${relative_target_source_dir})
    # Write out all the aggregated ly_add_target function calls and the final ly_create_alias() calls to the target CMakeLists.txt
    file(WRITE ${target_install_source_dir}/CMakeLists.txt
        "${cmake_copyright_comment}"
        "${all_configured_targets}"
        "\n"
        "${CREATE_ALIASES_PLACEHOLDER}"
        "${ENABLE_GEMS_PLACEHOLDER}"
    )

    # get the component ID. if the property isn't set for the directory, it will auto fallback to use CMAKE_INSTALL_DEFAULT_COMPONENT_NAME
    get_property(install_component DIRECTORY ${absolute_target_source_dir} PROPERTY INSTALL_COMPONENT)

    install(FILES "${target_install_source_dir}/CMakeLists.txt"
        DESTINATION ${relative_target_source_dir}
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
        cmake_path(RELATIVE_PATH external_subdir BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE engine_rel_external_subdir)
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
        cmake_path(RELATIVE_PATH target_subdirectory BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE relative_target_subdirectory)
        string(APPEND FIND_PACKAGES_PLACEHOLDER "    add_subdirectory(${relative_target_subdirectory})\n")
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

    set(pal_builtin_file ${CMAKE_CURRENT_BINARY_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
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
"function(ly_copy source_file target_directory)
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
            cmake_path(RELATIVE_PATH target_runtime_output_directory BASE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} OUTPUT_VARIABLE target_runtime_output_subdirectory)
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

    # Gem Source Assets and configuration files
    # Find all gem directories relative to the CMake Source Dir

    # This first loop is to filter out transient and .gitignore'd folders that should be added to
    # the install layout from the root directory. Such as <external-subdirectory-root>/Cache.
    # This is also done to avoid globbing thousands of files in subdirectories that shouldn't
    # be processed.
    foreach(gem_candidate_dir IN LISTS LY_EXTERNAL_SUBDIRS LY_PROJECTS)
        file(REAL_PATH ${gem_candidate_dir} gem_candidate_dir BASE_DIRECTORY ${LY_ROOT_FOLDER})
        # Don't recurse immediately in order to exclude transient source artifacts
        file(GLOB
            external_subdir_files
            LIST_DIRECTORIES TRUE
            "${gem_candidate_dir}/*"
        )
        # Exclude transient artifacts that shouldn't be copied to the install layout
        list(FILTER external_subdir_files EXCLUDE REGEX "/([Bb]uild|[Cc]ache|[Uu]ser)$")
        list(APPEND filtered_asset_paths ${external_subdir_files})
    endforeach()

    # At this point the filtered_assets_paths contains the list of all directories and files
    # that are non-excluded candidates that can be scanned for target directories and files
    # to copy over to the install layout
    foreach(filtered_asset_path IN LISTS filtered_asset_paths)
        if(IS_DIRECTORY ${filtered_asset_path})
            file(GLOB_RECURSE
                recurse_assets_paths
                LIST_DIRECTORIES TRUE
                "${filtered_asset_path}/*"
            )
            set(gem_file_paths ${recurse_assets_paths})
            # Make sure to prepend the current path iteration to the gem_dirs_path to filter
            set(gem_dir_paths ${filtered_asset_path} ${recurse_assets_paths})

            # Gather directories to copy over
            # Currently only the Assets, Registry and Config directories are copied over
            list(FILTER gem_dir_paths INCLUDE REGEX "/(Assets|Registry|Config)$")
            list(APPEND gems_assets_dir_path ${gem_dir_paths})
        else()
            set(gem_file_paths ${filtered_asset_path})
        endif()

        # Gather files to copy over
        # Currently only the gem.json file is copied over
        list(FILTER gem_file_paths INCLUDE REGEX "/(gem.json)$")
        list(APPEND gems_assets_file_path "${gem_file_paths}")
    endforeach()

    # gem directories to install
    foreach(gem_absolute_dir_path ${gems_assets_dir_path})
        cmake_path(RELATIVE_PATH gem_absolute_dir_path BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE gem_relative_dir_path)
        if (EXISTS ${gem_absolute_dir_path})
            # The trailing slash is IMPORTANT here as that is needed to prevent
            # the "Assets" folder from being copied underneath the <gem-root>/Assets folder
            install(DIRECTORY "${gem_absolute_dir_path}/"
                DESTINATION ${gem_relative_dir_path}
            )
        endif()
    endforeach()

    # gem files to install
    foreach(gem_absolute_file_path ${gems_assets_file_path})
        cmake_path(RELATIVE_PATH gem_absolute_file_path BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE gem_relative_file_path)
        cmake_path(GET gem_relative_file_path PARENT_PATH gem_relative_parent_dir)
        install(FILES ${gem_absolute_file_path}
            DESTINATION ${gem_relative_parent_dir}
        )
    endforeach()

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
