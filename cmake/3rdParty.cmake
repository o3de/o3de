#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! Register a directory containing <package>/CMakeLists.txt, Find<Package>.cmake, or package configs.
# With no argument, register the current source 3rdParty folder.
# Search paths are inherited by the caller descendants.
# Targets, once created, are shared by the whole build.
function(o3de_register_3rdparty_root)
    if(ARGC EQUAL 0)
        set(directory "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty")
    elseif(ARGC EQUAL 1 AND NOT "${ARGV0}" STREQUAL "")
        set(directory "${ARGV0}")
    else()
        message(FATAL_ERROR "o3de_register_3rdparty_root accepts zero arguments or one non-empty directory")
    endif()
    cmake_path(ABSOLUTE_PATH directory NORMALIZE)
    if(NOT IS_DIRECTORY "${directory}")
        return()
    endif()
    file(REAL_PATH "${directory}" directory)
    if(NOT directory IN_LIST O3DE_3RDPARTY_ROOTS)
        list(APPEND O3DE_3RDPARTY_ROOTS "${directory}")
    endif()
    foreach(search_path CMAKE_MODULE_PATH CMAKE_PREFIX_PATH)
        if(NOT directory IN_LIST ${search_path})
            list(APPEND ${search_path} "${directory}")
        endif()
    endforeach()

    get_property(registered GLOBAL PROPERTY "O3DE_3RDPARTY_ROOT_${directory}" SET)
    if(NOT registered)
        # Record logical ownership before add_subdirectory changes the parent chain.
        # External Gem roots need not be descendants of their consumer.
        cmake_path(IS_PREFIX LY_ROOT_FOLDER "${directory}" NORMALIZE in_engine)
        if(in_engine)
            set(owner "${LY_ROOT_FOLDER}")
        else()
            cmake_path(GET directory PARENT_PATH owner)
            if(COMMAND o3de_find_ancestor_gem_root)
                o3de_find_ancestor_gem_root(owner_gem owner_name "${directory}")
                if(owner_gem)
                    set(owner "${owner_gem}")
                endif()
            endif()
        endif()
        set_property(GLOBAL PROPERTY "O3DE_3RDPARTY_ROOT_${directory}" "${owner}")
        set_property(GLOBAL APPEND PROPERTY O3DE_REGISTERED_3RDPARTY_ROOTS "${directory}")
        # Installer setup replays these roots even when a generated Gem CMakeLists.txt no longer calls o3de_gem_setup
    endif()
    return(PROPAGATE O3DE_3RDPARTY_ROOTS CMAKE_MODULE_PATH CMAKE_PREFIX_PATH)
endfunction()

#! Map a compatibility target name to the package folder that exports it.
# The target name omits the implicit 3rdParty:: prefix. Components are preserved.
# This only registers the lookup. The recipe creates the actual target alias.
function(o3de_register_3rdparty_alias target package)
    set_property(GLOBAL PROPERTY "O3DE_3RDPARTY_PACKAGE_3rdParty::${target}" "${package}")
endfunction()

# A local Find module owns its acquisition strategy.
# Associations are the fallback for modules outside the registered source roots.
function(o3de_find_3rdparty_package package)
    foreach(root IN LISTS O3DE_3RDPARTY_ROOTS)
        if(EXISTS "${root}/Find${package}.cmake")
            list(PREPEND CMAKE_MODULE_PATH "${root}")
            find_package(${package} REQUIRED MODULE GLOBAL COMPONENTS ${ARGN})
            return()
        endif()
    endforeach()
    # The compatibility API also exposes paths to dependencies declared from sibling directories.
    # Preserve that fallback for existing integrations.
    get_property(additional_module_paths GLOBAL PROPERTY LY_ADDITIONAL_MODULE_PATH)
    list(APPEND CMAKE_MODULE_PATH ${additional_module_paths})
    ly_download_associated_package("${package}")
    find_package(${package} REQUIRED MODULE GLOBAL COMPONENTS ${ARGN})
endfunction()

