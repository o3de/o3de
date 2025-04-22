#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/FileUtil.cmake)

set(LY_INSTALL_EXTERNAL_BUILD_DIRS "" CACHE PATH "External build directories to be included in the install process. This allows to package non-monolithic and monolithic.")
unset(normalized_external_build_dirs)
foreach(external_dir ${LY_INSTALL_EXTERNAL_BUILD_DIRS})
    cmake_path(ABSOLUTE_PATH external_dir BASE_DIRECTORY ${LY_ROOT_FOLDER} NORMALIZE)
    list(APPEND normalized_external_build_dirs ${external_dir})
endforeach()
set(LY_INSTALL_EXTERNAL_BUILD_DIRS ${normalized_external_build_dirs})

set(CMAKE_INSTALL_MESSAGE NEVER) # Simplify messages to reduce output noise

define_property(TARGET PROPERTY LY_INSTALL_GENERATE_RUN_TARGET
    BRIEF_DOCS "Defines if a \"RUN\" targets should be created when installing this target Gem"
    FULL_DOCS [[
        Property which is set on targets that should generate a "RUN"
        target when installed. This \"RUN\" target helps to run the
        binary from the installed location directly from the IDE.
    ]]
)

# We can have elements being installed under the following components:
# - Core (required for all) (default)
# - Default
#   - Default_$<CONFIG>
# - Monolithic
#   - Monolithic_$<CONFIG>
# Debug/Monolithic are build permutations, so for a CMake run, it can only generate
# one of the permutations. Each build permutation can generate only one cmake_install.cmake.
# Each build permutation will generate the same elements in Core.
# CPack is able to put the two together by taking Core from one permutation and then taking
# each permutation.

ly_set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME CORE)
if(LY_MONOLITHIC_GAME)
    set(LY_BUILD_PERMUTATION Monolithic)
else()
    set(LY_BUILD_PERMUTATION Default)
endif()
string(TOUPPER ${LY_BUILD_PERMUTATION} LY_INSTALL_PERMUTATION_COMPONENT)

cmake_path(RELATIVE_PATH CMAKE_RUNTIME_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE runtime_output_directory)
cmake_path(RELATIVE_PATH CMAKE_LIBRARY_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE library_output_directory)
# Get the output folders, archive is always the same, but runtime/library can be in subfolders defined per target
cmake_path(RELATIVE_PATH CMAKE_ARCHIVE_OUTPUT_DIRECTORY BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE archive_output_directory)

cmake_path(APPEND archive_output_directory "${PAL_PLATFORM_NAME}/$<CONFIG>/${LY_BUILD_PERMUTATION}")
cmake_path(APPEND library_output_directory "${PAL_PLATFORM_NAME}/$<CONFIG>/${LY_BUILD_PERMUTATION}")
cmake_path(APPEND runtime_output_directory "${PAL_PLATFORM_NAME}/$<CONFIG>/${LY_BUILD_PERMUTATION}")

