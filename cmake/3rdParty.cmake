#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

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
    if($ENV{LY_3RDPARTY_PATH})
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
        ly_get_absolute_pal_filename(pal_file ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}/${ly_add_external_target_PACKAGE}_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
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

# Add the 3rdParty folder to find the modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/3rdParty)
ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/3rdParty/Platform/${PAL_PLATFORM_NAME})
list(APPEND CMAKE_MODULE_PATH ${pal_dir})

if(NOT INSTALLED_ENGINE)
    # Add the 3rdParty cmake files to the IDE
    ly_include_cmake_file_list(cmake/3rdParty/cmake_files.cmake)
    ly_get_absolute_pal_filename(pal_3rdparty_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME})
    ly_include_cmake_file_list(${pal_3rdparty_dir}/cmake_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake)
endif()
