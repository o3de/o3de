#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Contains functions for the project manager to call that gather data from o3de scripts and massage it
"""

import logging
import pathlib

from o3de import manifest, utils, compatibility, enable_gem, register, project_properties, repo

logger = logging.getLogger('o3de.project_manager_interface')
logging.basicConfig(format=utils.LOG_FORMAT)


#### Engine methods ###

def get_engine_info(engine_name: str = None, engine_path: str = None) -> dict or None:
    """
        If engine_path is provided then continue
        Elif only engine_name is provided then call get_registered and use returned path as engine_path
        Elif neither parameter is provided call get_this_engine_path and use returned path as engine_path

        Call get_engine_json_data using engine_path to populate initial engine data

        Call load_o3de_manifest or get default folders for gems, projects, ect. using get_o3de_..._folder
        Insert either default folder from manifest or get_o3de_..._folder into engine dict for all folders

        Call get_manifest_engines and check that engine is in the list if it is set registered attr to True

        :param engine_name: Engine name to find engine path for and get json data for
        :param engine_path: Engine path to gather json data for

        :return dict containing engine info if it was found. Otherwise None is returned
    """
    return dict()


def set_engine_info(engine_info: dict):
    """
        Call get_engine_info using path from engine_info

        Call edit_engine_props if any relevant properties have changed

        Call register if any relevant properties have changed

        :param engine_info: dict containing values to change in engine
    """
    pass


#### Template methods ###

def get_project_template_infos() -> list:
    """
        Get list of registered project templates using get_templates_for_project_creation

        Gather together list of dicts for each template using get_template_json_data

        :return list of dicts containing template infos.
    """
    return list()


#### Project methods ###

def register_project(project_path: pathlib.Path, force:bool) -> int:
    """
        Registers project with engine and sets the user/project.json engine_path
        in case there are multiple versions of this project on the user's machine

        :param project_path: Project path to register
        :param force: Whether to force registeration, bypassing compatibility checks
        :return 0 on success or a non-zero error code
    """
    result = register.register(project_path=project_path, force=force)
    if result == 0:
        result = project_properties.edit_project_props(proj_path=project_path, 
                                                       user=True, 
                                                       new_engine_path=manifest.get_this_engine_path())

    return result



def unregister_project(project_path: str):
    """
        Unregisters project with engine

        :param project_path: Project path to unregister
    """
    pass


def unregister_invalid_projects():
    """
        Call remove_invalid_o3de_projects
    """
    pass


def create_project(project_info: dict, template_path: str):
    """
        Creates project from project_info data with template_path using create_project

        If project is created successfully then registers project using register_project

        :param project_info: dict containing project info to create new project
        :param template_path: Path to template to create project with
    """
    pass


def get_enabled_gems(project_path: str, include_dependencies: bool = True) -> list:
    """
        Call get_enabled_gem_cmake_file for project_path

        Return get_enabled_gems for cmake file

        :param project_path: Project path to gather enable gems for

        :return list of strs of enable gems for project.
    """
    return manifest.get_project_enabled_gems(project_path=project_path, include_dependencies=include_dependencies)


def get_project_info(project_path: str) -> dict or None:
    """
        Call get_project_json_data

        :param project_path: Project path to gather info for

        :return dict with project info. Otherwise None is returned
    """
    return dict()


def get_all_project_infos() -> list:
    """
        Get list of all registered project paths get_all_projects

        Gather together list of dicts for each project using get_project_json_data

        :return list of dicts containing project infos.
    """
    project_paths = manifest.get_all_projects()

    # get all engine info once up front
    engines_json_data = manifest.get_engines_json_data_by_path()

    project_infos = []
    for project_path in project_paths:
        project_json_data = manifest.get_project_json_data(project_path=project_path)
        if not project_json_data:
            continue
        user_project_json_data = manifest.get_project_json_data(project_path=project_path, user=True)
        if user_project_json_data:
            project_json_data.update(user_project_json_data)

        project_json_data['path'] = project_path
        project_json_data['engine_path'] = manifest.get_project_engine_path(project_path=project_path, 
                                                                            project_json_data=project_json_data, 
                                                                            user_project_json_data=user_project_json_data, 
                                                                            engines_json_data=engines_json_data)
        project_infos.append(project_json_data)
    return project_infos


def set_project_info(project_info: dict):
    """
        Call edit_project_props using parameters gathered from project_info

        :param engine_info: dict containing values to change in project
    """
    pass


def get_project_engine_incompatible_objects(project_path: pathlib.Path, engine_path: pathlib.Path = None) -> set() or int:
    """
        Checks for compatibility issues between the provided project and engine

        :param project_path: Project path 
        :param engine_path: Optional engine path 
        :return a set of all incompatible objects which may include APIs and gems or an error code on failure
    """
    engine_path = engine_path or manifest.get_this_engine_path()
    if not manifest.get_engine_json_data(engine_path=engine_path):
        return 1
    if not manifest.get_project_json_data(project_path=project_path):
        return 2

    return compatibility.get_project_engine_incompatible_objects(project_path=project_path, 
                                                                 engine_path=engine_path)


def get_incompatible_project_gems(gem_paths:list, gem_names: list, project_path: str) -> set() or int:
    """
        Checks for compatibility issues between the provided gems and project

        :param gem_paths: Gem paths for the gems to check
        :param gem_names: Gem names with optional version specifiers
        :param project_path: Project path 
        :return a set of all incompatible gems or int error code
    """
    # we need to know the engine to check compatibility 
    engine_path = manifest.get_project_engine_path(project_path)
    if not engine_path:
        return 1

    # check compatibility for all gems at once for speeeeeeeeed
    incompatible_objects = compatibility.get_gems_project_incompatible_objects(
        gem_paths=gem_paths, gem_names=gem_names, project_path=project_path)
    if incompatible_objects:
        logger.error(f"The following dependency issues were found:\n"
            "\n  ".join(incompatible_objects))
    return incompatible_objects


def add_gems_to_project(gem_paths:list, gem_names: list, project_path: str, force: bool = False) -> int:
    """
        Activates gem_path in project_path

        :param gem_names: Gem names to activate
        :param project_path: Project path 
        :return 0 on success, non-zero on error
    """
    if not force:
        # we need to know the engine to check compatibility 
        engine_path = manifest.get_project_engine_path(project_path)
        if engine_path:
            # check compatibility for all gems at once for speeeeeeeeed
            incompatible_objects = compatibility.get_gems_project_incompatible_objects(
                gem_paths=gem_paths, gem_names=gem_names, project_path=project_path)
            if incompatible_objects:
                logger.error(f"The following dependency issues were found:\n"
                    "\n  ".join(incompatible_objects))
                return 1

    # compatibility passed, force add gems to avoid additional compatibility checks
    result = 0
    for index in range(len(gem_paths)):
       result = enable_gem.enable_gem_in_project(gem_name=gem_names[index], 
                                                 gem_path=gem_paths[index], 
                                                 project_path=project_path, 
                                                 force=True) or result

    return result

def remove_gem_from_project(gem_path: str, project_path: str):
    """
        Deactivates gem_path in project_path

        :param gem_path: Gem path to deactivate
        :param project_path: Project path to deactivate
    """
    pass


#### Gem methods ###

def register_gem(gem_path: str, project_path: str = None,):
    """
        Registers gem with engine if only gem_path is provided
        Registers gem to project if path is provided

        :param gem_path: Gem path to register
        :param project_path: Project path to register project gem with
    """
    pass


def unregister_gem(gem_path: str, project_path: str = None,):
    """
        Unregisters gem with engine if only gem_path is provided
        Unregisters gem to project if path is provided

        :param gem_path: Gem path to unregister
        :param project_path: Project path to unregister project gem with
    """
    pass


def get_gem_info(gem_path: str) -> dict or None:
    """
        Call get_gem_json_data

        :param gem_path: Gem path to gather info for

        :return dict with project info. Otherwise None is returned
    """
    return dict()


def get_all_gem_infos(project_path: pathlib.Path or None) -> list:
    """
        Get list of all avaliable gem json data for a project. If no project
        path is provided, the gems for this engine are returned.

        :param project_path: Project path to gather avaliable gem infos for

        :return list of dicts containing gem json data
    """
    # it's easier to determine which gems are engine gems here rather than in c++
    # because the project might be using a different engine than the one Project Manager is
    # running out of

    # Attempt to get the engine path from the project path if possible (except when the project path
    # is a template). If not, then default to attempting to get engine path from the registered 
    # '--this-engine' value from the manifest
    engine_path = None
    if project_path and utils.find_ancestor_dir_containing_file('template.json', project_path) is None:
        engine_path = manifest.get_project_engine_path(project_path=project_path)
    if not engine_path:
        engine_path = manifest.get_this_engine_path()
    if not engine_path:
        logger.error("Failed to get engine path for gem info retrieval")
        return [] 

    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error("Failed to get engine json data for gem info retrieval")
        return [] 

    # get all gem json data by path so we can use it for detecting engine and project gems
    # without re-opening and parsing gem.json files again
    all_gem_json_data = manifest.get_gems_json_data_by_path(engine_path=engine_path,
                                                            project_path=project_path,
                                                            include_engine_gems=True,
                                                            include_manifest_gems=True)
    # include gems inside gems
    recurse = True
    engine_gem_paths = [pathlib.PurePath(path) for path in manifest.get_engine_gems(engine_path, recurse, all_gem_json_data)]
    if project_path:
        project_gem_paths = [pathlib.PurePath(path) for path in manifest.get_project_gems(project_path, recurse, all_gem_json_data)]

    # convert all_gem_json_data to have gem names as keys with values that are gem version lists
    utils.replace_dict_keys_with_value_key(all_gem_json_data, value_key='gem_name', replaced_key_name='path', place_values_in_list=True)

    # flatten into a single list
    all_gem_json_data_list = [gem_json_data for gem_versions in all_gem_json_data.values() for gem_json_data in gem_versions]

    for i, gem_json_data in enumerate(all_gem_json_data_list):
        if project_path:
            if gem_json_data['path'] in project_gem_paths:
                all_gem_json_data_list[i]['project_gem'] = True
        
        if gem_json_data['path'] in engine_gem_paths:
            all_gem_json_data_list[i]['engine_gem'] = True
        else:
            # gather general incompatibility information
            incompatible_engine =  compatibility.get_incompatible_objects_for_engine(gem_json_data, engine_json_data)
            if incompatible_engine:
                all_gem_json_data_list[i]['incompatible_engine'] = incompatible_engine

            incompatible_gem_dependencies = compatibility.get_incompatible_gem_dependencies(gem_json_data, all_gem_json_data)
            if incompatible_gem_dependencies:
                all_gem_json_data_list[i]['incompatible_gem_dependencies'] = incompatible_gem_dependencies
        

    return all_gem_json_data_list

def download_gem(gem_name: str, force_overwrite: bool = False, progress_callback = None):
    """
        Call download_gem

        :param gem_name: Name of gem to download from a repo
        :param force_overwrite: bool if False does not overwrite gem if already downloaded, otherwise overwrites gem
        :param progress_callback: A cpp function callback to update progress
    """
    pass


def is_gem_update_avaliable(gem_name: str, last_updated: str) -> bool:
    """
        Call is_o3de_gem_update_available

        :param gem_name: Name of gem to check for update on
        :param last_updated: Timestamp when gem was last updated, must be parsable using datetime.fromisoformat

        :return bool True if update is avaliable, otherwise False.
    """
    return False


#### Gem Repo methods ###

def register_gem_repo(repo_uri: str):
    """
        Registers gem repo using register

        :param repo_uri: Uri of repo to register
    """
    pass


def unregister_gem_repo(repo_uri: str):
    """
        Unregisters gem repo using register

        :param repo_uri: Uri of repo to unregister
    """
    pass


def get_all_repo_infos() -> list:
    """
        Get list of all registered repo uris get_manifest_repos

        Gather together list of dicts for each repo using get_repo_json_data
        Also insert the repo path from get_repo_path as the path attr into each dict

        :return list of dicts containing repo infos.
    """
    return list()


def get_gem_infos_from_repo(repo_uri: str) -> list:
    """
        Call get_gem_json_paths_from_cached_repo for repo_uri

        Gather together list of dicts for each gem using get_gem_json_data

        :param repo_uri: Uri of repo to gather gemo infos from

        :return list of dicts containing gem infos.
    """
    return list()

def get_gem_infos_from_all_repos(project_path:pathlib.Path = None, enabled_only:bool = True) -> list:
    """
        Call get_gem_json_paths_from_all_cached_repos

        Gather together list of dicts for each gem using get_gem_json_data

        :return list of dicts containing gem infos.
    """
    remote_gem_json_data_list = repo.get_gem_json_data_from_all_cached_repos(enabled_only)
    if not remote_gem_json_data_list:
        return list()

    # Attempt to get the engine path from the project path if possible (except when the project path
    # is a template). If not, then default to attempting to get engine path from the registered 
    # '--this-engine' value from the manifest
    engine_path = None
    if project_path and utils.find_ancestor_dir_containing_file('template.json', project_path) is None:
        engine_path = manifest.get_project_engine_path(project_path=project_path)
    if not engine_path:
        engine_path = manifest.get_this_engine_path()
    if not engine_path:
        logger.error("Failed to get engine path for remote gem compatibility checks")
        return list() 

    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error("Failed to get engine json data remote gem compatibility checks")
        return list() 

    # get all gem json data by path so we can use it for detecting engine and project gems
    # without re-opening and parsing gem.json files again
    all_gem_json_data = manifest.get_gems_json_data_by_path(engine_path=engine_path,
                                                            project_path=project_path,
                                                            include_engine_gems=True,
                                                            include_manifest_gems=True)

    # first add all the known gems together before checking compatibility and dependencies
    for i, gem_json_data in enumerate(remote_gem_json_data_list):
        all_gem_json_data[i] = gem_json_data
    
    # convert all_gem_json_data to have gem names as keys with values that are gem version lists
    utils.replace_dict_keys_with_value_key(all_gem_json_data, value_key='gem_name', replaced_key_name='path', place_values_in_list=True)

    # do general compatibility checks - dependency resolution is too slow for now
    for i, gem_json_data in enumerate(remote_gem_json_data_list):
        incompatible_engine =  compatibility.get_incompatible_objects_for_engine(gem_json_data, engine_json_data)
        if incompatible_engine:
            remote_gem_json_data_list[i]['incompatible_engine_dependencies'] = incompatible_engine

        incompatible_gem_dependencies = compatibility.get_incompatible_gem_dependencies(gem_json_data, all_gem_json_data)
        if incompatible_gem_dependencies:
            remote_gem_json_data_list[i]['incompatible_gem_dependencies'] = incompatible_gem_dependencies

    return remote_gem_json_data_list


def refresh_gem_repo(repo_uri: str):
    """
        Call refresh_repo with repo_uri

        :param repo_uri: Uri of repo to refresh
    """
    pass

def refresh_all_gem_repos():
    """
        Call refresh_repos
    """
    pass

def set_repo_enabled(repo_uri: str, enabled: bool) -> int:
    """
        Call set_repo_enabled 
    """
    return repo.set_repo_enabled(repo_uri, enabled)