#! ly_setup_target: Setup the data needed to re-create the cmake target commands for a single target
function(ly_setup_target OUTPUT_CONFIGURED_TARGET ALIAS_TARGET_NAME absolute_target_source_dir)
    # De-alias target name
    ly_de_alias_target(${ALIAS_TARGET_NAME} TARGET_NAME)

    # Get the target source directory relative to the LY root folder
    ly_get_engine_relative_source_dir(${absolute_target_source_dir} relative_target_source_dir)

    # All include directories marked PUBLIC or INTERFACE will be installed. We dont use PUBLIC_HEADER because in order to do that
    # we need to set the PUBLIC_HEADER property of the target for all the headers we are exporting. After doing that, installing the
    # headers end up in one folder instead of duplicating the folder structure of the public/interface include directory.
    # Instead, we install them with install(DIRECTORY)
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

                # For some cases (e.g. codegen) we generate headers that end up in the BUILD_DIR. Since the BUILD_DIR
                # is per-permutation, we need to install such headers per permutation. For the other cases, we can install
                # under the default component since they are shared across permutations/configs.
                cmake_path(IS_PREFIX CMAKE_BINARY_DIR ${include_directory} NORMALIZE include_directory_child_of_build)

                # In order to combine profile and release monolithic, we use the CORE component
                # because CPack will fail if it finds duplicated content in CORE and DEFAULT/MONOLITHIC
                set(include_directory_component ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})

                unset(rel_include_dir)
                if(include_directory_child_of_build)
                    # We need to use the path relative to the binary folder otherwise you will get an invalid 
                    # relative path if the build folder is outside the engine root.
                    cmake_path(RELATIVE_PATH include_directory BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE rel_include_dir)
                else()
                    cmake_path(RELATIVE_PATH include_directory BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE rel_include_dir)
                endif()
                cmake_path(APPEND rel_include_dir "..")
                cmake_path(NORMAL_PATH rel_include_dir OUTPUT_VARIABLE destination_dir)

                ly_install(DIRECTORY ${include_directory}
                    DESTINATION ${destination_dir}
                    COMPONENT ${include_directory_component}
                    FILES_MATCHING
                        PATTERN *.h
                        PATTERN *.hpp
                        PATTERN *.inl
                        PATTERN *.hxx
                        PATTERN *.jinja # LyAutoGen files
                )
            endif()
        endforeach()
    endif()

    get_target_property(target_runtime_output_directory ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if(target_runtime_output_directory)
        cmake_path(RELATIVE_PATH target_runtime_output_directory BASE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} OUTPUT_VARIABLE target_runtime_output_subdirectory)
    endif()

    get_target_property(target_library_output_directory ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)
    if(target_library_output_directory)
        cmake_path(RELATIVE_PATH target_library_output_directory BASE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} OUTPUT_VARIABLE target_library_output_subdirectory)
    endif()

    if(COMMAND ly_setup_target_install_targets_override)
        # Mac needs special handling because of a cmake issue
        ly_setup_target_install_targets_override(TARGET ${TARGET_NAME}
            ARCHIVE_DIR ${archive_output_directory}
            LIBRARY_DIR ${library_output_directory}
            RUNTIME_DIR ${runtime_output_directory}
            LIBRARY_SUBDIR ${target_library_output_subdirectory}
            RUNTIME_SUBDIR ${target_runtime_output_subdirectory}
        )
    else()
        foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER ${conf} UCONF)
            ly_install(TARGETS ${TARGET_NAME}
                ARCHIVE
                    DESTINATION ${archive_output_directory}
                    COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                    CONFIGURATIONS ${conf}
                LIBRARY
                    DESTINATION ${library_output_directory}/${target_library_output_subdirectory}
                    COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                    CONFIGURATIONS ${conf}
                RUNTIME
                    DESTINATION ${runtime_output_directory}/${target_runtime_output_subdirectory}
                    COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                    CONFIGURATIONS ${conf}
            )
        endforeach()
    endif()

    # CMakeLists.txt related files
    string(REGEX MATCH "(.*)::(.*)$" match ${ALIAS_TARGET_NAME})
    if(match)
        set(NAMESPACE_PLACEHOLDER "NAMESPACE ${CMAKE_MATCH_1}")
        set(NAME_PLACEHOLDER ${CMAKE_MATCH_2})
    else()
        set(NAMESPACE_PLACEHOLDER "")
        set(NAME_PLACEHOLDER ${TARGET_NAME})
    endif()
    get_target_property(should_create_helper ${TARGET_NAME} LY_INSTALL_GENERATE_RUN_TARGET)
    if(should_create_helper)
        set(NAME_PLACEHOLDER ${NAME_PLACEHOLDER}.Imported)
    endif()

    set(TARGET_TYPE_PLACEHOLDER "")
    get_target_property(target_type ${TARGET_NAME} TYPE)
    # Remove the _LIBRARY since we dont need to pass that to ly_add_targets
    string(REPLACE "_LIBRARY" "" TARGET_TYPE_PLACEHOLDER ${target_type})
    # For HEADER_ONLY libs we end up generating "INTERFACE" libraries, need to specify HEADERONLY instead
    string(REPLACE "INTERFACE" "HEADERONLY" TARGET_TYPE_PLACEHOLDER ${TARGET_TYPE_PLACEHOLDER})
    # In non-monolithic mode, gem targets are MODULE libraries, In monolithic mode gem targets are STATIC libraries
    set(GEM_LIBRARY_TYPES "MODULE" "STATIC")
    if(TARGET_TYPE_PLACEHOLDER IN_LIST GEM_LIBRARY_TYPES)
        get_target_property(gem_module ${TARGET_NAME} GEM_MODULE)
        if(gem_module)
            string(PREPEND TARGET_TYPE_PLACEHOLDER "GEM_")
        endif()
    endif()

    string(REPEAT " " 12 PLACEHOLDER_INDENT)
    get_target_property(COMPILE_DEFINITIONS_PLACEHOLDER ${TARGET_NAME} INTERFACE_COMPILE_DEFINITIONS)
    if(COMPILE_DEFINITIONS_PLACEHOLDER)
        set(COMPILE_DEFINITIONS_PLACEHOLDER "${PLACEHOLDER_INDENT}${COMPILE_DEFINITIONS_PLACEHOLDER}")
        list(JOIN COMPILE_DEFINITIONS_PLACEHOLDER "\n${PLACEHOLDER_INDENT}" COMPILE_DEFINITIONS_PLACEHOLDER)
    else()
        unset(COMPILE_DEFINITIONS_PLACEHOLDER)
    endif()

    # Includes need additional processing to add the install root
    foreach(include IN LISTS include_directories)
        string(GENEX_STRIP ${include} include_genex_expr)
        if(include_genex_expr STREQUAL include) # only for cases where there are no generation expressions
            # Make the include path relative to the source dir where the target will be declared
            cmake_path(IS_PREFIX CMAKE_BINARY_DIR ${include} NORMALIZE include_directory_child_of_build)
            if(include_directory_child_of_build)
                # Some autogen files are placed in the build folder so remove the build folder prefix
                # and use it to calculate the relative path  
                cmake_path(RELATIVE_PATH include BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE rel_include)
                cmake_path(SET base_path ${LY_ROOT_FOLDER})
                cmake_path(APPEND base_path ${rel_include} OUTPUT_VARIABLE absolute_include)
                cmake_path(RELATIVE_PATH absolute_include BASE_DIRECTORY ${absolute_target_source_dir} OUTPUT_VARIABLE target_include)
            else()
                cmake_path(RELATIVE_PATH include BASE_DIRECTORY ${absolute_target_source_dir} OUTPUT_VARIABLE target_include)
            endif()
            list(APPEND INCLUDE_DIRECTORIES_PLACEHOLDER "${PLACEHOLDER_INDENT}${target_include}")
        endif()
    endforeach()
    list(JOIN INCLUDE_DIRECTORIES_PLACEHOLDER "\n" INCLUDE_DIRECTORIES_PLACEHOLDER)

    string(REPEAT " " 12 PLACEHOLDER_INDENT)
    get_property(interface_build_dependencies_props TARGET ${TARGET_NAME} PROPERTY LY_DELAYED_LINK)
    unset(INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER)
    # We can have private build dependencies that contains direct or indirect runtime dependencies.
    # Since imported targets cannot contain build dependencies, we need another way to propagate the runtime dependencies.
    # We dont want to put such dependencies in the interface because a user can mistakenly use a symbol that is not available
    # when using the engine from source (and that the author of the target didn't want to set public).
    # To overcome this, we will actually expose the private build dependencies as runtime dependencies. Our runtime dependency
    # algorithm will walk recursively also through static libraries and will only copy binaries to the output.
    unset(RUNTIME_DEPENDENCIES_PLACEHOLDER)
    if(interface_build_dependencies_props)
        cmake_parse_arguments(build_deps "" "" "PRIVATE;PUBLIC;INTERFACE" ${interface_build_dependencies_props})
        # Interface and public dependencies should always be exposed
        set(build_deps_target ${build_deps_INTERFACE})
        if(build_deps_PUBLIC)
            set(build_deps_target "${build_deps_target};${build_deps_PUBLIC}")
        endif()
        # Private dependencies should only be exposed if it is a static library, since in those cases, link
        # dependencies are transfered to the downstream dependencies
        if("${target_type}" STREQUAL "STATIC_LIBRARY")
            set(build_deps_target "${build_deps_target};${build_deps_PRIVATE}")
        endif()

        # But we will also pass the private dependencies as runtime dependencies (as long as they are targets, note the comment above)
        foreach(build_dep_private IN LISTS build_deps_PRIVATE)
            if(TARGET ${build_dep_private})
                list(APPEND RUNTIME_DEPENDENCIES_PLACEHOLDER "${build_dep_private}")
            endif()
        endforeach()

        foreach(build_dependency IN LISTS build_deps_target)
            # Skip wrapping produced when targets are not created in the same directory
            if(build_dependency)
                list(APPEND INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER "${PLACEHOLDER_INDENT}${build_dependency}")
            endif()
        endforeach()
    endif()
    list(JOIN INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER "\n" INTERFACE_BUILD_DEPENDENCIES_PLACEHOLDER)

    string(REPEAT " " 8 PLACEHOLDER_INDENT)
    get_target_property(manually_added_dependencies ${TARGET_NAME} MANUALLY_ADDED_DEPENDENCIES)
    if(manually_added_dependencies) # not found properties return the name of the variable with a "-NOTFOUND" at the end, here we set it to empty if not found
        list(APPEND RUNTIME_DEPENDENCIES_PLACEHOLDER ${manually_added_dependencies})
    endif()
    if(RUNTIME_DEPENDENCIES_PLACEHOLDER)
        set(RUNTIME_DEPENDENCIES_PLACEHOLDER "${PLACEHOLDER_INDENT}${RUNTIME_DEPENDENCIES_PLACEHOLDER}")
        list(JOIN RUNTIME_DEPENDENCIES_PLACEHOLDER "\n${PLACEHOLDER_INDENT}" RUNTIME_DEPENDENCIES_PLACEHOLDER)
    else()
        unset(RUNTIME_DEPENDENCIES_PLACEHOLDER)
    endif()

    string(REPEAT " " 8 PLACEHOLDER_INDENT)
    # If a target has an LY_PROJECT_NAME property, forward that property to new target
    get_target_property(target_project_association ${TARGET_NAME} LY_PROJECT_NAME)
    if(target_project_association)
        list(APPEND TARGET_PROPERTIES_PLACEHOLDER "${PLACEHOLDER_INDENT}LY_PROJECT_NAME ${target_project_association}")
    endif()

    # If the target is an executable/application, add a custom target so we can debug the target in project-centric workflow
    if(should_create_helper)
        string(REPLACE ".Imported" "" RUN_TARGET_NAME ${NAME_PLACEHOLDER})
        set(target_types_with_debugging_helper EXECUTABLE APPLICATION)
        if(NOT target_type IN_LIST target_types_with_debugging_helper)
            message(FATAL_ERROR "Cannot generate a RUN target for ${TARGET_NAME}, type is ${target_type}")
        endif()
        set(TARGET_RUN_HELPER
"add_custom_target(${RUN_TARGET_NAME})
set_target_properties(${RUN_TARGET_NAME} PROPERTIES
    FOLDER \"O3DE_SDK\"
    VS_DEBUGGER_COMMAND \$<GENEX_EVAL:\$<TARGET_PROPERTY:${NAME_PLACEHOLDER},IMPORTED_LOCATION>>
    VS_DEBUGGER_COMMAND_ARGUMENTS \"--project-path=\${LY_DEFAULT_PROJECT_PATH}\"
)
set_property(GLOBAL APPEND PROPERTY LY_ALL_TARGETS ${RUN_TARGET_NAME})
"
)
    endif()

    # Config files
    set(target_file_contents "# Generated by O3DE install\n\n")
    if(NOT target_type STREQUAL INTERFACE_LIBRARY)

        unset(target_location)
        set(runtime_types EXECUTABLE APPLICATION)
        if(target_type IN_LIST runtime_types)
            set(target_location "\${LY_ROOT_FOLDER}/${runtime_output_directory}/${target_runtime_output_subdirectory}/$<TARGET_FILE_NAME:${TARGET_NAME}>")
        elseif(target_type STREQUAL MODULE_LIBRARY)
            set(target_location "\${LY_ROOT_FOLDER}/${library_output_directory}/${target_library_output_subdirectory}/$<TARGET_FILE_NAME:${TARGET_NAME}>")
        elseif(target_type STREQUAL SHARED_LIBRARY)
            string(APPEND target_file_contents
"set_property(TARGET ${NAME_PLACEHOLDER}
    APPEND_STRING PROPERTY IMPORTED_IMPLIB
        $<$<CONFIG:$<CONFIG>$<ANGLE-R>:\"\${LY_ROOT_FOLDER}/${archive_output_directory}/$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}>\"$<ANGLE-R>
)
")
            string(APPEND target_file_contents
"set_property(TARGET ${NAME_PLACEHOLDER}
    PROPERTY IMPORTED_IMPLIB_$<UPPER_CASE:$<CONFIG>>
        \"\${LY_ROOT_FOLDER}/${archive_output_directory}/$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}>\"
)
")
            set(target_location "\${LY_ROOT_FOLDER}/${library_output_directory}/${target_library_output_subdirectory}/$<TARGET_FILE_NAME:${TARGET_NAME}>")
        elseif(target_type STREQUAL STATIC_LIBRARY) # STATIC_LIBRARY, OBJECT_LIBRARY, INTERFACE_LIBRARY
            set(target_location "\${LY_ROOT_FOLDER}/${archive_output_directory}/$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}>")
        else() # OBJECT_LIBRARY has no output target
        endif()

        if(target_location)
            string(APPEND target_file_contents
"set_property(TARGET ${NAME_PLACEHOLDER}
    APPEND_STRING PROPERTY IMPORTED_LOCATION
        $<$<CONFIG:$<CONFIG>$<ANGLE-R>:${target_location}$<ANGLE-R>
)
set_property(TARGET ${NAME_PLACEHOLDER}
    PROPERTY IMPORTED_LOCATION_$<UPPER_CASE:$<CONFIG>>
        ${target_location}
)
")
        endif()
    endif()

    set(target_install_source_dir ${CMAKE_CURRENT_BINARY_DIR}/install/${relative_target_source_dir})
    file(GENERATE OUTPUT "${target_install_source_dir}/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/${NAME_PLACEHOLDER}_$<CONFIG>.cmake" CONTENT "${target_file_contents}")

    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(FILES "${target_install_source_dir}/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/${NAME_PLACEHOLDER}_${conf}.cmake"
            DESTINATION ${relative_target_source_dir}/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}
            COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
            CONFIGURATIONS ${conf}
        )
    endforeach()

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

