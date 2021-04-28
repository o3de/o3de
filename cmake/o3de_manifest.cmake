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

# Set the user home directory
set(O3DE_BUILD_SERVER_HOME_PATH "" CACHE PATH "!!!ONLY FOR BUILD SERVERS!!! This will override the home directory")
if(O3DE_BUILD_SERVER_HOME_PATH)
    set(home_directory ${O3DE_BUILD_SERVER_HOME_PATH})
elseif(CMAKE_HOST_WIN32)
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" home_directory)
elseif()
    file(TO_CMAKE_PATH "$ENV{HOME}" home_directory)
endif()
if (NOT home_directory)
    message(FATAL_ERROR "Cannot find user home directory, without know the home directory the o3de manifest cannot be found")
endif()

################################################################################
# O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH
#
# If you set this Cache Variable it will delete <user>/.o3de folder and recreate
# it by calling o3de register --this-engine using the supplied engine path.
# This is really only useful for build servers which have nothing registered and
# need to register, build and test a specific engine.
# Note: If this is run and an o3de object has a restricted set, that restricted
# must be 'o3de' or registration will fail because the only restricted 'o3de' is
# registered by default. So theoretically anything we ship as part of the engine
# will succeed, anything relying on something outside the engine will fail.
# For users, you could add anything your build servers need regiatered here:
# Modify this to register your outside objects by adding the appropriate
# additional calls after the register --this-engine call. EX:
# execute_process(
#   COMMAND cmd /c ${O3DE_ENGINE_PATH}/scripts/o3de.bat register --restricted-path <some/restricted/path>
#   RESULT_VARIABLE o3de_register_this_engine_cmd_result
#  )
#
# or if you need a certain outside project to always be registered
#
# execute_process(
#   COMMAND cmd /c ${O3DE_ENGINE_PATH}/scripts/o3de.bat register --project-path <some/project/path>
#   RESULT_VARIABLE o3de_register_outside_project_cmd_result
#  )
#
# or if you need a certain outside gem to always be resitered
#
# execute_process(
#   COMMAND cmd /c ${O3DE_ENGINE_PATH}/scripts/o3de.bat register --gem-path <some/gem/path>
#   RESULT_VARIABLE o3de_register_outside_gem_cmd_result
#  )
################################################################################
set(O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH "" CACHE PATH "!!!ONLY FOR BUILD SERVERS!!! This will wipe out the <user>/.o3de folder and register --this-engine using the provided engine path.")
if(O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH)
    message(STATUS "O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH is set to ${O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH}.")
    message(STATUS "Delete ${home_directory}/.o3de and try to register --this-engine using the ${O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH}...")

    if(EXISTS ${home_directory}/.o3de)
        file(REMOVE_RECURSE ${home_directory}/.o3de)
    endif()

    if(CMAKE_HOST_WIN32)
        execute_process(
                  COMMAND cmd /c ${O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH}/scripts/o3de.bat register --this-engine
                  RESULT_VARIABLE o3de_register_this_engine_cmd_result
               )
    else()
        execute_process(
                  COMMAND bash ${O3DE_BUILD_SERVER_REGISTER_ENGINE_PATH}/scripts/o3de.sh register --this-engine
                  RESULT_VARIABLE o3de_register_this_engine_cmd_result
               )
    endif()
    if(o3de_register_this_engine_cmd_result)
        message(FATAL_ERROR "An error occured trying to o3de register --this-engine: ${o3de_register_this_engine_cmd_result}")
    else()
        message(STATUS "Registration successfull.")
    endif()
endif()


################################################################################
# o3de manifest
################################################################################
# Set manifest json path to the <home_directory>/.o3de/o3de_manifest.json
set(o3de_manifest_json_path ${home_directory}/.o3de/o3de_manifest.json)
if(NOT EXISTS ${o3de_manifest_json_path})
    message(FATAL_ERROR "${o3de_manifest_json_path} not found. You must o3de register --this-engine.")
endif()
file(READ ${o3de_manifest_json_path} manifest_json_data)