# Shared by lazy activation and the generated installed-engine directory list.
function(o3de_add_3rdparty_subdirectory directory)
    # Installed script-only builds use baked mappings, not provider recipes.
    # The generated directory list must not recreate their target aliases.
    if(O3DE_SCRIPT_ONLY)
        return()
    endif()
    cmake_path(ABSOLUTE_PATH directory NORMALIZE)
    file(REAL_PATH "${directory}" directory)
    get_property(state GLOBAL PROPERTY "O3DE_3RDPARTY_STATE_${directory}")
    if(state STREQUAL "LOADING")
        message(FATAL_ERROR "Recursive third-party activation: ${directory}")
    elseif(state STREQUAL "LOADED")
        return()
    endif()

    cmake_path(GET directory PARENT_PATH root)
    get_property(owner GLOBAL PROPERTY "O3DE_3RDPARTY_ROOT_${root}")
    if(NOT owner)
        message(FATAL_ERROR "Third-party directory has no registered root: ${directory}")
    endif()
    set_property(GLOBAL PROPERTY "O3DE_3RDPARTY_DIRECTORY_OWNER_${directory}" "${owner}")
    cmake_path(RELATIVE_PATH directory BASE_DIRECTORY "${owner}" OUTPUT_VARIABLE relative_directory)
    cmake_path(IS_PREFIX LY_ROOT_FOLDER "${directory}" NORMALIZE in_engine)
    if(NOT in_engine)
        cmake_path(GET owner FILENAME owner_name)
        set(relative_directory "External/${owner_name}/${relative_directory}")
    endif()
    get_property(engine_binary DIRECTORY "${LY_ROOT_FOLDER}" PROPERTY BINARY_DIR)
    set_property(GLOBAL PROPERTY "O3DE_3RDPARTY_STATE_${directory}" LOADING)
    add_subdirectory("${directory}" "${engine_binary}/${relative_directory}")
    set_property(GLOBAL PROPERTY "O3DE_3RDPARTY_STATE_${directory}" LOADED)

    ly_get_vs_folder_directory("${directory}" ide_folder)
    o3de_set_3rdparty_folder("${directory}" "${ide_folder}")
endfunction()

# Visit only a providers activated subtree, including upstream FetchContent directories.
# A fixup helpers explicit IDE_FOLDER takes precedence.
function(o3de_set_3rdparty_folder directory folder)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_property(explicit_folder TARGET "${target}" PROPERTY O3DE_EXPLICIT_IDE_FOLDER)
        if(NOT explicit_folder)
            set_property(TARGET "${target}" PROPERTY FOLDER "${folder}")
        endif()
    endforeach()
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        get_property(provider_owner GLOBAL PROPERTY "O3DE_3RDPARTY_DIRECTORY_OWNER_${child}")
        if(NOT provider_owner)
            o3de_set_3rdparty_folder("${child}" "${folder}")
        endif()
    endforeach()
endfunction()

# Returns FALSE for legacy Find-module resolution.
# Do not cache misses! A Gem can register another root later in the same configure invocation.
function(o3de_resolve_3rdparty_target target resolved)
    if(TARGET "${target}")
        set(${resolved} TRUE PARENT_SCOPE)
        return()
    endif()
    # Compatibility names select a provider. Recipes own their actual aliases.
    # Never rewrite the requested target or change case-sensitive names.
    get_property(package_directory GLOBAL PROPERTY "O3DE_3RDPARTY_PACKAGE_${target}")
    if(NOT package_directory)
        string(REPLACE "::" ";" target_parts "${target}")
        list(GET target_parts 1 package)
        string(TOLOWER "${package}" package_directory)
    endif()
    foreach(root IN LISTS O3DE_3RDPARTY_ROOTS)
        if(EXISTS "${root}/${package_directory}/CMakeLists.txt")
            o3de_add_3rdparty_subdirectory("${root}/${package_directory}")
            if(NOT TARGET "${target}")
                message(FATAL_ERROR "Third-party provider ${root}/${package_directory} did not create ${target}")
            endif()
            set(${resolved} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${resolved} FALSE PARENT_SCOPE)