#! ly_setup_3p_target: Export enough data such that the script-only mode knows about the runtime dependencies
#! for the given target.  This function returns a string which is a setup function that can be called
#! in script only mode to create a fake target for the 3p library that just includes its copy dependencies.
function(ly_setup_3p_target OUTVAR_GENERATED_DATA_FILENAME OUTVAR_FILE_NAME_TO_GENERATE ALIAS_TARGET_NAME)
    ly_de_alias_target(${ALIAS_TARGET_NAME} TARGET_NAME)
    o3de_get_dependencies_for_target(
            TARGET "${TARGET_NAME}"
            COPY_DEPENDENCIES_VAR target_copy_dependencies
            TARGET_DEPENDENCIES_VAR target_target_dependencies
        )

    # note that we want to actually create and register the aliased target name
    # but get the data from the de-aliased target.  So except for the call above
    # we use the aliased target name.
    string(REPLACE "::" "__" CLEAN_TARGET_NAME "${ALIAS_TARGET_NAME}")  

    unset(copy_dependencies_target_rel_path)
    unset(copy_dependencies_source_file_path)
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        unset(copy_dependencies_${conf})
    endforeach()

    foreach(dependency_for_target IN LISTS target_target_dependencies)
        if (NOT TARGET ${dependency_for_target})
            # Sometimes, things set actual shared libraries as a target dependency instead of a copy dependency.
            list(APPEND target_copy_dependencies "${dependency_for_target}\n")
        endif()
    endforeach()     
    
    # note that the copy dependencies are actually in pairs newline delimited, so
    # file_to_copy_a\nwhere_to_copy_a;file_to_copy_b\nwhere_to_copy_b;...
    foreach(copy_dependency IN LISTS target_copy_dependencies)
        string(REPLACE "\n" ";" copy_dep_pair "${copy_dependency}")
        list(POP_FRONT copy_dep_pair source_filepath destination_relpath)
        list(APPEND copy_dependencies_source_file_path "${source_filepath}")
        # a quirk of list append is that if you give it an empty element, it does nothing.
        # use dummy element (0) to indicate its an empty path
        if (NOT destination_relpath)
            set(destination_relpath "0")
        endif()

        list(APPEND copy_dependencies_target_rel_path "${destination_relpath}")
    endforeach()

    # sanity check
    if (copy_dependencies_source_file_path OR copy_dependencies_target_rel_path)
        list(LENGTH copy_dependencies_source_file_path source_list_length)
        list(LENGTH copy_dependencies_target_rel_path  relpath_list_lenth)
        if (NOT source_list_length EQUAL relpath_list_lenth)
            message(FATAL_ERROR "Programmer error - for target ${ALIAS_TARGET_NAME}, lists are supposed to be same length, but\n\
                                ${source_list_length} source and ${relpath_list_lenth} relpath\n\
                                source:'${copy_dependencies_source_file_path}'\n\
                                relpath: '${copy_dependencies_target_rel_path}'")
        endif()
    endif()

    unset(${OUTVAR_GENERATED_DATA_FILENAME} PARENT_SCOPE)

    # the absolute source paths of files to copy can have generator expressions that are per-config.
    # use file(GENERATE ...) to evaluate the genexes, by writing it as a build file
    # since GENERATE only happens during generate step, this is as far as we can go during
    # configure.  It will be up to a build step to read these generated files back and
    # do a final transform that removes things like absolute paths.
    # note that we generate a file for every 3p target, as they must exist.
    set(generate_content "set(target_name ${ALIAS_TARGET_NAME})\n")
    if (copy_dependencies_source_file_path)
        string(APPEND generate_content "set(copy_dependencies_source_file_path \"${copy_dependencies_source_file_path}\")\n")
        string(APPEND generate_content "set(copy_dependencies_target_rel_path \"${copy_dependencies_target_rel_path}\")\n")
    endif()

    file(GENERATE OUTPUT  "${CMAKE_BINARY_DIR}/install/$<CONFIG>/${PAL_PLATFORM_NAME}/${CLEAN_TARGET_NAME}.cmake" 
                  CONTENT "${generate_content}")
    set(${OUTVAR_GENERATED_DATA_FILENAME} "${CMAKE_BINARY_DIR}/install/$<CONFIG>/${PAL_PLATFORM_NAME}/${CLEAN_TARGET_NAME}.cmake" PARENT_SCOPE)
    set(${OUTVAR_FILE_NAME_TO_GENERATE} ${CLEAN_TARGET_NAME} PARENT_SCOPE)

