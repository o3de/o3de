
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains all the code that has to do with registering engines, projects, gems and templates
"""

import argparse
import hashlib
import logging
import json
import os
import pathlib
import shutil
import sys
import urllib.parse
import urllib.request

from o3de import get_registration, manifest, repo, utils, validation

logger = logging.getLogger()
logging.basicConfig()


def register_shipped_engine_o3de_objects(force: bool = False) -> int:
    engine_path = manifest.get_this_engine_path()

    ret_val = 0

    # register anything in the users default folders globally
    error_code = register_all_engines_in_folder(manifest.get_registered(default_folder='engines'), force=force)
    if error_code:
        ret_val = error_code
    error_code = register_all_projects_in_folder(manifest.get_registered(default_folder='projects'))
    if error_code:
        ret_val = error_code
    error_code = register_all_gems_in_folder(manifest.get_registered(default_folder='gems'))
    if error_code:
        ret_val = error_code
    error_code = register_all_templates_in_folder(manifest.get_registered(default_folder='templates'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(manifest.get_registered(default_folder='restricted'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(manifest.get_registered(default_folder='projects'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(manifest.get_registered(default_folder='gems'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(manifest.get_registered(default_folder='templates'))
    if error_code:
        ret_val = error_code

    return ret_val


def register_all_in_folder(folder_path: pathlib.Path,
                           remove: bool = False,
                           engine_path: pathlib.Path = None,
                           exclude: list = None) -> int:
    if not folder_path:
        logger.error(f'Folder path cannot be empty.')
        return 1

    folder_path = pathlib.Path(folder_path).resolve()
    if not folder_path.is_dir():
        logger.error(f'Folder path is not dir.')
        return 1

    engines_set = set()
    projects_set = set()
    gems_set = set()
    templates_set = set()
    restricted_set = set()
    repo_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(folder_path):
        if root in exclude:
            continue

        for name in files:
            if name == 'engine.json':
                engines_set.add(root)
            elif name == 'project.json':
                projects_set.add(root)
            elif name == 'gem.json':
                gems_set.add(root)
            elif name == 'template.json':
                templates_set.add(root)
            elif name == 'restricted.json':
                restricted_set.add(root)
            elif name == 'repo.json':
                repo_set.add(root)

    for engine in sorted(engines_set, reverse=True):
        error_code = register(engine_path=engine, remove=remove)
        if error_code:
            ret_val = error_code

    for project in sorted(projects_set, reverse=True):
        error_code = register(engine_path=engine_path, project_path=project, remove=remove)
        if error_code:
            ret_val = error_code

    for gem in sorted(gems_set, reverse=True):
        error_code = register(engine_path=engine_path, gem_path=gem, remove=remove)
        if error_code:
            ret_val = error_code

    for template in sorted(templates_set, reverse=True):
        error_code = register(engine_path=engine_path, template_path=template, remove=remove)
        if error_code:
            ret_val = error_code

    for restricted in sorted(restricted_set, reverse=True):
        error_code = register(engine_path=engine_path, restricted_path=restricted, remove=remove)
        if error_code:
            ret_val = error_code

    for repo in sorted(repo_set, reverse=True):
        error_code = register(engine_path=engine_path, repo_uri=repo, remove=remove)
        if error_code:
            ret_val = error_code

    return ret_val


def register_all_o3de_objects_of_type_in_folder(o3de_object_path: pathlib.Path,
                                                o3de_object_type: str,
                                                remove: bool,
                                                force: bool,
                                                stop_iteration_callable: callable,
                                                **register_kwargs) -> int:
    if not o3de_object_path:
        logger.error(f'Engines path cannot be empty.')
        return 1

    o3de_object_path = pathlib.Path(o3de_object_path).resolve()
    if not o3de_object_path.is_dir():
        logger.error(f'Engines path is not dir.')
        return 1

    o3de_object_type_set = set()
    register_path_kwarg = f'{o3de_object_type}_path' if o3de_object_type != 'repo' else f'{o3de_object_type}_uri'

    ret_val = 0
    for root, dirs, files in os.walk(o3de_object_path):
        # Skip subdirectories where the stop iteration callback is true
        if stop_iteration_callable and stop_iteration_callable(dirs, files):
            dirs[:] = []
        if f'{o3de_object_type}.json' in files:
            o3de_object_type_set.add(pathlib.Path(root))
            # Stop iteration of any subdirectories
            # Nested o3de objects of the same type aren't supported(i.e an engine cannot be inside of a engine).
            dirs[:] = []

    for o3de_object_type_root in sorted(o3de_object_type_set, reverse=True):
        error_code = register(**{register_path_kwarg: o3de_object_type_root},
                              remove=remove, force=force, **register_kwargs)
        if error_code:
            ret_val = error_code

    return ret_val


def stop_on_template_folders(dirs: list, files: list) -> bool:
    return 'template.json' in files


def register_all_engines_in_folder(engines_path: pathlib.Path,
                                   remove: bool = False,
                                   force: bool = False) -> int:
    return register_all_o3de_objects_of_type_in_folder(engines_path, 'engine', remove, force, None)


def register_all_projects_in_folder(projects_path: pathlib.Path,
                                    remove: bool = False,
                                    engine_path: pathlib.Path = None) -> int:
    return register_all_o3de_objects_of_type_in_folder(projects_path, 'project', remove, False,
                                                       stop_on_template_folders, engine_path=engine_path)


def register_all_gems_in_folder(gems_path: pathlib.Path,
                                remove: bool = False,
                                engine_path: pathlib.Path = None,
                                project_path: pathlib.Path = None) -> int:
    return register_all_o3de_objects_of_type_in_folder(gems_path, 'gem', remove, False, stop_on_template_folders,
                                                       engine_path=engine_path)


def register_all_templates_in_folder(templates_path: pathlib.Path,
                                     remove: bool = False,
                                     engine_path: pathlib.Path = None) -> int:
    return register_all_o3de_objects_of_type_in_folder(templates_path, 'template', remove, False, None,
                                                       engine_path=engine_path)


def register_all_restricted_in_folder(restricted_path: pathlib.Path,
                                      remove: bool = False,
                                      engine_path: pathlib.Path = None) -> int:
    return register_all_o3de_objects_of_type_in_folder(restricted_path, 'restricted', remove, False, None,
                                                       engine_path=engine_path)


def register_all_repos_in_folder(repos_path: pathlib.Path,
                                 remove: bool = False,
                                 engine_path: pathlib.Path = None) -> int:
    return register_all_o3de_objects_of_type_in_folder(repos_path, 'repo', remove, force, None, engine_path=engine_path)


def remove_engine_name_to_path(json_data: dict,
                               engine_path: pathlib.Path) -> int:
    """
    Remove the engine at the specified path if it exist in the o3de manifest
    :param json_data in-memory json view of the o3de_manifest.json data
    :param engine_path path to engine to remove from the manifest data

    returns 0 to indicate no issues has occurred with removal
    """
    if engine_path.is_dir() and validation.valid_o3de_engine_json(engine_path):
        engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
        if 'engine_name' in engine_json_data and 'engines_path' in json_data:
            engine_name = engine_json_data['engine_name']
            try:
                del json_data['engines_path'][engine_name]
            except KeyError:
                # Attempting to remove a non-existent engine_name is fine
                pass
    return 0


def add_engine_name_to_path(json_data: dict, engine_path: pathlib.Path, force: bool):
    # Add an engine path JSON object which maps the "engine_name" -> "engine_path"
    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error(f'Unable to retrieve json data from engine.json at path {engine_path.as_posix()}')
        return 1
    engines_path_json = json_data.setdefault('engines_path', {})
    if 'engine_name' not in engine_json_data:
        logger.error(f'engine.json at path {engine_path.as_posix()} is missing "engine_name" key')
        return 1

    engine_name = engine_json_data['engine_name']
    if not force and engine_name in engines_path_json and \
            pathlib.PurePath(engines_path_json[engine_name]) != engine_path:
        logger.error(
            f'Attempting to register existing engine "{engine_name}" with a new path of {engine_path.as_posix()}.'
            f' The current path is {pathlib.Path(engines_path_json[engine_name]).as_posix()}.'
            f' To force registration of a new engine path, specify the -f/--force option.'
            f'\nAlternatively the engine can be registered with a different name by changing the "engine_name" field in the engine.json.')
        return 1
    engines_path_json[engine_name] = engine_path.as_posix()
    return 0


def register_o3de_object_path(json_data: dict,
                              o3de_object_path: str or pathlib.Path,
                              o3de_object_key: str,
                              o3de_json_filename: str,
                              validation_func: callable,
                              remove: bool = False,
                              engine_path: pathlib.Path = None,
                              project_path: pathlib.Path = None) -> int:
    # save_path variable is used to save the changes to the store the path to the file to save
    # if the registration is for the project or engine
    save_path = None

    if not o3de_object_path:
        logger.error(f'o3de object path cannot be empty.')
        return 1

    o3de_object_path = pathlib.Path(o3de_object_path).resolve()

    if engine_path and project_path:
        logger.error(f'Both a project path: {project_path} and engine path: {engine_path} has been supplied.'
                     'A subdirectory can only be registered to either the engine path or project in one command')

    manifest_data = None
    if engine_path:
        manifest_data = manifest.get_engine_json_data(engine_path=engine_path)
        if not manifest_data:
            logger.error(f'Cannot load engine.json data at path {engine_path}')
            return 1

        save_path = (engine_path / 'engine.json').resolve()
    elif project_path:
        manifest_data = manifest.get_project_json_data(project_path=project_path)
        if not manifest_data:
            logger.error(f'Cannot load project.json data at path {project_path}')
            return 1

        save_path = (project_path / 'project.json').resolve()
    else:
        manifest_data = json_data

    paths_to_remove = [o3de_object_path]
    if save_path:
        try:
            paths_to_remove.append(o3de_object_path.relative_to(save_path.parent))
        except ValueError:
            pass # It is not an error if a relative path cannot be formed
    manifest_data[o3de_object_key] = list(filter(lambda p: pathlib.Path(p) not in paths_to_remove,
                                                           manifest_data.setdefault(o3de_object_key, [])))

    if remove:
        if save_path:
            manifest.save_o3de_manifest(manifest_data, save_path)
        return 0

    if not o3de_object_path.is_dir():
        logger.error(f'o3de object path {o3de_object_path} does not exist.')
        return 1

    manifest_json_path = o3de_object_path / o3de_json_filename
    if validation_func and not validation_func(manifest_json_path):
        logger.error(f'Manifest at path {manifest_json_path} is not valid.')
        return 1

    # if there is a save path make it relative the directory containing o3de object json file
    if save_path:
        try:
            o3de_object_path = o3de_object_path.relative_to(save_path.parent)
        except ValueError:
            pass # It is OK relative path cannot be formed
    manifest_data[o3de_object_key].insert(0, o3de_object_path.as_posix())
    if save_path:
        manifest.save_o3de_manifest(manifest_data, save_path)

    return 0


def register_engine_path(json_data: dict,
                         engine_path: pathlib.Path,
                         remove: bool = False,
                         force: bool = False) -> int:
    # If the o3de_manifest.json 'engines' key is list containing dictionary entries, transform it to a list of strings
    engine_list = json_data.get('engines', [])

    def transform_engine_dict_to_string(engine): return engine.get('path', '') if isinstance(engine, dict) else engine
    json_data['engines'] = list(map(transform_engine_dict_to_string, engine_list))

    result = register_o3de_object_path(json_data, engine_path, 'engines', 'engine.json',
                                       validation.valid_o3de_engine_json, remove)
    if result != 0:
        return result

    if remove:
        return remove_engine_name_to_path(json_data, engine_path)

    return add_engine_name_to_path(json_data, engine_path, force)


def register_external_subdirectory(json_data: dict,
                                   external_subdir_path: pathlib.Path,
                                   remove: bool = False,
                                   engine_path: pathlib.Path = None,
                                   project_path: pathlib.Path = None) -> int:
    """
    :return An integer return code indicating whether registration or removal of the external subdirectory
    completed successfully
    """
    # If a project path or engine path has not been supplied auto detect which manifest to register the input path with
    if not project_path and not engine_path:
        project_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), external_subdir_path)
        if not project_path:
            engine_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('engine.json'), external_subdir_path)
    return register_o3de_object_path(json_data, external_subdir_path, 'external_subdirectories', '', None, remove,
                                     pathlib.Path(engine_path).resolve() if engine_path else None,
                                     pathlib.Path(project_path).resolve() if project_path else None)


def register_gem_path(json_data: dict,
                      gem_path: pathlib.Path,
                      remove: bool = False,
                      engine_path: pathlib.Path = None,
                      project_path:  pathlib.Path = None) -> int:
    # If a project path or engine path has not been supplied auto detect which manifest to register the input path with
    if not project_path and not engine_path:
        project_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), gem_path)
        if not project_path:
            engine_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('engine.json'), gem_path)
    return register_o3de_object_path(json_data, gem_path, 'external_subdirectories', 'gem.json',
                                     validation.valid_o3de_gem_json, remove,
                                     pathlib.Path(engine_path).resolve() if engine_path else None,
                                     pathlib.Path(project_path).resolve() if project_path else None)


def register_project_path(json_data: dict,
                          project_path: pathlib.Path,
                          remove: bool = False,
                          engine_path: pathlib.Path = None) -> int:
    # If an engine path has not been supplied auto detect if the project should be register with the engine.json
    # or the ~/.o3de/o3de_manifest.json
    if not engine_path:
        engine_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('engine.json'), project_path)

    result = register_o3de_object_path(json_data, project_path, 'projects', 'project.json',
                                       validation.valid_o3de_project_json, remove,
                                       pathlib.Path(engine_path).resolve() if engine_path else None)

    if result != 0:
        return result

    if not remove:
        # registering a project has the additional step of setting the project.json 'engine' field
        this_engine_json = manifest.get_engine_json_data(engine_path=manifest.get_this_engine_path())
        if not this_engine_json:
            return 1
        project_json_data = manifest.get_project_json_data(project_path=project_path)
        if not project_json_data:
            return 1

        update_project_json = False
        try:
            update_project_json = project_json_data['engine'] != this_engine_json['engine_name']
        except KeyError as e:
            update_project_json = True

        if update_project_json:
            project_json_path = project_path / 'project.json'
            project_json_data['engine'] = this_engine_json['engine_name']
            utils.backup_file(project_json_path)
            if not manifest.save_o3de_manifest(project_json_data, project_json_path):
                return 1

    return 0


def register_template_path(json_data: dict,
                           template_path: pathlib.Path,
                           remove: bool = False,
                           project_path: pathlib.Path = None,
                           engine_path: pathlib.Path = None) -> int:
    # If a project path or engine path has not been supplied auto detect which manifest to register the input path
    if not project_path and not engine_path:
        project_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), template_path)
        if not project_path:
            engine_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('engine.json'), template_path)
    return register_o3de_object_path(json_data, template_path, 'templates', 'template.json',
                                     validation.valid_o3de_template_json, remove,
                                     pathlib.Path(engine_path).resolve() if engine_path else None,
                                     pathlib.Path(project_path).resolve() if project_path else None)


def register_restricted_path(json_data: dict,
                             restricted_path: pathlib.Path,
                             remove: bool = False,
                             project_path: pathlib.Path = None,
                             engine_path: pathlib.Path = None) -> int:
    # If a project path or engine path has not been supplied auto detect which manifest to register the input path
    if not project_path and not engine_path:
        project_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), restricted_path)
        if not project_path:
            engine_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('engine.json'), restricted_path)
    return register_o3de_object_path(json_data, restricted_path, 'restricted', 'restricted.json',
                                     validation.valid_o3de_restricted_json, remove,
                                     pathlib.Path(engine_path).resolve() if engine_path else None,
                                     pathlib.Path(project_path).resolve() if project_path else None)


def register_repo(json_data: dict,
                  repo_uri: str,
                  remove: bool = False) -> int:
    if not repo_uri:
        logger.error(f'Repo URI cannot be empty.')
        return 1

    url = f'{repo_uri}/repo.json'
    parsed_uri = urllib.parse.urlparse(url)

    if parsed_uri.scheme in ['http', 'https', 'ftp', 'ftps']:
        while repo_uri in json_data.get('repos', []):
            json_data['repos'].remove(repo_uri)
    else:
        repo_uri = pathlib.Path(repo_uri).resolve().as_posix()
        while repo_uri in json_data.get('repos', []):
            json_data['repos'].remove(repo_uri)

    if remove:
        logger.warning(f'Removing repo uri {repo_uri}.')
        return 0
    repo_sha256 = hashlib.sha256(url.encode())
    cache_file = manifest.get_o3de_cache_folder() / str(repo_sha256.hexdigest() + '.json')

    result = utils.download_file(parsed_uri, cache_file, True)
    if result == 0:
        json_data.setdefault('repos', []).insert(0, repo_uri)

    repo_set = set()
    result = repo.process_add_o3de_repo(cache_file, repo_set)

    return result


def register_default_o3de_object_folder(json_data: dict,
                                        default_o3de_object_folder: pathlib.Path,
                                        o3de_object_key: str) -> int:
    # make sure the path exists
    default_o3de_object_folder = pathlib.Path(default_o3de_object_folder).resolve()
    if not default_o3de_object_folder.is_dir():
        logger.error(f'Default o3de object folder {default_o3de_object_folder} does not exist.')
        return 1

    json_data[o3de_object_key] = default_o3de_object_folder.as_posix()

    return 0


def register_default_engines_folder(json_data: dict,
                                    default_engines_folder: pathlib.Path,
                                    remove: bool = False) -> int:
    return register_default_o3de_object_folder(json_data,
                                               manifest.get_o3de_engines_folder() if remove else default_engines_folder,
                                               'default_engines_folder')


def register_default_projects_folder(json_data: dict,
                                     default_projects_folder: pathlib.Path,
                                     remove: bool = False) -> int:
    return register_default_o3de_object_folder(json_data,
                                               manifest.get_o3de_projects_folder() if remove else default_projects_folder,
                                               'default_projects_folder')


def register_default_gems_folder(json_data: dict,
                                 default_gems_folder: pathlib.Path,
                                 remove: bool = False) -> int:
    return register_default_o3de_object_folder(json_data,
                                               manifest.get_o3de_gems_folder() if remove else default_gems_folder,
                                               'default_gems_folder')


def register_default_templates_folder(json_data: dict,
                                      default_templates_folder: pathlib.Path,
                                      remove: bool = False) -> int:
    return register_default_o3de_object_folder(json_data,
                                               manifest.get_o3de_templates_folder() if remove else default_templates_folder,
                                               'default_templates_folder')


def register_default_restricted_folder(json_data: dict,
                                       default_restricted_folder: pathlib.Path,
                                       remove: bool = False) -> int:
    return register_default_o3de_object_folder(json_data,
                                               manifest.get_o3de_restricted_folder() if remove else default_restricted_folder,
                                               'default_restricted_folder')

def register_default_third_party_folder(json_data: dict,
                                       default_third_party_folder: pathlib.Path,
                                       remove: bool = False) -> int:
    return register_default_o3de_object_folder(json_data,
                                               manifest.get_o3de_third_party_folder() if remove else default_third_party_folder,
                                               'default_third_party_folder')


def remove_invalid_o3de_projects(manifest_path: pathlib.Path = None) -> int:
    if not manifest_path:
        manifest_path = manifest.get_o3de_manifest()

    json_data = manifest.load_o3de_manifest(manifest_path)

    result = 0

    for project in json_data.get('projects', []):
        if not validation.valid_o3de_project_json(pathlib.Path(project).resolve() / 'project.json'):
            logger.warning(f"Project path {project} is invalid.")
            # Attempt to unregister all invalid projects even if previous projects failed to unregister
            # but combine the result codes of each command.
            result = register(project_path=pathlib.Path(project), remove=True) or result

    return result


def remove_invalid_o3de_objects() -> None:
    for engine_path in manifest.get_engines():
        if not validation.valid_o3de_engine_json(pathlib.Path(engine_path).resolve() / 'engine.json'):
            logger.warning(f"Engine path {engine_path} is invalid.")
            register(engine_path=engine_path, remove=True)

    remove_invalid_o3de_projects()

    for external in manifest.get_external_subdirectories():
        external = pathlib.Path(external).resolve()
        if not external.is_dir():
            logger.warning(f"External subdirectory {external} is invalid.")
            register(engine_path=engine_path, external_subdir_path=external, remove=True)

    for template in manifest.get_templates():
        if not validation.valid_o3de_template_json(pathlib.Path(template).resolve() / 'template.json'):
            logger.warning(f"Template path {template} is invalid.")
            register(template_path=template, remove=True)

    for restricted in manifest.get_restricted():
        if not validation.valid_o3de_restricted_json(pathlib.Path(restricted).resolve() / 'restricted.json'):
            logger.warning(f"Restricted path {restricted} is invalid.")
            register(restricted_path=restricted, remove=True)

    json_data = manifest.load_o3de_manifest()
    default_engines_folder = pathlib.Path(
        json_data.get('default_engines_folder', manifest.get_o3de_engines_folder())).resolve()
    if not default_engines_folder.is_dir():
        new_default_engines_folder = manifest.get_o3de_folder() / 'Engines'
        new_default_engines_folder.mkdir(parents=True, exist_ok=True)
        logger.warning(
            f"Default engines folder {default_engines_folder} is invalid. Set default {new_default_engines_folder}")
        register(default_engines_folder=new_default_engines_folder.as_posix())

    default_projects_folder = pathlib.Path(
        json_data.get('default_projects_folder', manifest.get_o3de_projects_folder())).resolve()
    if not default_projects_folder.is_dir():
        new_default_projects_folder = manifest.get_o3de_folder() / 'Projects'
        new_default_projects_folder.mkdir(parents=True, exist_ok=True)
        logger.warning(
            f"Default projects folder {default_projects_folder} is invalid. Set default {new_default_projects_folder}")
        register(default_projects_folder=new_default_projects_folder.as_posix())

    default_gems_folder = pathlib.Path(json_data.get('default_gems_folder', manifest.get_o3de_gems_folder())).resolve()
    if not default_gems_folder.is_dir():
        new_default_gems_folder = manifest.get_o3de_folder() / 'Gems'
        new_default_gems_folder.mkdir(parents=True, exist_ok=True)
        logger.warning(f"Default gems folder {default_gems_folder} is invalid."
                       f" Set default {new_default_gems_folder}")
        register(default_gems_folder=new_default_gems_folder.as_posix())

    default_templates_folder = pathlib.Path(
        json_data.get('default_templates_folder', manifest.get_o3de_templates_folder())).resolve()
    if not default_templates_folder.is_dir():
        new_default_templates_folder = manifest.get_o3de_folder() / 'Templates'
        new_default_templates_folder.mkdir(parents=True, exist_ok=True)
        logger.warning(
            f"Default templates folder {default_templates_folder} is invalid."
            f" Set default {new_default_templates_folder}")
        register(default_templates_folder=new_default_templates_folder.as_posix())

    default_restricted_folder = pathlib.Path(
        json_data.get('default_restricted_folder', manifest.get_o3de_restricted_folder())).resolve()
    if not default_restricted_folder.is_dir():
        default_restricted_folder = manifest.get_o3de_folder() / 'Restricted'
        default_restricted_folder.mkdir(parents=True, exist_ok=True)
        logger.warning(
            f"Default restricted folder {default_restricted_folder} is invalid."
            f" Set default {default_restricted_folder}")
        register(default_restricted_folder=default_restricted_folder.as_posix())

    default_third_party_folder = pathlib.Path(
        json_data.get('default_third_party_folder', manifest.get_o3de_third_party_folder())).resolve()
    if not default_third_party_folder.is_dir():
        default_third_party_folder = manifest.get_o3de_folder() / '3rdParty'
        default_third_party_folder.mkdir(parents=True, exist_ok=True)
        logger.warning(
            f"Default 3rd Party folder {default_third_party_folder} is invalid."
            f" Set default {default_third_party_folder}")
        register(default_third_party_folder=default_third_party_folder.as_posix())


def register(engine_path: pathlib.Path = None,
             project_path: pathlib.Path = None,
             gem_path: pathlib.Path = None,
             external_subdir_path: pathlib.Path = None,
             template_path: pathlib.Path = None,
             restricted_path: pathlib.Path = None,
             repo_uri: str = None,
             default_engines_folder: pathlib.Path = None,
             default_projects_folder: pathlib.Path = None,
             default_gems_folder: pathlib.Path = None,
             default_templates_folder: pathlib.Path = None,
             default_restricted_folder: pathlib.Path = None,
             default_third_party_folder: pathlib.Path = None,
             external_subdir_engine_path: pathlib.Path = None,
             external_subdir_project_path: pathlib.Path = None,
             remove: bool = False,
             force: bool = False
             ) -> int:
    """
    Adds/Updates entries to the ~/.o3de/o3de_manifest.json

    :param engine_path: if engine folder is supplied the path will be added to the engine if it can, if not global
    :param project_path: project folder
    :param gem_path: gem folder
    :param external_subdir_path: external subdirectory
    :param template_path: template folder
    :param restricted_path: restricted folder
    :param repo_uri: repo uri
    :param default_engines_folder: default engines folder
    :param default_projects_folder: default projects folder
    :param default_gems_folder: default gems folder
    :param default_templates_folder: default templates folder
    :param default_restricted_folder: default restricted code folder
    :param default_third_party_folder: default 3rd party cache folder
    :param external_subdir_engine_path: Path to the engine to use when registering an external subdirectory.
     The registration occurs in the engine.json file in this case
    :param external_subdir_project_path: Path to the project to use when registering an external subdirectory.
     The registrations occurs in the project.json in this case
    :param remove: add/remove the entries
    :param force: force update of the engine_path for specified "engine_name" from the engine.json file

    :return: 0 for success or non 0 failure code
    """

    try:
        json_data = manifest.load_o3de_manifest()
    except json.JSONDecodeError:
        if not force:
            logger.error('O3DE object registration has halted due to JSON Decode Error in manifest at path:'
                         f' "{manifest.get_o3de_manifest()}".'
                         '\n      Registration can be forced using the --force option,'
                         ' but that will result in the manifest using default data')
            return 1
        else:
            # Use a default manifest data an proceed
            json_data = manifest.get_default_o3de_manifest_json_data()

    result = 0

    # do anything that could require a engine context first
    if isinstance(project_path, pathlib.PurePath):
        if not project_path:
            logger.error(f'Project path cannot be empty.')
            return 1
        result = result or register_project_path(json_data, project_path, remove, engine_path)

    if isinstance(gem_path, pathlib.PurePath):
        if not gem_path:
            logger.error(f'Gem path cannot be empty.')
            return 1
        result = result or register_gem_path(json_data, gem_path, remove,
                                   external_subdir_engine_path, external_subdir_project_path)

    if isinstance(external_subdir_path, pathlib.PurePath):
        if not external_subdir_path:
            logger.error(f'External Subdirectory path is None.')
            return 1
        result = result or register_external_subdirectory(json_data, external_subdir_path, remove,
                                                external_subdir_engine_path, external_subdir_project_path)

    if isinstance(template_path, pathlib.PurePath):
        if not template_path:
            logger.error(f'Template path cannot be empty.')
            return 1
        result = result or register_template_path(json_data, template_path, remove, project_path, engine_path)

    if isinstance(restricted_path, pathlib.PurePath):
        if not restricted_path:
            logger.error(f'Restricted path cannot be empty.')
            return 1
        result = result or register_restricted_path(json_data, restricted_path, remove, project_path, engine_path)

    if isinstance(repo_uri, str):
        if not repo_uri:
            logger.error(f'Repo URI cannot be empty.')
            return 1
        result = result or register_repo(json_data, repo_uri, remove)

    if isinstance(default_engines_folder, pathlib.PurePath):
        result = result or register_default_engines_folder(json_data, default_engines_folder, remove)

    if isinstance(default_projects_folder, pathlib.PurePath):
        result = result or register_default_projects_folder(json_data, default_projects_folder, remove)

    if isinstance(default_gems_folder, pathlib.PurePath):
        result = result or register_default_gems_folder(json_data, default_gems_folder, remove)

    if isinstance(default_templates_folder, pathlib.PurePath):
        result = result or register_default_templates_folder(json_data, default_templates_folder, remove)

    if isinstance(default_restricted_folder, pathlib.PurePath):
        result = result or register_default_restricted_folder(json_data, default_restricted_folder, remove)

    if isinstance(default_third_party_folder, pathlib.PurePath):
        result = result or register_default_third_party_folder(json_data, default_third_party_folder, remove)

    # engine is done LAST
    # Now that everything that could have an engine context is done, if the engine is supplied that means this is
    # registering the engine itself
    if isinstance(engine_path, pathlib.PurePath):
        if not engine_path:
            logger.error(f'Engine path cannot be empty.')
            return 1
        result = result or register_engine_path(json_data, engine_path, remove, force)

    if not result:
        manifest.save_o3de_manifest(json_data)

    return result


def _run_register(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    if args.update:
        remove_invalid_o3de_objects()
        return repo.refresh_repos()
    elif args.this_engine:
        ret_val = register(engine_path=manifest.get_this_engine_path(), force=args.force)
        return ret_val
    elif args.all_engines_path:
        return register_all_engines_in_folder(args.all_engines_path, args.remove, args.force)
    elif args.all_projects_path:
        return register_all_projects_in_folder(args.all_projects_path, args.remove)
    elif args.all_gems_path:
        return register_all_gems_in_folder(args.all_gems_path, args.remove)
    elif args.all_templates_path:
        return register_all_templates_in_folder(args.all_templates_path, args.remove)
    elif args.all_restricted_path:
        return register_all_restricted_in_folder(args.all_restricted_path, args.remove)
    elif args.all_repo_uri:
        return register_all_repos_in_folder(args.all_restricted_path, args.remove)
    else:
        return register(engine_path=args.engine_path,
                        project_path=args.project_path,
                        gem_path=args.gem_path,
                        external_subdir_path=args.external_subdirectory,
                        template_path=args.template_path,
                        restricted_path=args.restricted_path,
                        repo_uri=args.repo_uri,
                        default_engines_folder=args.default_engines_folder,
                        default_projects_folder=args.default_projects_folder,
                        default_gems_folder=args.default_gems_folder,
                        default_templates_folder=args.default_templates_folder,
                        default_restricted_folder=args.default_restricted_folder,
                        default_third_party_folder=args.default_third_party_folder,
                        external_subdir_engine_path=args.external_subdirectory_engine_path,
                        external_subdir_project_path=args.external_subdirectory_project_path,
                        remove=args.remove,
                        force=args.force)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python register.py --engine-path "C:/o3de"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--this-engine', action='store_true', required=False,
                       default=False,
                       help='Registers the engine this script is running from.')
    group.add_argument('-ep', '--engine-path', type=pathlib.Path, required=False,
                       help='Engine path to register/remove.')
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='Project path to register/remove.')
    group.add_argument('-gp', '--gem-path', type=pathlib.Path, required=False,
                       help='Gem path to register/remove.')
    group.add_argument('-es', '--external-subdirectory', type=pathlib.Path, required=False,
                       help='External subdirectory path to register/remove.')
    group.add_argument('-tp', '--template-path', type=pathlib.Path, required=False,
                       help='Template path to register/remove.')
    group.add_argument('-rp', '--restricted-path', type=pathlib.Path, required=False,
                       help='A restricted folder to register/remove.')
    group.add_argument('-ru', '--repo-uri', type=str, required=False,
                       help='A repo uri to register/remove.')
    group.add_argument('-aep', '--all-engines-path', type=pathlib.Path, required=False,
                       help='All engines under this folder to register/remove.')
    group.add_argument('-app', '--all-projects-path', type=pathlib.Path, required=False,
                       help='All projects under this folder to register/remove.')
    group.add_argument('-agp', '--all-gems-path', type=pathlib.Path, required=False,
                       help='All gems under this folder to register/remove.')
    group.add_argument('-atp', '--all-templates-path', type=pathlib.Path, required=False,
                       help='All templates under this folder to register/remove.')
    group.add_argument('-arp', '--all-restricted-path', type=pathlib.Path, required=False,
                       help='All templates under this folder to register/remove.')
    group.add_argument('-aru', '--all-repo-uri', type=pathlib.Path, required=False,
                       help='All repos under this folder to register/remove.')
    group.add_argument('-def', '--default-engines-folder', type=pathlib.Path, required=False,
                       help='The default engines folder to register/remove.')
    group.add_argument('-dpf', '--default-projects-folder', type=pathlib.Path, required=False,
                       help='The default projects folder to register/remove.')
    group.add_argument('-dgf', '--default-gems-folder', type=pathlib.Path, required=False,
                       help='The default gems folder to register/remove.')
    group.add_argument('-dtf', '--default-templates-folder', type=pathlib.Path, required=False,
                       help='The default templates folder to register/remove.')
    group.add_argument('-drf', '--default-restricted-folder', type=pathlib.Path, required=False,
                       help='The default restricted folder to register/remove.')
    group.add_argument('-dtpf', '--default-third-party-folder', type=pathlib.Path, required=False,
                       help='The default 3rd Party folder to register/remove.')
    group.add_argument('-u', '--update', action='store_true', required=False,
                       default=False,
                       help='Refresh the repo cache.')

    parser.add_argument('-ohf', '--override-home-folder', type=pathlib.Path, required=False,
                                    help='By default the home folder is the user folder, override it to this folder.')
    parser.add_argument('-r', '--remove', action='store_true', required=False,
                                    default=False,
                                    help='Remove entry.')
    parser.add_argument('-f', '--force', action='store_true', default=False,
                                    help='For the update of the registration field being modified.')

    external_subdir_group =  parser.add_argument_group(title='external-subdirectory',
                                                       description='path arguments to use with the --external-subdirectory option')
    external_subdir_path_group = external_subdir_group.add_mutually_exclusive_group()
    external_subdir_path_group.add_argument('-esep', '--external-subdirectory-engine-path', type=pathlib.Path,
                                       help='If supplied, registers the external subdirectory with the engine.json at' \
                                            ' the engine-path location')
    external_subdir_path_group.add_argument('-espp', '--external-subdirectory-project-path', type=pathlib.Path)
    parser.set_defaults(func=_run_register)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py register --engine-path "C:/o3de"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    register_subparser = subparsers.add_parser('register')
    add_parser_args(register_subparser)


def main():
    """
    Runs register.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
