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

from o3de import cmake, disable_gem, download, enable_gem, engine_properties, engine_template, manifest, project_properties, register, repo

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

def register_project(project_path: str):
    """
        Registers project with engine

        :param project_path: Project path to register
    """
    pass


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


def get_enabled_gem_names(project_path: str) -> list:
    """
        Call get_enabled_gem_cmake_file for project_path

        Return get_enabled_gems for cmake file

        :param project_path: Project path to gather enable gems for

        :return list of strs of enable gems for project.
    """
    return list()


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
    return list()


def set_project_info(project_info: dict):
    """
        Call edit_project_props using parameters gathered from project_info

        :param engine_info: dict containing values to change in project
    """
    pass


def add_gem_to_project(gem_path: str, project_path: str):
    """
        Activates gem_path in project_path

        :param gem_path: Gem path to activate
        :param project_path: Project path to activate
    """
    pass

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


def get_all_gem_infos(project_path: str) -> list:
    """
        Get list of all avaliable gem paths for project at project_path using get_all_gems

        Gather together list of dicts for each gem using get_gem_json_data

        :param project_path: Project path to gather avaliable gem infos for

        :return list of dicts containing gem infos.
    """
    return list()


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


def get_gem_infos_from_all_repos() -> list:
    """
        Call get_gem_json_paths_from_all_cached_repos

        Gather together list of dicts for each gem using get_gem_json_data

        :return list of dicts containing gem infos.
    """
    return list()


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