endfunction()

# In "Script only mode", the 3p libraries are not downloaded at all.
# But some of them (not all) have extraneous .so or .dll files that need to be deployed
# into the final runtime package if you intend to ship a script-only runtime.
# This function exports a file that is used only in script-only mode to supply that list
# of extra DLLs to copy along with the game.
function(ly_setup_3p_dependencies)

    # 3p dependency generation is only relevant to installer builds, which is expected to happen only if no project is being built
    # prevent this code from running in non-instlaler-builds, otherwise it will try to hang these commands off a non-existant generic
    # game launcher target (which itself is also only relevant to installer builds for script-only mode.)
    if (LY_PROJECTS)
        return()
    endif()

    unset(list_of_files_to_process)
    unset(expected_output_files)
    set(final_3p_output_dir "${CMAKE_BINARY_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}")
    get_property(o3de_all_3rdparty_targets GLOBAL PROPERTY O3DE_ALL_3RDPARTY_TARGETS)

    # call the ly_setup_3p_target for each 3p target.  It will create a GENERATE step
    # that will output a file, and also return the file name of the generated file.
    foreach(target_3p IN LISTS o3de_all_3rdparty_targets)
        ly_setup_3p_target(file_to_process output_filename ${target_3p})
        if (file_to_process)
            list(APPEND list_of_files_to_process ${file_to_process})
            list(APPEND expected_output_files ${final_3p_output_dir}/${output_filename}.cmake)
        endif()
    endforeach()
    
    # note that generate only occurs during the generate step.  The above file will not be available until the build step
    # so a finalization build step needs to run a script to finalize it.
    if (list_of_files_to_process)
        add_custom_command(
            COMMAND ${CMAKE_COMMAND} -P 
                "${LY_ROOT_FOLDER}/cmake/3rdParty/script-only-mode/PostProcessScriptOnlyMappings.cmake" # Script to run
                "${final_3p_output_dir}" # the output file to make
                ${list_of_files_to_process}
            OUTPUT ${expected_output_files}
            COMMENT "Updating script-only mode 3rd Party library mappings..."
            DEPENDS ${list_of_files_to_process} 
                    "${LY_ROOT_FOLDER}/cmake/3rdParty/script-only-mode/PostProcessScriptOnlyMappings.cmake"
            VERBATIM
        )

        # make a custom target that depends on the above output file.
        add_custom_target(GenerateScriptOnlyMappings DEPENDS ${expected_output_files})

        # make it so that at least one real target that actually gets built depends on the above target.
        # or else cmake won't evaluate it since nothing hangs off it.
        add_dependencies(O3DE.GameLauncher GenerateScriptOnlyMappings)

        ly_install(FILES ${expected_output_files}
            DESTINATION "cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}"
            COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT})
    endif()
endfunction()