################################################################################
# o3de manifest name
################################################################################
string(JSON o3de_manifest_name ERROR_VARIABLE json_error GET ${manifest_json_data} o3de_manifest_name)
message(STATUS "o3de_manifest_name: ${o3de_manifest_name}")
if(json_error)
    message(FATAL_ERROR "Unable to read repo_name from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de origin
################################################################################
string(JSON o3de_origin ERROR_VARIABLE json_error GET ${manifest_json_data} origin)
if(json_error)
    message(FATAL_ERROR "Unable to read origin from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de default engines folder
################################################################################
string(JSON o3de_default_engines_folder ERROR_VARIABLE json_error GET ${manifest_json_data} default_engines_folder)
message(STATUS "default_engines_folder: ${o3de_default_engines_folder}")
if(json_error)
    message(FATAL_ERROR "Unable to read default_engines_folder from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de default projects folder
################################################################################
string(JSON o3de_default_projects_folder ERROR_VARIABLE json_error GET ${manifest_json_data} default_projects_folder)
message(STATUS "default_projects_folder: ${o3de_default_projects_folder}")
if(json_error)
    message(FATAL_ERROR "Unable to read default_projects_folder from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de default gems folder
################################################################################
string(JSON o3de_default_gems_folder ERROR_VARIABLE json_error GET ${manifest_json_data} default_gems_folder)
message(STATUS "default_gems_folder: ${o3de_default_gems_folder}")
if(json_error)
    message(FATAL_ERROR "Unable to read default_gems_folder from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de default templates folder
################################################################################
string(JSON o3de_default_templates_folder ERROR_VARIABLE json_error GET ${manifest_json_data} default_templates_folder)
message(STATUS "default_templates_folder: ${o3de_default_templates_folder}")
if(json_error)
    message(FATAL_ERROR "Unable to read default_templates_folder from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de default restricted folder
################################################################################
string(JSON o3de_default_restricted_folder ERROR_VARIABLE json_error GET ${manifest_json_data} default_restricted_folder)
message(STATUS "default_restricted_folder: ${o3de_default_restricted_folder}")
if(json_error)
    message(FATAL_ERROR "Unable to read default_restricted_folder from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

################################################################################
# o3de projects
################################################################################
string(JSON o3de_projects_count ERROR_VARIABLE json_error LENGTH ${manifest_json_data} projects)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'projects' from '${o3de_manifest_json_path}', error: ${json_error}")
endif()
if(${o3de_projects_count} GREATER 0)
    math(EXPR o3de_projects_count "${o3de_projects_count}-1")
    foreach(projects_index RANGE ${o3de_projects_count})
        string(JSON projects_path ERROR_VARIABLE json_error GET ${manifest_json_data} projects ${projects_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read projects[${projects_index}] '${o3de_manifest_json_path}', error: ${json_error}")
        endif()
        list(APPEND o3de_projects ${projects_path})
        list(APPEND o3de_global_projects ${projects_path})
    endforeach()
endif()

################################################################################
# o3de gems
################################################################################
string(JSON o3de_gems_count ERROR_VARIABLE json_error LENGTH ${manifest_json_data} gems)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'gems' from '${o3de_manifest_json_path}', error: ${json_error}")
endif()
if(${o3de_gems_count} GREATER 0)
    math(EXPR o3de_gems_count "${o3de_gems_count}-1")
    foreach(gems_index RANGE ${o3de_gems_count})
        string(JSON gems_path ERROR_VARIABLE json_error GET ${manifest_json_data} gems ${gems_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read gems[${gems_index}] '${o3de_manifest_json_path}', error: ${json_error}")
        endif()
        list(APPEND o3de_gems ${gems_path})
        list(APPEND o3de_global_gems ${gems_path})
    endforeach()
endif()

################################################################################
# o3de templates
################################################################################
string(JSON o3de_templates_count ERROR_VARIABLE json_error LENGTH ${manifest_json_data} templates)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'templates' from '${o3de_manifest_json_path}', error: ${json_error}")
endif()
if(${o3de_templates_count} GREATER 0)
    math(EXPR o3de_templates_count "${o3de_templates_count}-1")
    foreach(templates_index RANGE ${o3de_templates_count})
        string(JSON templates_path ERROR_VARIABLE json_error GET ${manifest_json_data} templates ${templates_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read templates[${templates_index}] '${o3de_manifest_json_path}', error: ${json_error}")
        endif()
        list(APPEND o3de_templates ${templates_path})
        list(APPEND o3de_global_templates ${templates_path})
    endforeach()
endif()

################################################################################
# o3de repos
################################################################################
string(JSON o3de_repos_count ERROR_VARIABLE json_error LENGTH ${manifest_json_data} repos)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'repos' from '${o3de_manifest_json_path}', error: ${json_error}")
endif()
if(${o3de_repos_count} GREATER 0)
    math(EXPR o3de_repos_count "${o3de_repos_count}-1")
    foreach(repos_index RANGE ${o3de_repos_count})
        string(JSON repo_uri ERROR_VARIABLE json_error GET ${manifest_json_data} repos ${repos_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read repos[${repos_index}] '${o3de_manifest_json_path}', error: ${json_error}")
        endif()
        list(APPEND o3de_repos ${repo_uri})
        list(APPEND o3de_global_repos ${repo_uri})
    endforeach()
endif()

################################################################################
# o3de restricted
################################################################################
string(JSON o3de_restricted_count ERROR_VARIABLE json_error LENGTH ${manifest_json_data} restricted)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'restricted' from '${o3de_manifest_json_path}', error: ${json_error}")
endif()
if(${o3de_restricted_count} GREATER 0)
    math(EXPR o3de_restricted_count "${o3de_restricted_count}-1")
    foreach(restricted_index RANGE ${o3de_restricted_count})
        string(JSON restricted_path ERROR_VARIABLE json_error GET ${manifest_json_data} restricted ${restricted_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read restricted[${restricted_index}] '${o3de_manifest_json_path}', error: ${json_error}")
        endif()
        list(APPEND o3de_restricted ${restricted_path})
        list(APPEND o3de_global_restricted ${restricted_path})
    endforeach()
endif()

################################################################################
# o3de engines
################################################################################
string(JSON o3de_engines_count ERROR_VARIABLE json_error LENGTH ${manifest_json_data} engines)
if(json_error)
    message(FATAL_ERROR "Unable to read key 'engines' from '${o3de_manifest_json_path}', error: ${json_error}")
endif()

if(${o3de_engines_count} GREATER 0)
    math(EXPR o3de_engines_count "${o3de_engines_count}-1")
    # Either the engine_path and engine_json are set in which case the user is configuring from the engine
    # or project_path and project_json are set in which case the user is configuring from the project.
    # We need to know which engine_path the user is using so if the project_json is set then we need
    # to read the project_json and disambiguate the engine_path.
    if(NOT o3de_engine_path)
        if(NOT o3de_project_json)
            message(FATAL_ERROR "Neither o3de_engine_path nor o3de_project_json defined. Cannot determine engine!")
        endif()

        # get the name of the engine this project uses
        file(READ ${o3de_project_json} project_json_data)
        string(JSON project_engine_name ERROR_VARIABLE json_error GET ${project_json_data} engine)
        if(json_error)
            message(FATAL_ERROR "Unable to read 'engine' from '${o3de_project_json}', error: ${json_error}")
        endif()

        # search each engine in order from the manifest to find the matching engine name
        foreach(engines_index RANGE ${o3de_engines_count})
            string(JSON engine_data ERROR_VARIABLE json_error GET ${manifest_json_data} engines ${engines_index})
            if(json_error)
                message(FATAL_ERROR "Unable to read engines[${engines_index}] '${o3de_manifest_json_path}', error: ${json_error}")
            endif()

            # get this engines path
            string(JSON this_engine_path ERROR_VARIABLE json_error GET ${engine_data} path)
            if(json_error)
                message(FATAL_ERROR "Unable to read engine path from '${o3de_manifest_json_path}', error: ${json_error}")
            endif()

            # add this engine to the engines list
            list(APPEND o3de_engines ${this_engine_path})

            # use path to get the engine.json
            set(this_engine_json ${this_engine_path}/engine.json)

            # read the name of this engine
            file(READ ${this_engine_json} this_engine_json_data)
            string(JSON this_engine_name ERROR_VARIABLE json_error GET ${this_engine_json_data} engine_name)
            if(json_error)
                message(FATAL_ERROR "Unable to read engine_name from '${this_engine_json}', error: ${json_error}")
            endif()

            # see if this engines name is the same as the one this projects should use
            if(${this_engine_name} STREQUAL ${project_engine_name})
                message(STATUS "Found engine: '${project_engine_name}' at ${this_engine_path}")
                set(o3de_engine_path ${this_engine_path})
                break()
            endif()
        endforeach()
    endif()
endif()

#we should have an engine_path at this point
if(NOT o3de_engine_path)
    message(FATAL_ERROR "o3de_engine_path not defined. Cannot determine engine!")
endif()

# now that we have an engine_path read in that engines o3de resources
if(${o3de_engines_count} GREATER -1)
    foreach(engines_index RANGE ${o3de_engines_count})
        string(JSON engine_data ERROR_VARIABLE json_error GET ${manifest_json_data} engines ${engines_index})
        if(json_error)
            message(FATAL_ERROR "Unable to read engines[${engines_index}] '${o3de_manifest_json_path}', error: ${json_error}")
        endif()

        # get this engines path
        string(JSON this_engine_path ERROR_VARIABLE json_error GET ${engine_data} path)
        if(json_error)
            message(FATAL_ERROR "Unable to read engine path from '${o3de_manifest_json_path}', error: ${json_error}")
        endif()

        if(${o3de_engine_path} STREQUAL ${this_engine_path})
            ################################################################################
            # o3de engine projects
            ################################################################################
            string(JSON o3de_engine_projects_count ERROR_VARIABLE json_error LENGTH ${engine_data} projects)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'projects' from '${engine_data}', error: ${json_error}")
            endif()
            if(${o3de_engine_projects_count} GREATER 0)
                math(EXPR o3de_engine_projects_count "${o3de_engine_projects_count}-1")
                foreach(engine_projects_index RANGE ${o3de_engine_projects_count})
                    string(JSON engine_projects_path ERROR_VARIABLE json_error GET ${engine_data} projects ${engine_projects_index})
                    if(json_error)
                        message(FATAL_ERROR "Unable to read engine projects[${projects_index}] '${o3de_manifest_json_path}', error: ${json_error}")
                    endif()
                    list(APPEND o3de_projects ${engine_projects_path})
                    list(APPEND o3de_engine_projects ${engine_projects_path})
                endforeach()
            endif()

            ################################################################################
            # o3de engine gems
            ################################################################################
            string(JSON o3de_engine_gems_count ERROR_VARIABLE json_error LENGTH ${engine_data} gems)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'gems' from '${engine_data}', error: ${json_error}")
            endif()
            if(${o3de_engine_gems_count} GREATER 0)
                math(EXPR o3de_engine_gems_count "${o3de_engine_gems_count}-1")
                foreach(engine_gems_index RANGE ${o3de_engine_gems_count})
                    string(JSON engine_gems_path ERROR_VARIABLE json_error GET ${engine_data} gems ${engine_gems_index})
                    if(json_error)
                        message(FATAL_ERROR "Unable to read engine gems[${gems_index}] '${o3de_manifest_json_path}', error: ${json_error}")
                    endif()
                    list(APPEND o3de_gems ${engine_gems_path})
                    list(APPEND o3de_engine_gems ${engine_gems_path})
                endforeach()
            endif()

            ################################################################################
            # o3de engine templates
            ################################################################################
            string(JSON o3de_engine_templates_count ERROR_VARIABLE json_error LENGTH ${engine_data} templates)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'templates' from '${o3de_manifest_json_path}', error: ${json_error}")
            endif()
            if(${o3de_engine_gems_count} GREATER 0)
                math(EXPR o3de_engine_templates_count "${o3de_engine_templates_count}-1")
                foreach(engine_templates_index RANGE ${o3de_engine_templates_count})
                    string(JSON engine_templates_path ERROR_VARIABLE json_error GET ${engine_data} templates ${engine_templates_index})
                    if(json_error)
                        message(FATAL_ERROR "Unable to read engine templates[${templates_index}] '${o3de_manifest_json_path}', error: ${json_error}")
                    endif()
                    list(APPEND o3de_templates ${engine_templates_path})
                    list(APPEND o3de_engine_templates ${engine_templates_path})
                endforeach()
            endif()

            ################################################################################
            # o3de engine restricted
            ################################################################################
            string(JSON o3de_engine_restricted_count ERROR_VARIABLE json_error LENGTH ${engine_data} restricted)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'restricted' from '${o3de_manifest_json_path}', error: ${json_error}")
            endif()
            if(${o3de_engine_restricted_count} GREATER 0)
                math(EXPR o3de_engine_restricted_count "${o3de_engine_restricted_count}-1")
                foreach(engine_restricted_index RANGE ${o3de_engine_restricted_count})
                    string(JSON engine_restricted_path ERROR_VARIABLE json_error GET ${engine_data} restricted ${engine_restricted_index})
                    if(json_error)
                        message(FATAL_ERROR "Unable to read engine restricted[${engine_restricted_index}] '${o3de_manifest_json_path}', error: ${json_error}")
                    endif()
                    list(APPEND o3de_restricted ${engine_restricted_path})
                    list(APPEND o3de_engine_restricted ${engine_restricted_path})
                endforeach()
            endif()

            ################################################################################
            # o3de engine external_subdirectories
            ################################################################################
            string(JSON o3de_external_subdirectories_count ERROR_VARIABLE json_error LENGTH ${engine_data} external_subdirectories)
            if(json_error)
                message(FATAL_ERROR "Unable to read key 'external_subdirectories' from '${o3de_manifest_json_path}', error: ${json_error}")
            endif()
            if(${o3de_external_subdirectories_count} GREATER 0)
                math(EXPR o3de_external_subdirectories_count "${o3de_external_subdirectories_count}-1")
                foreach(external_subdirectories_index RANGE ${o3de_external_subdirectories_count})
                    string(JSON external_subdirectories_path ERROR_VARIABLE json_error GET ${engine_data} external_subdirectories ${external_subdirectories_index})
                    if(json_error)
                        message(FATAL_ERROR "Unable to read engine external_subdirectories[${gems_index}] '${o3de_manifest_json_path}', error: ${json_error}")
                    endif()
                    list(APPEND o3de_engine_external_subdirectories ${external_subdirectories_path})
                endforeach()
            endif()

            break()

        endif()
    endforeach()
endif()


################################################################################
#! o3de_engine_id:
#
# \arg:engine returns the engine association element from an o3de json
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_engine_id o3de_json_file engine)
    file(READ ${o3de_json_file} json_data)
    string(JSON engine_entry ERROR_VARIABLE json_error GET ${json_data} engine)
    if(json_error)
        message(WARNING "Unable to read engine from '${o3de_json_file}', error: ${json_error}")
        message(WARNING "Setting engine to engine default 'o3de'")
        set(engine_entry "o3de")
    endif()
    if(engine_entry)
        set(${engine} ${engine_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_project_id:
#
# \arg:project returns the project association element from an o3de json
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_project_id o3de_json_file project)
    file(READ ${o3de_json_file} json_data)
    string(JSON project_entry ERROR_VARIABLE json_error GET ${json_data} project)
    if(json_error)
        message(FATAL_ERROR "Unable to read project from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(project_entry)
        set(${project} ${project_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_gem_id:
#
# \arg:gem returns the gem association element from an o3de json
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_gem_id o3de_json_file gem)
    file(READ ${o3de_json_file} json_data)
    string(JSON gem_entry ERROR_VARIABLE json_error GET ${json_data} gem)
    if(json_error)
        message(FATAL_ERROR "Unable to read gem from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(gem_entry)
        set(${gem} ${gem_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_template_id:
#
# \arg:template returns the template association element from an o3de json
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_template_id o3de_json_file template)
    file(READ ${o3de_json_file} json_data)
    string(JSON template_entry ERROR_VARIABLE json_error GET ${json_data} template)
    if(json_error)
        message(FATAL_ERROR "Unable to read template from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(template_entry)
        set(${template} ${template_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_repo_id:
#
# \arg:repo returns the repo association element from an o3de json
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_repo_id o3de_json_file repo)
    file(READ ${o3de_json_file} json_data)
    string(JSON repo_entry ERROR_VARIABLE json_error GET ${json_data} repo)
    if(json_error)
        message(FATAL_ERROR "Unable to read repo from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(repo_entry)
        set(${repo} ${repo_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_restricted_id:
#
# \arg:restricted returns the restricted association element from an o3de json, otherwise engine 'o3de' is assumed
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_restricted_id o3de_json_file restricted)
    file(READ ${o3de_json_file} json_data)
    string(JSON restricted_entry ERROR_VARIABLE json_error GET ${json_data} restricted)
    if(json_error)
        message(WARNING "Unable to read restricted from '${o3de_json_file}', error: ${json_error}")
        message(WARNING "Setting restricted to engine default 'o3de'")
        set(restricted_entry "o3de")
    endif()
    if(restricted_entry)
       set(${restricted} ${restricted_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_find_engine_folder:
#
# \arg:engine_path returns the path of the o3de engine folder with name engine_name
# \arg:engine_name name of the engine
################################################################################
function(o3de_find_engine_folder engine_name engine_path)
    foreach(engine_entry ${o3de_engines})
        set(engine_json_file ${engine_entry}/engine.json)
        file(READ ${engine_json_file} engine_json)
        string(JSON this_engine_name ERROR_VARIABLE json_error GET ${engine_json} engine_name)
        if(json_error)
            message(WARNING "Unable to read engine_name from '${engine_json_file}', error: ${json_error}")
        else()
            if(this_engine_name STREQUAL engine_name)
                set(${engine_path} ${engine_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    message(FATAL_ERROR "Unable to find repo_name: '${engine_name}'")
endfunction()


################################################################################
#! o3de_find_project_folder:
#
# \arg:project_path returns the path of the o3de project folder with name project_name
# \arg:project_name name of the project
################################################################################
function(o3de_find_project_folder project_name project_path)
    foreach(project_entry ${o3de_projects})
        set(project_json_file ${project_entry}/project.json)
        file(READ ${project_json_file} project_json)
        string(JSON this_project_name ERROR_VARIABLE json_error GET ${project_json} project_name)
        if(json_error)
            message(WARNING "Unable to read project_name from '${project_json_file}', error: ${json_error}")
        else()
            if(this_project_name STREQUAL project_name)
                set(${project_path} ${project_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    message(FATAL_ERROR "Unable to find project_name: '${project_name}'")
endfunction()


################################################################################
#! o3de_find_gem_folder:
#
# \arg:gem_path returns the path of the o3de gem folder with name gem_name
# \arg:gem_name name of the gem
################################################################################
function(o3de_find_gem_folder gem_name gem_path)
    foreach(gem_entry ${o3de_gems})
        set(gem_json_file ${gem_entry}/gem.json)
        file(READ ${gem_json_file} gem_json)
        string(JSON this_gem_name ERROR_VARIABLE json_error GET ${gem_json} gem_name)
        if(json_error)
            message(WARNING "Unable to read gem_name from '${gem_json_file}', error: ${json_error}")
        else()
            if(this_gem_name STREQUAL gem_name)
                set(${gem_path} ${gem_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    message(FATAL_ERROR "Unable to find gem_name: '${gem_name}'")
endfunction()


################################################################################
#! o3de_find_template_folder:
#
# \arg:template_path returns the path of the o3de template folder with name template_name
# \arg:template_name name of the template
################################################################################
function(o3de_find_template_folder template_name template_path)
    foreach(template_entry ${o3de_templates})
        set(template_json_file ${template_entry}/template.json)
        file(READ ${template_json_file} template_json)
        string(JSON this_template_name ERROR_VARIABLE json_error GET ${template_json} template_name)
        if(json_error)
            message(WARNING "Unable to read template_name from '${template_json_file}', error: ${json_error}")
        else()
            if(this_template_name STREQUAL template_name)
                set(${template_path} ${template_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    message(FATAL_ERROR "Unable to find template_name: '${template_name}'")
endfunction()


################################################################################
#! o3de_find_repo_folder:
#
# \arg:repo_path returns the path of the o3de repo folder with name repo_name
# \arg:repo_name name of the repo
################################################################################
function(o3de_find_repo_folder repo_name repo_path)
    foreach(repo_entry ${o3de_repos})
        set(repo_json_file ${repo_entry}/repo.json)
        file(READ ${repo_json_file} repo_json)
        string(JSON this_repo_name ERROR_VARIABLE json_error GET ${repo_json} repo_name)
        if(json_error)
            message(WARNING "Unable to read repo_name from '${repo_json_file}', error: ${json_error}")
        else()
            if(this_repo_name STREQUAL repo_name)
                set(${repo_path} ${repo_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    message(FATAL_ERROR "Unable to find repo_name: '${repo_name}'")
endfunction()


################################################################################
#! o3de_find_restricted_folder:
#
# \arg:restricted_path returns the path of the o3de restricted folder with name restricted_name
# \arg:restricted_name name of the restricted
################################################################################
function(o3de_find_restricted_folder restricted_name restricted_path)
    foreach(restricted_entry ${o3de_restricted})
        set(restricted_json_file ${restricted_entry}/restricted.json)
        file(READ ${restricted_json_file} restricted_json)
        string(JSON this_restricted_name ERROR_VARIABLE json_error GET ${restricted_json} restricted_name)
        if(json_error)
            message(WARNING "Unable to read restricted_name from '${restricted_json_file}', error: ${json_error}")
        else()
            if(this_restricted_name STREQUAL restricted_name)
                set(${restricted_path} ${restricted_entry} PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    message(FATAL_ERROR "Unable to find restricted_name: '${restricted_name}'")
endfunction()


################################################################################
#! o3de_engine_name:
#
# \arg:engine returns the engine_name element from an engine.json
# \arg:o3de_engine_json_file name of the o3de json file
################################################################################
function(o3de_engine_name o3de_engine_json_file engine)
    file(READ ${o3de_engine_json_file} json_data)
    string(JSON engine_entry ERROR_VARIABLE json_error GET ${json_data} engine_name)
    if(json_error)
        message(FATAL_ERROR "Unable to read engine_name from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(engine_entry)
        set(${engine} ${engine_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_project_name:
#
# \arg:project returns the project_name element from an project.json
# \arg:o3de_project_json_file name of the o3de json file
################################################################################
function(o3de_project_name o3de_project_json_file project)
    file(READ ${o3de_project_json_file} json_data)
    string(JSON project_entry ERROR_VARIABLE json_error GET ${json_data} project_name)
    if(json_error)
        message(FATAL_ERROR "Unable to read project_name from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(project_entry)
        set(${project} ${project_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_gem_name:
#
# \arg:gem returns the gem_name element from an gem.json
# \arg:o3de_gem_json_file name of the o3de json file
################################################################################
function(o3de_gem_name o3de_gem_json_file gem)
    file(READ ${o3de_gem_json_file} json_data)
    string(JSON gem_entry ERROR_VARIABLE json_error GET ${json_data} gem_name)
    if(json_error)
        message(FATAL_ERROR "Unable to read gem_name from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(gem_entry)
        set(${gem} ${gem_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_template_name:
#
# \arg:template returns the template_name element from an template json
# \arg:o3de_template_json_file name of the o3de json file
################################################################################
function(o3de_template_name o3de_template_json_file template)
    file(READ ${o3de_template_json_file} json_data)
    string(JSON template_entry ERROR_VARIABLE json_error GET ${json_data} template_name)
    if(json_error)
        message(FATAL_ERROR "Unable to read template_name from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(template_entry)
        set(${template} ${template_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_repo_name:
#
# \arg:repo returns the repo_name element from an repo.json or o3de_manifest.json
# \arg:o3de_repo_json_file name of the o3de json file
################################################################################
function(o3de_repo_name o3de_repo_json_file repo)
    file(READ ${o3de_repo_json_file} json_data)
    string(JSON repo_entry ERROR_VARIABLE json_error GET ${json_data} repo_name)
    if(json_error)
        message(FATAL_ERROR "Unable to read repo_name from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(repo_entry)
        set(${repo} ${repo_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_restricted_name:
#
# \arg:restricted returns the restricted association element from an o3de json
# \arg:o3de_json_file name of the o3de json file
################################################################################
function(o3de_restricted_name o3de_json_file restricted)
    file(READ ${o3de_json_file} json_data)
    string(JSON restricted_entry ERROR_VARIABLE json_error GET ${json_data} restricted_name)
    if(json_error)
        message(WARNING "FATAL_ERROR to read restricted_name from '${o3de_json_file}', error: ${json_error}")
    endif()
    if(restricted_entry)
        set(${restricted} ${restricted_entry} PARENT_SCOPE)
    endif()
endfunction()


################################################################################
#! o3de_engine_path:
#
# \arg:engine_path returns the path of the o3de engine folder with name engine_name
# \arg:engine_name name of the engine
################################################################################
function(o3de_engine_path o3de_json_file engine_path)
    o3de_engine_id(${o3de_json_file} engine_name)
    if(engine_name)
        o3de_find_engine_folder(${engine_name} engine_folder)
        if(engine_folder)
            set(${engine_path} ${engine_folder} PARENT_SCOPE)
        endif()
    endif()
endfunction()


################################################################################
#! o3de_project_path:
#
# \arg:project_path returns the path of the o3de project folder with name project_name
# \arg:project_name name of the project
################################################################################
function(o3de_project_path o3de_json_file project_path)
    o3de_project_id(${o3de_json_file} project_name)
    if(project_name)
        o3de_find_project_folder(${project_name} project_folder)
        if(project_folder)
            set(${project_path} ${project_folder} PARENT_SCOPE)
        endif()
    endif()
endfunction()


################################################################################
#! o3de_template_path:
#
# \arg:template_path returns the path of the o3de template folder with name template_name
# \arg:template_name name of the template
################################################################################
function(o3de_template_path o3de_json_file template_path)
    o3de_template_id(${o3de_json_file} template_name)
    if(template_name)
        o3de_find_template_folder(${template_name} template_folder)
        if(template_folder)
            set(${template_path} ${template_folder} PARENT_SCOPE)
        endif()
    endif()
endfunction()


################################################################################
#! o3de_repo_path:
#
# \arg:repo_path returns the path of the o3de repo folder with name repo_name
# \arg:repo_name name of the repo
################################################################################
function(o3de_repo_path o3de_json_file repo_path)
    o3de_repo_id(${o3de_json_file} repo_name)
    if(repo_name)
        o3de_find_repo_folder(${repo_name} repo_folder)
        if(repo_folder)
            set(${repo_path} ${repo_folder} PARENT_SCOPE)
        endif()
    endif()
endfunction()


################################################################################
#! o3de_restricted_path:
#
# \arg:restricted_path returns the path of the o3de restricted folder with name restricted_name
# \arg:restricted_name name of the restricted
################################################################################
function(o3de_restricted_path o3de_json_file restricted_path)
    o3de_restricted_id(${o3de_json_file} restricted_name)
    if(restricted_name)
        o3de_find_restricted_folder(${restricted_name} restricted_folder)
        if(restricted_folder)
            set(${restricted_path} ${restricted_folder} PARENT_SCOPE)
        endif()
    endif()
endfunction()