endfunction()

set(O3DE_RADEON_GPU_ANALYZER_ENABLED FALSE CACHE BOOL "Whether to download Radeon GPU Analyzer from Github.")
set(O3DE_FETCHCONTENT_MESSAGE_LEVEL "ERROR" CACHE STRING "Message level when fetching 3rd party libraries.  Set to DEBUG or VERBOSE to debug")
set(O3DE_FETCHCONTENT_FORCE_GIT OFF CACHE BOOL "Force FetchContent to use git to acquire packages instead of downloading archives")

define_property(TARGET PROPERTY LY_SYSTEM_LIBRARY
    BRIEF_DOCS "Defines a 3rdParty library as a system library"
    FULL_DOCS [[
        Property which is set on third party targets that should be considered
        as provided by the system. Such targets are excluded from the runtime
        dependencies considerations, and are not distributed as part of the
        O3DE SDK package. Instead, users of the SDK are expected to install
        such a third party library themselves.
    ]]
)

# Do not overcomplicate searching for the 3rdParty path, if it is not easy to find,
# the user should define it.

#! get_default_third_party_folder: Stores the default 3rdParty directory into the supplied output variable
#
# \arg:output_third_party_path name of variable to set the default project directory into
# It defaults to the ~/.o3de/3rdParty directory
function(get_default_third_party_folder output_third_party_path)
    
    # 1. Highest priority, cache variable, that will override the value of any of the cases below
    # 2. if defined in an env variable, take it from there
    if(DEFINED ENV{LY_3RDPARTY_PATH})
        set(${output_third_party_path} $ENV{LY_3RDPARTY_PATH} PARENT_SCOPE)
        return()
    endif()

    # 3. If defined in the o3de_manifest.json, take it from there
    cmake_path(SET home_directory "$ENV{USERPROFILE}") # Windows
    if(NOT EXISTS ${home_directory})
        cmake_path(SET home_directory "$ENV{HOME}") # Unix
        if (NOT EXISTS ${home_directory})
            return()
        endif()
    endif()

    set(manifest_path ${home_directory}/.o3de/o3de_manifest.json)
    if(EXISTS ${manifest_path})
        file(READ ${manifest_path} manifest_json)
        string(JSON default_third_party_folder ERROR_VARIABLE json_error GET ${manifest_json} default_third_party_folder)
        if(NOT json_error)
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_path})
            set(${output_third_party_path} ${default_third_party_folder} PARENT_SCOPE)
            return()
        endif()
    endif()

    # 4. Lowest priority, use the home directory as the location for 3rdparty
    set(${output_third_party_path} ${home_directory}/.o3de/3rdParty PARENT_SCOPE)

endfunction()

get_default_third_party_folder(o3de_default_third_party_path)
set(LY_3RDPARTY_PATH "${o3de_default_third_party_path}" CACHE PATH "Path to the 3rdParty folder")

if(LY_3RDPARTY_PATH)
    file(TO_CMAKE_PATH ${LY_3RDPARTY_PATH} LY_3RDPARTY_PATH)
    if(NOT EXISTS ${LY_3RDPARTY_PATH})
        file(MAKE_DIRECTORY ${LY_3RDPARTY_PATH})
    endif()
endif()
if(NOT EXISTS ${LY_3RDPARTY_PATH})
    message(FATAL_ERROR "3rdParty folder: ${LY_3RDPARTY_PATH} does not exist, call cmake defining a valid LY_3RDPARTY_PATH or use cmake-gui to configure it")
endif()

#! ly_add_external_target_path: adds a path to module path so 3rdparty Find files can be added from paths different than cmake/3rdParty
#
# \arg:PATH path to add
#
function(ly_add_external_target_path PATH)
    list(APPEND CMAKE_MODULE_PATH ${PATH})
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
    set_property(GLOBAL APPEND PROPERTY LY_ADDITIONAL_MODULE_PATH ${PATH})
endfunction()