#! ly_setup_subdirectory: setup all targets in the subdirectory
function(ly_setup_subdirectory absolute_target_source_dir)
    ly_get_engine_relative_source_dir(${absolute_target_source_dir} relative_target_source_dir)

    # The builtin BUILDSYSTEM_TARGETS property isn't being used here as that returns the de-alised
    # TARGET and we need the alias namespace for recreating the CMakeLists.txt in the install layout
    get_property(ALIAS_TARGETS_NAME DIRECTORY ${absolute_target_source_dir} PROPERTY LY_DIRECTORY_TARGETS)
    foreach(ALIAS_TARGET_NAME IN LISTS ALIAS_TARGETS_NAME)
        ly_setup_target(configured_target ${ALIAS_TARGET_NAME} ${absolute_target_source_dir})
        string(APPEND all_configured_targets "${configured_target}")
    endforeach()

    # Initialize the target install source directory to path underneath the current binary directory
    set(target_install_source_dir "${CMAKE_CURRENT_BINARY_DIR}/install/${relative_target_source_dir}")

    ly_file_read(${LY_ROOT_FOLDER}/cmake/install/Copyright.in cmake_copyright_comment)

    # 1. Create the base CMakeLists.txt that will just include a cmake file per platform
    string(CONFIGURE [[
@cmake_copyright_comment@
if(LY_MONOLITHIC_GAME)
    include(Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake OPTIONAL)
else()
    include(Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
endif()
]] subdirectory_cmakelist_content @ONLY)

    # Store off the generated CMakeLists.txt into a DIRECTORY property based on the subdirectory being visited
    # In the ly_setup_assets() function, it generates an empty CMakeLists.txt into the gem root directory
    # if one does not exist by checking if this property is set
    set_property(DIRECTORY "${absolute_target_source_dir}" APPEND_STRING PROPERTY O3DE_SUBDIRECTORY_CMAKELIST_CONTENT "${subdirectory_cmakelist_content}")
    file(WRITE "${target_install_source_dir}/CMakeLists.txt" "${subdirectory_cmakelist_content}")

    ly_install(FILES "${target_install_source_dir}/CMakeLists.txt"
        DESTINATION ${relative_target_source_dir}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    # 2. For this platform file, create a Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake file
    #    that will include different configuration permutations (e.g. monolithic vs non-monolithic)
    file(CONFIGURE OUTPUT "${target_install_source_dir}/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake" CONTENT [[
@cmake_copyright_comment@
if(LY_MONOLITHIC_GAME)
    include(Platform/${PAL_PLATFORM_NAME}/Monolithic/permutation.cmake OPTIONAL)
else()
    include(Platform/${PAL_PLATFORM_NAME}/Default/permutation.cmake)
endif()
]])
    ly_install(FILES "${target_install_source_dir}/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake"
        DESTINATION ${relative_target_source_dir}/Platform/${PAL_PLATFORM_NAME}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    # 3. For this configuration permutation, generate a Platform/${PAL_PLATFORM_NAME}/${permutation}/permutation.cmake
    #    that will declare the target and configure it
    ly_setup_subdirectory_create_alias("${absolute_target_source_dir}" CREATE_ALIASES_PLACEHOLDER)
    ly_setup_subdirectory_set_gem_variant_to_load("${absolute_target_source_dir}" GEM_VARIANT_TO_LOAD_PLACEHOLDER)
    ly_setup_add_variant_dependencies_for_gem_dependencies("${absolute_target_source_dir}" O3DE_ADD_VARIANT_DEPS_FOR_GEM_DEPS)
    ly_setup_subdirectory_enable_gems("${absolute_target_source_dir}" ENABLE_GEMS_PLACEHOLDER)
    ly_setup_subdirectory_install_code("${absolute_target_source_dir}" O3DE_INSTALL_CODE_PLACEHOLDER)

    # Write out all the aggregated ly_add_target function calls and the final ly_create_alias() calls to the target CMakeLists.txt
    file(WRITE "${target_install_source_dir}/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/permutation.cmake"
        "${cmake_copyright_comment}"
        "${O3DE_INSTALL_CODE_PLACEHOLDER}"
        "${all_configured_targets}"
        "\n"
        "${CREATE_ALIASES_PLACEHOLDER}"
        "${GEM_VARIANT_TO_LOAD_PLACEHOLDER}"
        "${O3DE_ADD_VARIANT_DEPS_FOR_GEM_DEPS}"
        "${ENABLE_GEMS_PLACEHOLDER}"
    )

    ly_install(FILES "${target_install_source_dir}/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/permutation.cmake"
        DESTINATION ${relative_target_source_dir}/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}
        COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}
    )

endfunction()

#! ly_setup_cmake_install: install the "cmake" folder
function(ly_setup_cmake_install)

    ly_install(DIRECTORY "${LY_ROOT_FOLDER}/cmake"
        DESTINATION .
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
        PATTERN "__pycache__" EXCLUDE
        PATTERN "Findo3de.cmake" EXCLUDE
        PATTERN "cmake/ConfigurationTypes.cmake" EXCLUDE
        REGEX "3rdParty/Platform\/.*\/BuiltInPackages_.*\.cmake" EXCLUDE
    )

    # Connect configuration types
    ly_install(FILES "${LY_ROOT_FOLDER}/cmake/install/ConfigurationTypes.cmake"
        DESTINATION cmake
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    # generate each ConfigurationType_<CONFIG>.cmake file and install it under that configuration
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        configure_file("${LY_ROOT_FOLDER}/cmake/install/ConfigurationType_config.cmake.in"
            "${CMAKE_BINARY_DIR}/cmake/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/ConfigurationTypes_${conf}.cmake"
            @ONLY
        )
        ly_install(FILES "${CMAKE_BINARY_DIR}/cmake/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/ConfigurationTypes_${conf}.cmake"
            DESTINATION cmake/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}
            COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
            CONFIGURATIONS ${conf}
        )
    endforeach()

    # Transform the list of all external subdirectories used by the engine + projects into a json array
    set(indent "        ")

    get_external_subdirectories_in_use(external_subdirs)
    foreach(external_subdir ${external_subdirs})
        # If an external subdirectory is not a subdirectory of the engine root, then
        # prepend "External" to its subdirectory root
        ly_get_root_subdirectory_which_is_parent(${external_subdir} root_subdir_of_external_subdir)
        cmake_path(RELATIVE_PATH external_subdir BASE_DIRECTORY ${root_subdir_of_external_subdir} OUTPUT_VARIABLE engine_rel_external_subdir)

        cmake_path(IS_PREFIX LY_ROOT_FOLDER ${external_subdir} is_subdirectory_of_engine)
        if(NOT is_subdirectory_of_engine)
            cmake_path(GET root_subdir_of_external_subdir FILENAME root_subdir_dirname)
            set(relative_subdir ${engine_rel_external_subdir})
            unset(engine_rel_external_subdir)
            cmake_path(APPEND engine_rel_external_subdir "External" ${root_subdir_dirname} ${relative_subdir})
        endif()

        set(quoted_engine_rel_external_subdir "\"${engine_rel_external_subdir}\"")
        if (quoted_engine_rel_external_subdir IN_LIST relative_external_subdirs)
            message(WARNING "An external subdirectory \"${external_subdir}\" has been found twice when generating the engine.json for the install layout")
        else()
            list(APPEND relative_external_subdirs "\"${engine_rel_external_subdir}\"")
        endif()
    endforeach()
    # Sort the external subdirectories before joining them with commas
    list(SORT relative_external_subdirs CASE INSENSITIVE)
    list(JOIN relative_external_subdirs ",\n${indent}" O3DE_INSTALL_EXTERNAL_SUBDIRS)

    # Use the cache list of "gem_names" from the engine.json to populate
    # the generated engine.json file
    # The O3DE_INSTALL_ENGINE_GEMS is the configure placeholder that needs to be set
    # at the end
    get_property(active_engine_gems GLOBAL PROPERTY "O3DE_EXPLICIT_ACTIVE_GEMS_ENGINE")
    if (active_engine_gems)
        foreach(active_engine_gem IN LISTS active_engine_gems)
            list(APPEND quoted_active_engine_gems "\"${active_engine_gem}\"")
        endforeach()
        list(SORT quoted_active_engine_gems CASE INSENSITIVE)
        list(JOIN quoted_active_engine_gems ",\n${indent}" O3DE_INSTALL_ENGINE_GEMS)
    endif()

    # Read the "templates" key from the source engine.json
    o3de_read_json_array(engine_templates ${LY_ROOT_FOLDER}/engine.json "templates")
    if(engine_templates)
        foreach(template_path IN LISTS engine_templates)
            list(APPEND relative_templates "\"${template_path}\"")
        endforeach()
        list(SORT relative_templates CASE INSENSITIVE)
        list(JOIN relative_templates ",\n${indent}" O3DE_INSTALL_TEMPLATES)
    endif()

    # Read the "repos" key from the source engine.json
    o3de_read_json_array(engine_repos ${LY_ROOT_FOLDER}/engine.json "repos")
    if(engine_repos)
        foreach(repo_uri ${engine_repos})
            list(APPEND repos "\"${repo_uri}\"")
        endforeach()
        list(SORT repos CASE INSENSITIVE)
        list(JOIN repos ",\n${indent}" O3DE_INSTALL_REPOS)
    endif()

    # Read the "api_versions" key from the source engine.json
    o3de_read_json_key(O3DE_INSTALL_API_VERSIONS ${LY_ROOT_FOLDER}/engine.json "api_versions")

    configure_file(${LY_ROOT_FOLDER}/cmake/install/engine.json.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/engine.json @ONLY)

    ly_install(FILES
            "${LY_ROOT_FOLDER}/CMakeLists.txt"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake/engine.json"
        DESTINATION .
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    # Collect all Find files that were added with ly_add_external_target_path
    unset(additional_find_files)
    unset(additional_platform_files)
    get_property(additional_module_paths GLOBAL PROPERTY LY_ADDITIONAL_MODULE_PATH)
    foreach(additional_module_path ${additional_module_paths})
        unset(find_files)
        file(GLOB find_files "${additional_module_path}/Find*.cmake")
        list(APPEND additional_find_files "${find_files}")
        foreach(find_file ${find_files})
            # also copy the Platform/<current_platform> to the destination
            cmake_path(GET find_file PARENT_PATH find_file_parent)
            unset(plat_files)
            file(GLOB plat_files "${find_file_parent}/Platform/${PAL_PLATFORM_NAME}/*.cmake")
            list(APPEND additional_platform_files "${plat_files}")
        endforeach()
    endforeach()

    ly_install(FILES ${additional_find_files}
        DESTINATION cmake/3rdParty
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )
    ly_install(FILES ${additional_platform_files}
        DESTINATION cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    # Findo3de.cmake file: we generate a different Findo3de.cmake file than the one we have in the source dir.
    configure_file(${LY_ROOT_FOLDER}/cmake/install/Findo3de.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/Findo3de.cmake @ONLY)
    ly_install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/Findo3de.cmake"
        DESTINATION cmake
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    unset(find_subdirectories)
    # Add to find_subdirectories all directories in which ly_add_target were called in
    get_property(all_subdirectories GLOBAL PROPERTY LY_ALL_TARGET_DIRECTORIES)
    foreach(target_subdirectory IN LISTS all_subdirectories)
        # If the subdirectory is an external_subdirectory, then it is being added in the engine.json file
        # Therefore it is not added to the o3de_subdirectories_<Platform>.cmake file to avoid
        # invoking add_subdirectory twice
        get_property(is_external_subdir GLOBAL PROPERTY "O3DE_SUBDIRECTORY_${target_subdirectory}" SET)
        if(is_external_subdir)
            continue()
        endif()
        ly_get_root_subdirectory_which_is_parent(${target_subdirectory} root_subdir_of_target)
        cmake_path(RELATIVE_PATH target_subdirectory BASE_DIRECTORY ${root_subdir_of_target} OUTPUT_VARIABLE relative_target_subdirectory)

        cmake_path(IS_PREFIX LY_ROOT_FOLDER ${target_subdirectory} is_subdirectory_of_engine)
        if(NOT is_subdirectory_of_engine)
            cmake_path(GET root_subdir_of_target FILENAME root_subdir_dirname)
            set(relative_subdir ${relative_target_subdirectory})
            unset(relative_target_subdirectory)
            cmake_path(APPEND relative_target_subdirectory "External" ${root_subdir_dirname} ${relative_subdir})
        endif()

        string(APPEND find_subdirectories "add_subdirectory(${relative_target_subdirectory})\n")
    endforeach()
    set(permutation_find_subdirectories ${CMAKE_CURRENT_BINARY_DIR}/cmake/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/o3de_subdirectories_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
    file(GENERATE OUTPUT ${permutation_find_subdirectories}
        CONTENT
"# Generated by O3DE install\n
${find_subdirectories}
"
    )
    ly_install(FILES "${permutation_find_subdirectories}"
        DESTINATION cmake/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}
        COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}
    )

    set(pal_builtin_file ${CMAKE_CURRENT_BINARY_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}${LY_ARCHITECTURE_NAME_EXTENSION}.cmake)
    file(GENERATE OUTPUT ${pal_builtin_file}
        CONTENT
"# Generated by O3DE install\n
if(LY_MONOLITHIC_GAME)
    include(cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/Monolithic/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}${LY_ARCHITECTURE_NAME_EXTENSION}.cmake)
else()
    include(cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/Default/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}${LY_ARCHITECTURE_NAME_EXTENSION}.cmake)
endif()
"
    )
    ly_install(FILES "${pal_builtin_file}"
        DESTINATION cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    # ${LY_BUILD_PERMUTATION}/BuiltInPackage_<platform>.cmake: since associations could happen in any cmake file across the engine. We collect
    # all the associations in ly_associate_package and then generate them into BuiltInPackages_<platform>.cmake. This will consolidate all
    # associations in one file
    # Associations are sensitive to platform and build permutation, so we make different files for each.
    get_property(all_package_names GLOBAL PROPERTY LY_PACKAGE_NAMES)
    list(REMOVE_DUPLICATES all_package_names)
    set(builtinpackages "# Generated by O3DE install\n\n")
    foreach(package_name IN LISTS all_package_names)
        get_property(package_hash GLOBAL PROPERTY LY_PACKAGE_HASH_${package_name})
        get_property(targets GLOBAL PROPERTY LY_PACKAGE_TARGETS_${package_name})
        list(REMOVE_DUPLICATES targets)
        string(APPEND builtinpackages "ly_associate_package(PACKAGE_NAME ${package_name} TARGETS ${targets} PACKAGE_HASH ${package_hash})\n")
    endforeach()

    # Allow the BuiltinPackages_<platform>.cmake script to propagate additional cmake code to be executed in the Install layout configure step
    get_property(builtin_packages_inject_code GLOBAL PROPERTY O3DE_BUILTIN_PACKAGES_INSTALL_CODE)
    if (builtin_packages_inject_code)
        string(APPEND builtinpackages "${builtin_packages_inject_code}\n")
    endif()

    set(permutation_builtin_file ${CMAKE_CURRENT_BINARY_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}${LY_ARCHITECTURE_NAME_EXTENSION}.cmake)
    file(GENERATE OUTPUT ${permutation_builtin_file}
        CONTENT "${builtinpackages}"
    )
    ly_install(FILES "${permutation_builtin_file}"
        DESTINATION cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}
        COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}
    )

endfunction()

#! ly_setup_runtime_dependencies: install runtime dependencies
function(ly_setup_runtime_dependencies)

    # Common functions used by the below code
    if(COMMAND ly_setup_runtime_dependencies_copy_function_override)
        ly_setup_runtime_dependencies_copy_function_override()
    else()
        # despite this copy function being the same, we need to install it per component that uses it
        # (which is per-configuration per-permutation component)

        # The template is needed to replace the @LY_COPY_PERMISSIONS@ variable with the permissions
        # that should be used for the copied file
        set(install_runtime_ly_copy_template [[
        function(ly_copy source_files relative_target_directory)
            set(options)
            set(oneValueArgs TARGET_FILE_DIR SOURCE_TYPE SOURCE_GEM_MODULE)
            set(multiValueArgs)
            cmake_parse_arguments("${CMAKE_CURRENT_FUNCTION}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
            set(target_file_dir "${${CMAKE_CURRENT_FUNCTION}_TARGET_FILE_DIR}")
            set(source_type "${${CMAKE_CURRENT_FUNCTION}_SOURCE_TYPE}")
            set(source_is_gem "${${CMAKE_CURRENT_FUNCTION}_SOURCE_GEM_MODULE}")

            # Create the full path to the target directory
            cmake_path(APPEND target_directory "${target_file_dir}" "${relative_target_directory}")
            foreach(source_file IN LISTS source_files)
                cmake_path(GET source_file FILENAME target_filename)
                cmake_path(APPEND full_target_directory "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}" "${target_directory}")
                cmake_path(APPEND target_file "${full_target_directory}" "${target_filename}")
                if("${source_file}" IS_NEWER_THAN "${target_file}")
                    message(STATUS "Copying ${source_file} to ${full_target_directory}...")
                    file(COPY "${source_file}" DESTINATION "${full_target_directory}" FILE_PERMISSIONS @LY_COPY_PERMISSIONS@ FOLLOW_SYMLINK_CHAIN)
                    file(TOUCH_NOCREATE "${target_file}")
                endif()
            endforeach()
        endfunction()
]])
        # replace the @...@ placeholders
        string(CONFIGURE "${install_runtime_ly_copy_template}" install_runtime_ly_copy @ONLY)
        foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER ${conf} UCONF)
            ly_install(CODE "${install_runtime_ly_copy}"
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
            )
        endforeach()
    endif()

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
        set(target_file_dir "${runtime_output_directory}/${target_runtime_output_subdirectory}")
        unset(target_copy_dependencies)
        unset(target_target_dependencies)
        unset(target_link_dependencies)
        unset(target_imported_dependencies)
        o3de_get_dependencies_for_target(
            TARGET "${target}"
            COPY_DEPENDENCIES_VAR target_copy_dependencies
            TARGET_DEPENDENCIES_VAR target_target_dependencies
            LINK_DEPENDENCIES_VAR target_link_dependencies
            IMPORTED_DEPENDENCIES_VAR target_imported_dependencies
        )
        foreach(dependency_for_target IN LISTS target_copy_dependencies target_target_dependencies
            target_link_dependencies target_imported_dependencies)
            unset(runtime_command)
            o3de_get_command_for_dependency(COMMAND_VAR runtime_command
                DEPENDENCY ${dependency_for_target})
            string(CONFIGURE "${runtime_command}" runtime_command @ONLY)
            list(APPEND runtime_commands ${runtime_command})
        endforeach()

    endforeach()

    list(REMOVE_DUPLICATES runtime_commands)
    list(JOIN runtime_commands "    " runtime_commands_str) # the spaces are just to see the right identation in the cmake_install.cmake file
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(CODE "${runtime_commands_str}"
            COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
        )
    endforeach()