#! ly_add_external_target: adds a library interface that exposes external libraries to be used as cmake dependencies.
#
# \arg:NAME name of the external library
# \arg:VERSION version of the external library. Location will be defined by 3rdPartyPath/NAME/VERSION
# \arg:3RDPARTY_DIRECTORY overrides the path to use when searching in the 3rdPartyPath (instead of NAME).
#                         If not indicated, PACKAGE will be used, if not indicated, NAME will be used.
# \arg:3RDPARTY_ROOT_DIRECTORY overrides the root path to the external library directory. This will be used instead of ${LY_3RDPARTY_PATH}.
# \arg:INCLUDE_DIRECTORIES include folders (relative to the root path where the external library is: ${LY_3RDPARTY_PATH}/${NAME}/${VERSION})
# \arg:PACKAGE if defined, defines the name of the external library "package". This is used when a package exposes multiple interfaces
#              if not defined, NAME is used
# \arg:COMPILE_DEFINITIONS compile definitions to be added to the interface
# \arg:BUILD_DEPENDENCIES list of interfaces this target depends on (could be a compilation dependency if the dependency is only
#                         exposing an include path, or could be a linking dependency is exposing a lib)
# \arg:RUNTIME_DEPENDENCIES list of files this target depends on (could be a dynamic libraries, text files, executables,
#                           applications, other 3rdParty targets, etc)
# \arg:OUTPUT_SUBDIRECTORY Subdirectory within bin/<Profile/Debug>/ where the ${PACKAGE_AND_NAME}_RUNTIME_DEPENDENCIES exported by the target will be copied to.
#                          If not specified, then the ${PACKAGE_AND_NAME}_RUNTIME_DEPENDENCIES will be copied directly under bin/<Profile/Debug>/.
#                          Each file listed in runtime dependencies can also customize its own output subfolder
#                          by adding "\n<subfolder path>" at the end of each listed file. OUTPUT_SUBDIRECTORY only works
#                          if such customized subfolder per file is NOT specified.
#                          Examples:
#                          1- If there are 5 files listed in ${PACKAGE_AND_NAME}_RUNTIME_DEPENDENCIES, and all of them
#                             should be copied to the same output subfolder named "My/Output/Subfolder" then it is advisable
#                             to set OUTPUT_SUBDIRECTORY to "My/Output/Subfolder" instead of appending: "\nMy/Output/Subfolder"
#                             at the end of each listed file.
#                          2- Assume there are 2 files listed in ${PACKAGE_AND_NAME}_RUNTIME_DEPENDENCIES, "fileA" must go to
#                             subdirectory "My/Output/Subfolder/lib" and "fileB" must go to subdirectory "My/Output/Subfolder/bin".
#                             In this case OUTPUT_SUBDIRECTORY should NOT be used, instead the files can be listed as:
#                             "fileA\nMy/Output/Subfolder/lib"
#                             "fileB\nMy/Output/Subfolder/bin"
#
# \arg:SYSTEM           If specified, the library is considered a system library, and is not copied to the build output directory
function(ly_add_external_target)

    set(options SYSTEM)
    set(oneValueArgs NAME VERSION 3RDPARTY_DIRECTORY PACKAGE 3RDPARTY_ROOT_DIRECTORY OUTPUT_SUBDIRECTORY)
    set(multiValueArgs HEADER_CHECK COMPILE_DEFINITIONS INCLUDE_DIRECTORIES BUILD_DEPENDENCIES RUNTIME_DEPENDENCIES)

    cmake_parse_arguments(ly_add_external_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_external_target_NAME)
        message(FATAL_ERROR "You must provide a name for the 3rd party library")
    endif()

    if(NOT ly_add_external_target_PACKAGE)
        set(ly_add_external_target_PACKAGE ${ly_add_external_target_NAME})
        set(PACKAGE_AND_NAME "${ly_add_external_target_NAME}")
        set(NAME_WITH_NAMESPACE "${ly_add_external_target_NAME}")
        set(has_package FALSE)
    else()
        set(PACKAGE_AND_NAME "${ly_add_external_target_PACKAGE}_${ly_add_external_target_NAME}")
        set(NAME_WITH_NAMESPACE "${ly_add_external_target_PACKAGE}::${ly_add_external_target_NAME}")
        set(has_package TRUE)
    endif()
    string(TOUPPER ${PACKAGE_AND_NAME} PACKAGE_AND_NAME)
    string(TOUPPER ${ly_add_external_target_PACKAGE} PACKAGE)

    if(NOT TARGET 3rdParty::${NAME_WITH_NAMESPACE})

        if(NOT DEFINED ly_add_external_target_VERSION AND NOT VERSION IN_LIST ly_add_external_target_KEYWORDS_MISSING_VALUES)
            message(FATAL_ERROR "You must provide a version of the \"${ly_add_external_target_PACKAGE}\" 3rd party library")
        endif()

        if(NOT ly_add_external_target_3RDPARTY_ROOT_DIRECTORY)
            if(NOT ly_add_external_target_3RDPARTY_DIRECTORY)
                if(ly_add_external_target_PACKAGE)
                    set(ly_add_external_target_3RDPARTY_DIRECTORY ${ly_add_external_target_PACKAGE})
                else()
                    set(ly_add_external_target_3RDPARTY_DIRECTORY ${ly_add_external_target_NAME})
                endif()
            endif()
            set(BASE_PATH "${LY_3RDPARTY_PATH}/${ly_add_external_target_3RDPARTY_DIRECTORY}")

        else()
            # only install external 3rdParty that are within the source tree
            cmake_path(IS_PREFIX LY_ROOT_FOLDER ${ly_add_external_target_3RDPARTY_ROOT_DIRECTORY} NORMALIZE is_in_source_tree)
            if(is_in_source_tree)
                ly_install_external_target(${ly_add_external_target_3RDPARTY_ROOT_DIRECTORY})
            endif()
            set(BASE_PATH "${ly_add_external_target_3RDPARTY_ROOT_DIRECTORY}")
        endif()

        if(ly_add_external_target_VERSION)
            set(BASE_PATH "${BASE_PATH}/${ly_add_external_target_VERSION}")
        endif()

        # Setting BASE_PATH variable in the parent scope to allow for the Find<3rdParty>.cmake scripts to use them
        set(BASE_PATH ${BASE_PATH} PARENT_SCOPE)

        add_library(3rdParty::${NAME_WITH_NAMESPACE} INTERFACE IMPORTED GLOBAL)

        if(ly_add_external_target_INCLUDE_DIRECTORIES)
            list(TRANSFORM ly_add_external_target_INCLUDE_DIRECTORIES PREPEND ${BASE_PATH}/)
            foreach(include_path ${ly_add_external_target_INCLUDE_DIRECTORIES})
                if(NOT EXISTS ${include_path})
                    message(FATAL_ERROR "Cannot find include path ${include_path} for 3rdParty::${NAME_WITH_NAMESPACE}")
                endif()
            endforeach()
            ly_target_include_system_directories(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                INTERFACE ${ly_add_external_target_INCLUDE_DIRECTORIES}
            )
        endif()

        # Check if there is a pal file
        o3de_pal_dir(pal_file ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}/${ly_add_external_target_PACKAGE}_${PAL_PLATFORM_NAME_LOWERCASE}.cmake "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")
        if(NOT EXISTS ${pal_file})
            set(pal_file ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}/${ly_add_external_target_PACKAGE}_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
        endif()
        if(EXISTS ${pal_file})
            include(${pal_file})
        endif()

        if(${PACKAGE_AND_NAME}_INCLUDE_DIRECTORIES)
            list(TRANSFORM ${PACKAGE_AND_NAME}_INCLUDE_DIRECTORIES PREPEND ${BASE_PATH}/)
            foreach(include_path ${${PACKAGE_AND_NAME}_INCLUDE_DIRECTORIES})
                string(GENEX_STRIP ${include_path} include_genex_expr)
                if(include_genex_expr STREQUAL include_path AND NOT EXISTS ${include_path}) # Exclude include paths that have generation expressions from validation
                    message(FATAL_ERROR "Cannot find include path ${include_path} for 3rdParty::${NAME_WITH_NAMESPACE}")
                endif()
            endforeach()
            ly_target_include_system_directories(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                INTERFACE ${${PACKAGE_AND_NAME}_INCLUDE_DIRECTORIES}
            )
        endif()

        if(has_package AND ${PACKAGE}_LIBS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "${${PACKAGE}_LIBS}"
            )
        endif()

        if(${PACKAGE_AND_NAME}_LIBS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "${${PACKAGE_AND_NAME}_LIBS}"
            )
        endif()

        if(has_package AND ${PACKAGE}_LINK_OPTIONS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_LINK_OPTIONS "${${PACKAGE}_LINK_OPTIONS}"
            )
        endif()
        if(${PACKAGE_AND_NAME}_LINK_OPTIONS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_LINK_OPTIONS "${${PACKAGE_AND_NAME}_LINK_OPTIONS}"
            )
        endif()

        if(has_package AND ${PACKAGE}_COMPILE_OPTIONS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_COMPILE_OPTIONS "${${PACKAGE}_COMPILE_OPTIONS}"
            )
        endif()
        if(${PACKAGE_AND_NAME}_COMPILE_OPTIONS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_COMPILE_OPTIONS "${${PACKAGE_AND_NAME}_COMPILE_OPTIONS}"
            )
        endif()

        unset(all_dependencies)
        if(ly_add_external_target_RUNTIME_DEPENDENCIES)
            list(APPEND all_dependencies ${ly_add_external_target_RUNTIME_DEPENDENCIES})
        endif()
        if(has_package AND ${PACKAGE}_RUNTIME_DEPENDENCIES)
            list(APPEND all_dependencies ${${PACKAGE}_RUNTIME_DEPENDENCIES})
        endif()
        if(${PACKAGE_AND_NAME}_RUNTIME_DEPENDENCIES)
            list(APPEND all_dependencies ${${PACKAGE_AND_NAME}_RUNTIME_DEPENDENCIES})
        endif()

        unset(locations)
        unset(manual_dependencies)
        if(all_dependencies)
            foreach(dependency ${all_dependencies})
                if(dependency MATCHES "3rdParty::")
                    list(APPEND manual_dependencies ${dependency})
                else()
                    if(ly_add_external_target_OUTPUT_SUBDIRECTORY)
                        string(APPEND dependency "\n${ly_add_external_target_OUTPUT_SUBDIRECTORY}")
                    endif()
                    list(APPEND locations ${dependency})
                endif()
            endforeach()
        endif()
        if(locations)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_IMPORTED_LOCATION "${locations}"
            )
        endif()
        if(manual_dependencies)
            ly_add_dependencies(3rdParty::${NAME_WITH_NAMESPACE} ${manual_dependencies})
        endif()
        get_property(additional_dependencies GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_3rdParty::${NAME_WITH_NAMESPACE})
        if(additional_dependencies)
            ly_add_dependencies(3rdParty::${NAME_WITH_NAMESPACE} ${additional_dependencies})
            # Clear the variable so we can track issues in case some dependency is added after
            set_property(GLOBAL PROPERTY LY_DELAYED_DEPENDENCIES_3rdParty::${NAME_WITH_NAMESPACE})
        endif()

        if(ly_add_external_target_COMPILE_DEFINITIONS)
            target_compile_definitions(3rdParty::${NAME_WITH_NAMESPACE}
                INTERFACE ${ly_add_external_target_COMPILE_DEFINITIONS}
            )
        endif()
        if(has_package AND ${PACKAGE}_COMPILE_DEFINITIONS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_COMPILE_DEFINITIONS "${${PACKAGE}_COMPILE_DEFINITIONS}"
            )
        endif()
        if(${PACKAGE_AND_NAME}_COMPILE_DEFINITIONS)
            set_property(TARGET 3rdParty::${NAME_WITH_NAMESPACE}
                APPEND PROPERTY
                    INTERFACE_COMPILE_DEFINITIONS "${${PACKAGE_AND_NAME}_COMPILE_DEFINITIONS}"
            )
        endif()

        if(has_package AND ${PACKAGE}_BUILD_DEPENDENCIES)
            list(APPEND ly_add_external_target_BUILD_DEPENDENCIES "${${PACKAGE}_BUILD_DEPENDENCIES}")
            list(REMOVE_DUPLICATES ly_add_external_target_BUILD_DEPENDENCIES)
        endif()
        if(${PACKAGE_AND_NAME}_BUILD_DEPENDENCIES)
            list(APPEND ly_add_external_target_BUILD_DEPENDENCIES "${${PACKAGE_AND_NAME}_BUILD_DEPENDENCIES}")
            list(REMOVE_DUPLICATES ly_add_external_target_BUILD_DEPENDENCIES)
        endif()

        # Interface dependencies may require to find_packages. So far, we are just using packages for 3rdParty, so we will
        # search for those and automatically bring those packages. The naming convention used is 3rdParty::PackageName::OptionalInterface
        foreach(dependency ${ly_add_external_target_BUILD_DEPENDENCIES})
            string(REPLACE "::" ";" dependency_list ${dependency})
            list(GET dependency_list 0 dependency_namespace)
            if(${dependency_namespace} STREQUAL "3rdParty")
                list(GET dependency_list 1 dependency_package)
                ly_download_associated_package(${dependency_package})
                find_package(${dependency_package} REQUIRED MODULE)
            endif()
        endforeach()

        if(ly_add_external_target_BUILD_DEPENDENCIES)
            target_link_libraries(3rdParty::${NAME_WITH_NAMESPACE}
                INTERFACE
                    ${ly_add_external_target_BUILD_DEPENDENCIES}
            )
        endif()

        if(ly_add_external_target_SYSTEM)
            set_target_properties(3rdParty::${NAME_WITH_NAMESPACE} PROPERTIES LY_SYSTEM_LIBRARY TRUE)
        endif()

    endif()