endfunction()

#! ly_setup_assets: install asset directories required by the engine
function(ly_setup_assets)

    # Gem Source Assets and configuration files
    # Find all gem directories relative to the CMake Source Dir

    # this first loop real-path-izes all the paths to avoid doing so repeatedly.
    get_external_subdirectories_in_use(external_subdirs_non_realpath)
    foreach(gem_candidate_dir IN LISTS external_subdirs_non_realpath)
        file(REAL_PATH ${gem_candidate_dir} gem_candidate_dir BASE_DIRECTORY ${LY_ROOT_FOLDER})
        list(APPEND external_subdirs ${gem_candidate_dir})
    endforeach()

    # This next loop is to filter out transient and .gitignore'd folders that should be added to
    # the install layout from the root directory. Such as <external-subdirectory-root>/Cache.
    # This is also done to avoid globbing thousands of files in subdirectories that shouldn't
    # be processed.
    # It also filters out subdirectories that are themselves gem candidate directories so they are
    # not double-visited.
    
    foreach(gem_candidate_dir IN LISTS external_subdirs)
        unset(external_subdir_files)
        unset(external_subdir_files_tenative)

        # Don't recurse immediately in order to exclude transient source artifacts
        file(GLOB
            external_subdir_files_tenative
            LIST_DIRECTORIES TRUE
            "${gem_candidate_dir}/*"
        )
        # Exclude transient artifacts that shouldn't be copied to the install layout
        list(FILTER external_subdir_files_tenative EXCLUDE REGEX "/([Bb]uild|[Cc]ache|[Uu]ser)$")

        # Exclude folders that themselves are gem_candidate_dirs with their own gem.json
        foreach(check_file IN LISTS external_subdir_files_tenative)
            if(IS_DIRECTORY ${check_file})
                list(FIND external_subdirs ${check_file} is_gem_candidate_dir)
                if(is_gem_candidate_dir EQUAL -1)
                    # it is not already in the list, so it is unique to this gem folder.
                    list(APPEND external_subdir_files ${check_file})
                endif()
            else()
                list(APPEND external_subdir_files ${check_file})                
            endif()
        endforeach()

        # Storing a "mapping" of gem candidate directories, to external_subdirectory files using
        # a DIRECTORY property for the "value" and the GLOBAL property for the "key"
        set_property(DIRECTORY ${gem_candidate_dir} APPEND PROPERTY directory_filtered_asset_paths "${external_subdir_files}")
        set_property(GLOBAL APPEND PROPERTY global_gem_candidate_dirs_prop ${gem_candidate_dir})
    endforeach()

    # Iterate over each gem candidate directories and populate a directory property
    # containing the files to copy over
    get_property(gem_candidate_dirs GLOBAL PROPERTY global_gem_candidate_dirs_prop)
    foreach(gem_candidate_dir IN LISTS gem_candidate_dirs)
        get_property(filtered_asset_paths DIRECTORY ${gem_candidate_dir} PROPERTY directory_filtered_asset_paths)

        # Check if the gem is a subdirectory of the engine
        cmake_path(IS_PREFIX LY_ROOT_FOLDER ${gem_candidate_dir} is_gem_subdirectory_of_engine)

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
                list(FILTER gem_dir_paths INCLUDE REGEX "/(Assets|Registry|Config|Editor/Scripts)$")
                set_property(DIRECTORY ${gem_candidate_dir} APPEND PROPERTY gems_assets_paths ${gem_dir_paths})
            else()
                set(gem_file_paths ${filtered_asset_path})
            endif()

            # Gather files to copy over
            # Currently only the gem.json file is copied over
            list(FILTER gem_file_paths INCLUDE REGEX "/(gem.json|preview.png)$")
            set_property(DIRECTORY ${gem_candidate_dir} APPEND PROPERTY gems_assets_paths "${gem_file_paths}")
        endforeach()

        # gem directories and files to install
        get_property(gems_assets_paths DIRECTORY ${gem_candidate_dir} PROPERTY gems_assets_paths)
        foreach(gem_absolute_path IN LISTS gems_assets_paths)
            # If an external subdirectory is not a subdirectory of the engine root, then
            # prepend "External" to its subdirectory root
            ly_get_root_subdirectory_which_is_parent(${gem_candidate_dir} root_subdir_of_gem)
            cmake_path(RELATIVE_PATH gem_absolute_path BASE_DIRECTORY ${root_subdir_of_gem} OUTPUT_VARIABLE gem_install_dest_dir)

            if(NOT is_gem_subdirectory_of_engine)
                cmake_path(GET root_subdir_of_gem FILENAME root_subdir_dirname)
                set(relative_subdir ${gem_install_dest_dir})
                unset(gem_install_dest_dir)
                cmake_path(APPEND gem_install_dest_dir "External" ${root_subdir_dirname} ${relative_subdir})
            endif()

            cmake_path(GET gem_install_dest_dir PARENT_PATH gem_install_dest_dir)
            if (NOT gem_install_dest_dir)
                cmake_path(SET gem_install_dest_dir .)
            endif()

            if(IS_DIRECTORY ${gem_absolute_path})
                ly_install(DIRECTORY "${gem_absolute_path}"
                    DESTINATION ${gem_install_dest_dir}
                    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
                )
            elseif (EXISTS ${gem_absolute_path})
                # Special case for the gem.json file, generate an empty CMakeLists.txt with the gem.json file
                # if there is not already a CMakeLists.txy being installed to gem root directory
                cmake_path(GET gem_absolute_path FILENAME filename)
                cmake_path(COMPARE "${filename}" EQUAL "gem.json" is_gem_root)
                get_property(has_gem_root_cmakelist_content DIRECTORY "${gem_candidate_dir}" PROPERTY O3DE_SUBDIRECTORY_CMAKELIST_CONTENT SET)
                if (is_gem_root AND NOT has_gem_root_cmakelist_content)
                    ly_file_read(${LY_ROOT_FOLDER}/cmake/install/Copyright.in cmake_copyright_comment)
                    # Generate an empty CMakeLists.txt  inside the cmake binary directory
                    # to allow it to be installed next to the gem.json
                    set(gem_scratch_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/install/${gem_install_dest_dir}")

                    # 1. Create the base CMakeLists.txt that will just include a cmake file per platform
                    string(CONFIGURE [[
                    @cmake_copyright_comment@
                    o3de_read_json_key(GEM_TYPE ${CMAKE_CURRENT_SOURCE_DIR}/gem.json "type")
                    if (GEM_TYPE STREQUAL "Asset")
                        if(LY_MONOLITHIC_GAME)
                            include(Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake OPTIONAL)                    
                        else()
                            include(Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
                        endif()
                    endif()
                    ]] subdirectory_cmakelist_content @ONLY)
                    

                    # Store off the generated CMakeLists.txt into a DIRECTORY property based on the subdirectory being visited
                    # In the ly_setup_assets() function, it generates an empty CMakeLists.txt into the gem root directory
                    # if one does not exist by checking if this property is set
                    set_property(DIRECTORY "${absolute_target_source_dir}" APPEND_STRING PROPERTY O3DE_SUBDIRECTORY_CMAKELIST_CONTENT "${subdirectory_cmakelist_content}")
                    
                    # copy the empty CMakeList.txt into the gem root directory, to allow add_subdirectory
                    # calls to succeed on the gem root in the install layout
                    file(WRITE "${gem_scratch_binary_dir}/CMakeLists.txt" "${subdirectory_cmakelist_content}")

                    ly_install(FILES "${gem_scratch_binary_dir}/CMakeLists.txt"
                        DESTINATION ${gem_install_dest_dir}
                        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
                    )
                endif()

                ly_install(FILES ${gem_absolute_path}
                    DESTINATION ${gem_install_dest_dir}
                    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
                )
            endif()

        endforeach()

    endforeach()

endfunction()

#! ly_setup_add_variant_dependencies_for_gem_dependencies: Replicates the call to
#!  the `o3de_add_variant_dependencies_for_gem_dependencies` function
#! within the generated CMakeLists.txt in the same relative install layout directory
function(ly_setup_add_variant_dependencies_for_gem_dependencies absolute_target_source_dir output_script)
    # Replicate the create_alias() calls made in the SOURCE_DIR into the generated CMakeLists.txt
    string(JOIN "\n" add_variant_dependencies_for_gem_dependencies_template
        "o3de_add_variant_dependencies_for_gem_dependencies(@add_variant_dependencies_for_gem_dependencies_args@)"
        "")

    unset(${output_script} PARENT_SCOPE)
    get_property(add_variant_dependencies_for_gem_dependencies_args_list
        DIRECTORY ${absolute_target_source_dir}
        PROPERTY O3DE_ADD_VARIANT_DEPENDENCIES_FOR_GEM_DEPENDENCIES_ARGUMENTS)

    unset(add_variant_dependencies_for_gem_dependencies_calls)
    foreach(add_variant_dependencies_for_gem_dependencies_args IN LISTS add_variant_dependencies_for_gem_dependencies_args_list)
        string(CONFIGURE "${add_variant_dependencies_for_gem_dependencies_template}" add_variant_dependencies_for_gem_dependencies_command @ONLY)
        string(APPEND add_variant_dependencies_for_gem_dependencies_calls ${add_variant_dependencies_for_gem_dependencies_command})
    endforeach()
    set(${output_script} ${add_variant_dependencies_for_gem_dependencies_calls} PARENT_SCOPE)
endfunction()

#! ly_setup_subdirectory_create_alias: Replicates the call to the `ly_create_alias` function
#! within the generated CMakeLists.txt in the same relative install layout directory
function(ly_setup_subdirectory_create_alias absolute_target_source_dir output_script)
    # Replicate the create_alias() calls made in the SOURCE_DIR into the generated CMakeLists.txt
    string(JOIN "\n" create_alias_template
        "if(NOT TARGET @alias_name@)"
        "   ly_create_alias(@create_alias_args@)"
        "endif()"
        "")

    unset(${output_script} PARENT_SCOPE)
    get_property(create_alias_args_list DIRECTORY ${absolute_target_source_dir} PROPERTY LY_CREATE_ALIAS_ARGUMENTS)
    foreach(create_alias_args IN LISTS create_alias_args_list)
        # Create a list out of the comma separated arguments and store it into the same variable
        string(REPLACE "," ";" create_alias_args ${create_alias_args})
        # The first argument of the create alias argument list is the ALIAS NAME so pop it from the list
        # It is used to protect against registering the same alias twice
        list(POP_FRONT create_alias_args alias_name)
        string(CONFIGURE "${create_alias_template}" create_alias_command @ONLY)
        string(APPEND create_alias_calls ${create_alias_command})
    endforeach()
    set(${output_script} ${create_alias_calls} PARENT_SCOPE)
endfunction()

#! ly_setup_subdirectory_set_gem_variant_to_load: Replicates the call to the `ly_set_gem_variant_to_load` function
#! within the generated CMakeLists.txt in the same relative install layout directory
function(ly_setup_subdirectory_set_gem_variant_to_load absolute_target_source_dir output_script)
    # Replicate the ly_set_gem_variant_to_load() calls made in the SOURCE_DIR for into the generated CMakeLists.txt
    set(set_gem_variant_args_template "ly_set_gem_variant_to_load(@set_gem_variant_args@)\n")

    unset(${output_script} PARENT_SCOPE)
    get_property(set_gem_variant_args_lists DIRECTORY ${absolute_target_source_dir} PROPERTY LY_SET_GEM_VARIANT_TO_LOAD_ARGUMENTS)
    foreach(set_gem_variant_args IN LISTS set_gem_variant_args_lists)
        string(CONFIGURE "${set_gem_variant_args_template}" set_gem_variant_to_load_command @ONLY)
        string(APPEND set_gem_variant_calls ${set_gem_variant_to_load_command})
    endforeach()
    set(${output_script} ${set_gem_variant_calls} PARENT_SCOPE)
endfunction()

#! ly_setup_subdirectory_enable_gems: Replicates the call to the `ly_enable_gems` function
#! within the generated CMakeLists.txt in the same relative install layout directory
function(ly_setup_subdirectory_enable_gems absolute_target_source_dir output_script)
    # Replicate the ly_set_gem_variant_to_load() calls made in the SOURCE_DIR into the generated CMakeLists.txt
    set(enable_gems_template "ly_enable_gems(@enable_gems_args@)\n")

    unset(${output_script} PARENT_SCOPE)
    get_property(enable_gems_args_list DIRECTORY ${absolute_target_source_dir} PROPERTY LY_ENABLE_GEMS_ARGUMENTS)
    foreach(enable_gems_args IN LISTS enable_gems_args_list)
        string(CONFIGURE "${enable_gems_template}" enable_gems_command @ONLY)
        string(APPEND enable_gems_calls ${enable_gems_command})
    endforeach()
    set(${output_script} ${enable_gems_calls} PARENT_SCOPE)
endfunction()

#! ly_setup_subdirectory_install_code: Add the CMake code specified in the O3DE_SUBDIRECTORY_INSTALL_CODE
#!  DIRECTORY property to the beginning of the generated CMakeLists.txt in the same relative install layout diredctory
#! within the generated CMakeLists.txt in the same relative install layout directory
function(ly_setup_subdirectory_install_code absolute_target_source_dir output_script)
    unset(${output_script} PARENT_SCOPE)
    get_property(subdirectory_install_code DIRECTORY ${absolute_target_source_dir} PROPERTY O3DE_SUBDIRECTORY_INSTALL_CODE)
    set(${output_script} ${subdirectory_install_code} PARENT_SCOPE)
endfunction()

#! ly_setup_o3de_install: orchestrates the installation of the different parts. This is the entry point from the root CMakeLists.txt
function(ly_setup_o3de_install)

    ly_setup_subdirectories()
    ly_setup_cmake_install()
    ly_setup_runtime_dependencies()
    ly_setup_assets()
    ly_setup_3p_dependencies()

    # Misc
    ly_install(FILES
        ${LY_ROOT_FOLDER}/pytest.ini
        ${LY_ROOT_FOLDER}/LICENSE.txt
        ${LY_ROOT_FOLDER}/README.md
        ${LY_ROOT_FOLDER}/CMakePresets.json
        DESTINATION .
        COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")
        ly_install(FILES
            ${LY_ROOT_FOLDER}/Code/Tools/ProjectManager/Resources/o3de_desktop.svg
            DESTINATION .
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
        )
    endif()

    # Inject other build directories
    foreach(external_dir ${LY_INSTALL_EXTERNAL_BUILD_DIRS})
        ly_install(CODE
"set(LY_CORE_COMPONENT_ALREADY_INCLUDED TRUE)
include(${external_dir}/cmake_install.cmake)
set(LY_CORE_COMPONENT_ALREADY_INCLUDED FALSE)"
            ALL_COMPONENTS
        )
    endforeach()

    if(COMMAND ly_post_install_steps)
        ly_post_install_steps()
    endif()

endfunction()