endfunction()

#! ly_install_external_target: external libraries which are not part of 3rdParty need to be installed
#
# \arg:3RDPARTY_ROOT_DIRECTORY custom 3rd party directory which needs to be installed
function(ly_install_external_target 3RDPARTY_ROOT_DIRECTORY)

    # Install the Find file to our <install_location>/cmake/3rdParty directory
    ly_install_files(FILES ${CMAKE_CURRENT_LIST_FILE}
        DESTINATION cmake/3rdParty
    )
    ly_install_directory(DIRECTORIES "${3RDPARTY_ROOT_DIRECTORY}")

endfunction()

# Utility function, pass it a single target or a list of targets, and it will do the following to them
# 1. Turn off warnings as errors (we are not responsible for warnings in 3p libraries)
# 2. Make sure its output directory is set to be different for each configuration so that binaries
#    do not overwrite each other between, for example, debug and release.
# 3. If the IDE the user is using has a visual display of folders, put the targets generated in that
#    folder instead of the root folder for visual display.
# 4. Specify that the target be installed
#
# Parameters:
#    IDE_FOLDER string - optional, defaults to the gem External folder.
#    TARGETS list      - required - The targets to fix up (can be a list or a single)
function(o3de_fixup_fetchcontent_targets)
    set(options)
    set(oneValueArgs IDE_FOLDER)
    set(multiValueArgs TARGETS)

    cmake_parse_arguments(o3de_fixup_fetchcontent_targets "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT o3de_fixup_fetchcontent_targets_TARGETS)
        message(FATAL_ERROR "o3de_fixup_fetchcontent_targets requires TARGETS to be specified")
        return()
    endif()

    # Suppress any developer warnings, fixing 3p libraries themselves are out of scope for us.
    set(PRIOR_SUPPRESS_DEVELOPER_WARNINGS ${CMAKE_SUPPRESS_DEVELOPER_WARNINGS}) # save the old CMAKE_SUPPRESS_DEVELOPER_WARNINGS
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

    set(BASE_LIBRARY_FOLDER "lib/${PAL_PLATFORM_NAME}")
    foreach(TARGET_TO_FIXUP ${o3de_fixup_fetchcontent_targets_TARGETS})
        if (NOT TARGET ${TARGET_TO_FIXUP})
            message(FATAL_ERROR "o3de_fixup_fetchcontent_targets invoked on non-existent target ${TARGET_TO_FIXUP}")
            continue()
        endif()
        # A lazy source provider supplies the default folder after activation.
        if(o3de_fixup_fetchcontent_targets_IDE_FOLDER)
            set_property(TARGET ${TARGET_TO_FIXUP} PROPERTY FOLDER "${o3de_fixup_fetchcontent_targets_IDE_FOLDER}")
            set_property(TARGET ${TARGET_TO_FIXUP} PROPERTY O3DE_EXPLICIT_IDE_FOLDER TRUE)
        else()
            get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
            if(this_gem_root)
                ly_get_engine_relative_source_dir("${this_gem_root}" relative_this_gem_root)
                set(folder "${relative_this_gem_root}/External")
            else()
                ly_get_vs_folder_directory("${CMAKE_CURRENT_SOURCE_DIR}" folder)
            endif()
            set_property(TARGET ${TARGET_TO_FIXUP} PROPERTY FOLDER "${folder}")
        endif()
        
        # alias it with 3rdParty::targetname
        add_library(3rdParty::${TARGET_TO_FIXUP} ALIAS ${TARGET_TO_FIXUP})

        # We install headers for fetchcontent libraries explicitly, so clear any PUBLIC_HEADER property.
        # Installing the target below without a PUBLIC_HEADER DESTINATION would warn.
        set_property(TARGET ${TARGET_TO_FIXUP} PROPERTY PUBLIC_HEADER)

        foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER ${conf} UCONF)

            # make sure that when building, the library and executable files end up in a place
            # that does not overwrite each other in different configs.
            set_target_properties(${TARGET_TO_FIXUP} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${UCONF} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}
                LIBRARY_OUTPUT_DIRECTORY_${UCONF} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UCONF}}
                )

            # 3p targets don't use warning-as-error
            target_compile_options(${TARGET_TO_FIXUP} ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS})

            # install any libraries to the install/lib/<Profile/Debug/Release> folder
            ly_install(TARGETS ${TARGET_TO_FIXUP}
                ARCHIVE
                    DESTINATION "${BASE_LIBRARY_FOLDER}/${conf}"
                    COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                    CONFIGURATIONS ${conf}
            )
        endforeach()
    endforeach()
    # restore the prior value of CMAKE_SUPPRESS_DEVELOPER_WARNINGS
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${PRIOR_SUPPRESS_DEVELOPER_WARNINGS} CACHE BOOL "" FORCE)
endfunction() # o3de_fixup_fetchcontent_targets


# Add the 3rdParty folder to find the modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/3rdParty)
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/3rdParty/Platform/${PAL_PLATFORM_NAME} "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")
list(APPEND CMAKE_MODULE_PATH ${pal_dir})

# Keep source-provider discovery with third-party search-path initialization.
include(${CMAKE_CURRENT_LIST_DIR}/3rdParty/LegacyAliases.cmake)
o3de_register_3rdparty_root("${LY_ROOT_FOLDER}/Code/3rdParty")

if(NOT INSTALLED_ENGINE)
    # Add the 3rdParty cmake files to the IDE
    ly_include_cmake_file_list(cmake/3rdParty/cmake_files.cmake)
    o3de_pal_dir(pal_3rdparty_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME} "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")
    ly_include_cmake_file_list(${pal_3rdparty_dir}/cmake_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake)
    if(O3DE_RADEON_GPU_ANALYZER_ENABLED)
        include(cmake/3rdParty/FetchRGA.cmake)
    endif()
endif()
