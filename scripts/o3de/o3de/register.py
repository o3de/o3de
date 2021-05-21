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
import urllib.parse
import urllib.request

from o3de import add_gem_cmake, get_registration, manifest, remove_external_subdirectory, repo, utils, validation

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


def register_all_in_folder(folder_path: str or pathlib.Path,
                           remove: bool = False,
                           engine_path: str or pathlib.Path = None,
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


def register_all_engines_in_folder(engines_path: str or pathlib.Path,
                                   remove: bool = False,
                                   force: bool = False) -> int:
    if not engines_path:
        logger.error(f'Engines path cannot be empty.')
        return 1

    engines_path = pathlib.Path(engines_path).resolve()
    if not engines_path.is_dir():
        logger.error(f'Engines path is not dir.')
        return 1

    engines_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(engines_path):
        for name in files:
            if name == 'engine.json':
                engines_set.add(root)

    for engine in sorted(engines_set, reverse=True):
        error_code = register(engine_path=engine, remove=remove, force=force)
        if error_code:
            ret_val = error_code

    return ret_val


def register_all_projects_in_folder(projects_path: str or pathlib.Path,
                                    remove: bool = False,
                                    engine_path: str or pathlib.Path = None) -> int:
    if not projects_path:
        logger.error(f'Projects path cannot be empty.')
        return 1

    projects_path = pathlib.Path(projects_path).resolve()
    if not projects_path.is_dir():
        logger.error(f'Projects path is not dir.')
        return 1

    projects_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(projects_path):
        for name in files:
            if name == 'project.json':
                projects_set.add(root)

    for project in sorted(projects_set, reverse=True):
        error_code = register(engine_path=engine_path, project_path=project, remove=remove)
        if error_code:
            ret_val = error_code

    return ret_val


def register_all_gems_in_folder(gems_path: str or pathlib.Path,
                                remove: bool = False,
                                engine_path: str or pathlib.Path = None) -> int:
    if not gems_path:
        logger.error(f'Gems path cannot be empty.')
        return 1

    gems_path = pathlib.Path(gems_path).resolve()
    if not gems_path.is_dir():
        logger.error(f'Gems path is not dir.')
        return 1

    gems_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(gems_path):
        for name in files:
            if name == 'gem.json':
                gems_set.add(root)

    for gem in sorted(gems_set, reverse=True):
        error_code = register(engine_path=engine_path, gem_path=gem, remove=remove)
        if error_code:
            ret_val = error_code

    return ret_val


def register_all_templates_in_folder(templates_path: str or pathlib.Path,
                                     remove: bool = False,
                                     engine_path: str or pathlib.Path = None) -> int:
    if not templates_path:
        logger.error(f'Templates path cannot be empty.')
        return 1

    templates_path = pathlib.Path(templates_path).resolve()
    if not templates_path.is_dir():
        logger.error(f'Templates path is not dir.')
        return 1

    templates_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(templates_path):
        for name in files:
            if name == 'template.json':
                templates_set.add(root)

    for template in sorted(templates_set, reverse=True):
        error_code = register(engine_path=engine_path, template_path=template, remove=remove)
        if error_code:
            ret_val = error_code

    return ret_val


def register_all_restricted_in_folder(restricted_path: str or pathlib.Path,
                                      remove: bool = False,
                                      engine_path: str or pathlib.Path = None) -> int:
    if not restricted_path:
        logger.error(f'Restricted path cannot be empty.')
        return 1

    restricted_path = pathlib.Path(restricted_path).resolve()
    if not restricted_path.is_dir():
        logger.error(f'Restricted path is not dir.')
        return 1

    restricted_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(restricted_path):
        for name in files:
            if name == 'restricted.json':
                restricted_set.add(root)

    for restricted in sorted(restricted_set, reverse=True):
        error_code = register(engine_path=engine_path, restricted_path=restricted, remove=remove)
        if error_code:
            ret_val = error_code

    return ret_val


def register_all_repos_in_folder(repos_path: str or pathlib.Path,
                                 remove: bool = False,
                                 engine_path: str or pathlib.Path = None) -> int:
    if not repos_path:
        logger.error(f'Repos path cannot be empty.')
        return 1

    repos_path = pathlib.Path(repos_path).resolve()
    if not repos_path.is_dir():
        logger.error(f'Repos path is not dir.')
        return 1

    repo_set = set()

    ret_val = 0
    for root, dirs, files in os.walk(repos_path):
        for name in files:
            if name == 'repo.json':
                repo_set.add(root)

    for repo in sorted(repo_set, reverse=True):
        error_code = register(engine_path=engine_path, repo_uri=repo, remove=remove)
        if error_code:
            ret_val = error_code

    return ret_val


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
            f' To force registration of a new engine path, specify the -f/--force option.')
        return 1
    engines_path_json[engine_name] = engine_path.as_posix()
    return 0


def register_engine_path(json_data: dict,
                         engine_path: str or pathlib.Path,
                         remove: bool = False,
                         force: bool = False) -> int:
    if not engine_path:
        logger.error(f'Engine path cannot be empty.')
        return 1
    engine_path = pathlib.Path(engine_path).resolve()

    for engine_object in json_data.get('engines', {}):
        engine_object_path = pathlib.Path(engine_object['path']).resolve()
        if engine_object_path == engine_path:
            json_data['engines'].remove(engine_object)

    if remove:
        return remove_engine_name_to_path(json_data, engine_path)

    if not engine_path.is_dir():
        logger.error(f'Engine path {engine_path} does not exist.')
        return 1

    engine_json = engine_path / 'engine.json'
    if not validation.valid_o3de_engine_json(engine_json):
        logger.error(f'Engine json {engine_json} is not valid.')
        return 1

    engine_object = {}
    engine_object.update({'path': engine_path.as_posix()})

    json_data.setdefault('engines', []).insert(0, engine_object)

    return add_engine_name_to_path(json_data, engine_path, force)


def register_gem_path(json_data: dict,
                      gem_path: str or pathlib.Path,
                      remove: bool = False,
                      engine_path: str or pathlib.Path = None) -> int:
    if not gem_path:
        logger.error(f'Gem path cannot be empty.')
        return 1
    gem_path = pathlib.Path(gem_path).resolve()

    if engine_path:
        engine_data = manifest.find_engine_data(json_data, engine_path)
        if not engine_data:
            logger.error(f'Engine path {engine_path} is not registered.')
            return 1

        while gem_path in engine_data['gems']:
            engine_data['gems'].remove(gem_path)

        while gem_path.as_posix() in engine_data['gems']:
            engine_data['gems'].remove(gem_path.as_posix())

        if remove:
            logger.warn(f'Removing Gem path {gem_path}.')
            return 0
    else:
        while gem_path in json_data['gems']:
            json_data['gems'].remove(gem_path)

        while gem_path.as_posix() in json_data['gems']:
            json_data['gems'].remove(gem_path.as_posix())

        if remove:
            logger.warn(f'Removing Gem path {gem_path}.')
            return 0

    if not gem_path.is_dir():
        logger.error(f'Gem path {gem_path} does not exist.')
        return 1

    gem_json = gem_path / 'gem.json'
    if not validation.valid_o3de_gem_json(gem_json):
        logger.error(f'Gem json {gem_json} is not valid.')
        return 1

    if engine_path:
        engine_data['gems'].insert(0, gem_path.as_posix())
    else:
        json_data['gems'].insert(0, gem_path.as_posix())

    return 0


def register_project_path(json_data: dict,
                          project_path: str or pathlib.Path,
                          remove: bool = False,
                          engine_path: str or pathlib.Path = None) -> int:
    if not project_path:
        logger.error(f'Project path cannot be empty.')
        return 1
    project_path = pathlib.Path(project_path).resolve()

    if engine_path:
        engine_data = manifest.find_engine_data(json_data, engine_path)
        if not engine_data:
            logger.error(f'Engine path {engine_path} is not registered.')
            return 1

        while project_path in engine_data['projects']:
            engine_data['projects'].remove(project_path)

        while project_path.as_posix() in engine_data['projects']:
            engine_data['projects'].remove(project_path.as_posix())

        if remove:
            logger.warn(f'Engine {engine_path} removing Project path {project_path}.')
            return 0
    else:
        while project_path in json_data['projects']:
            json_data['projects'].remove(project_path)

        while project_path.as_posix() in json_data['projects']:
            json_data['projects'].remove(project_path.as_posix())

        if remove:
            logger.warn(f'Removing Project path {project_path}.')
            return 0

    if not project_path.is_dir():
        logger.error(f'Project path {project_path} does not exist.')
        return 1

    project_json = project_path / 'project.json'
    if not validation.valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return 1

    if engine_path:
        engine_data['projects'].insert(0, project_path.as_posix())
    else:
        json_data['projects'].insert(0, project_path.as_posix())

    # registering a project has the additional step of setting the project.json 'engine' field
    this_engine_json = manifest.get_this_engine_path() / 'engine.json'
    with this_engine_json.open('r') as f:
        try:
            this_engine_json = json.load(f)
        except Exception as e:
            logger.error(f'Engine json failed to load: {str(e)}')
            return 1
    with project_json.open('r') as f:
        try:
            project_json_data = json.load(f)
        except Exception as e:
            logger.error(f'Project json failed to load: {str(e)}')
            return 1

    update_project_json = False
    try:
        update_project_json = project_json_data['engine'] != this_engine_json['engine_name']
    except Exception as e:
        update_project_json = True

    if update_project_json:
        project_json_data['engine'] = this_engine_json['engine_name']
        utils.backup_file(project_json)
        with project_json.open('w') as s:
            try:
                s.write(json.dumps(project_json_data, indent=4))
            except Exception as e:
                logger.error(f'Project json failed to save: {str(e)}')
                return 1

    return 0


def register_template_path(json_data: dict,
                           template_path: str or pathlib.Path,
                           remove: bool = False,
                           engine_path: str or pathlib.Path = None) -> int:
    if not template_path:
        logger.error(f'Template path cannot be empty.')
        return 1
    template_path = pathlib.Path(template_path).resolve()

    if engine_path:
        engine_data = manifest.find_engine_data(json_data, engine_path)
        if not engine_data:
            logger.error(f'Engine path {engine_path} is not registered.')
            return 1

        while template_path in engine_data['templates']:
            engine_data['templates'].remove(template_path)

        while template_path.as_posix() in engine_data['templates']:
            engine_data['templates'].remove(template_path.as_posix())

        if remove:
            logger.warn(f'Engine {engine_path} removing Template path {template_path}.')
            return 0
    else:
        while template_path in json_data['templates']:
            json_data['templates'].remove(template_path)

        while template_path.as_posix() in json_data['templates']:
            json_data['templates'].remove(template_path.as_posix())

        if remove:
            logger.warn(f'Removing Template path {template_path}.')
            return 0

    if not template_path.is_dir():
        logger.error(f'Template path {template_path} does not exist.')
        return 1

    template_json = template_path / 'template.json'
    if not validation.valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_json} is not valid.')
        return 1

    if engine_path:
        engine_data['templates'].insert(0, template_path.as_posix())
    else:
        json_data['templates'].insert(0, template_path.as_posix())

    return 0


def register_restricted_path(json_data: dict,
                             restricted_path: str or pathlib.Path,
                             remove: bool = False,
                             engine_path: str or pathlib.Path = None) -> int:
    if not restricted_path:
        logger.error(f'Restricted path cannot be empty.')
        return 1
    restricted_path = pathlib.Path(restricted_path).resolve()

    if engine_path:
        engine_data = manifest.find_engine_data(json_data, engine_path)
        if not engine_data:
            logger.error(f'Engine path {engine_path} is not registered.')
            return 1

        while restricted_path in engine_data['restricted']:
            engine_data['restricted'].remove(restricted_path)

        while restricted_path.as_posix() in engine_data['restricted']:
            engine_data['restricted'].remove(restricted_path.as_posix())

        if remove:
            logger.warn(f'Engine {engine_path} removing Restricted path {restricted_path}.')
            return 0
    else:
        while restricted_path in json_data['restricted']:
            json_data['restricted'].remove(restricted_path)

        while restricted_path.as_posix() in json_data['restricted']:
            json_data['restricted'].remove(restricted_path.as_posix())

        if remove:
            logger.warn(f'Removing Restricted path {restricted_path}.')
            return 0

    if not restricted_path.is_dir():
        logger.error(f'Restricted path {restricted_path} does not exist.')
        return 1

    restricted_json = restricted_path / 'restricted.json'
    if not validation.valid_o3de_restricted_json(restricted_json):
        logger.error(f'Restricted json {restricted_json} is not valid.')
        return 1

    if engine_path:
        engine_data['restricted'].insert(0, restricted_path.as_posix())
    else:
        json_data['restricted'].insert(0, restricted_path.as_posix())

    return 0


def register_repo(json_data: dict,
                  repo_uri: str or pathlib.Path,
                  remove: bool = False) -> int:
    if not repo_uri:
        logger.error(f'Repo URI cannot be empty.')
        return 1

    url = f'{repo_uri}/repo.json'
    parsed_uri = urllib.parse.urlparse(url)

    if parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        while repo_uri in json_data['repos']:
            json_data['repos'].remove(repo_uri)
    else:
        repo_uri = pathlib.Path(repo_uri).resolve()
        while repo_uri.as_posix() in json_data['repos']:
            json_data['repos'].remove(repo_uri.as_posix())

    if remove:
        logger.warn(f'Removing repo uri {repo_uri}.')
        return 0

    repo_sha256 = hashlib.sha256(url.encode())
    cache_file = manifest.get_o3de_cache_folder() / str(repo_sha256.hexdigest() + '.json')

    result = 0
    if parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        if not cache_file.is_file():
            with urllib.request.urlopen(url) as s:
                with cache_file.open('wb') as f:
                    shutil.copyfileobj(s, f)
        json_data['repos'].insert(0, repo_uri)
    else:
        if not cache_file.is_file():
            origin_file = pathlib.Path(url).resolve()
            if not origin_file.is_file():
                return 1
            shutil.copy(origin_file, origin_file)
        json_data['repos'].insert(0, repo_uri.as_posix())

    repo_set = set()
    result = repo.process_add_o3de_repo(cache_file, repo_set)

    return result


def register_default_engines_folder(json_data: dict,
                                    default_engines_folder: str or pathlib.Path,
                                    remove: bool = False) -> int:
    if remove:
        default_engines_folder = manifest.get_o3de_engines_folder()

    # make sure the path exists
    default_engines_folder = pathlib.Path(default_engines_folder).resolve()
    if not default_engines_folder.is_dir():
        logger.error(f'Default engines folder {default_engines_folder} does not exist.')
        return 1

    default_engines_folder = default_engines_folder.as_posix()
    json_data['default_engines_folder'] = default_engines_folder

    return 0


def register_default_projects_folder(json_data: dict,
                                     default_projects_folder: str or pathlib.Path,
                                     remove: bool = False) -> int:
    if remove:
        default_projects_folder = manifest.get_o3de_projects_folder()

    # make sure the path exists
    default_projects_folder = pathlib.Path(default_projects_folder).resolve()
    if not default_projects_folder.is_dir():
        logger.error(f'Default projects folder {default_projects_folder} does not exist.')
        return 1

    default_projects_folder = default_projects_folder.as_posix()
    json_data['default_projects_folder'] = default_projects_folder

    return 0


def register_default_gems_folder(json_data: dict,
                                 default_gems_folder: str or pathlib.Path,
                                 remove: bool = False) -> int:
    if remove:
        default_gems_folder = manifest.get_o3de_gems_folder()

    # make sure the path exists
    default_gems_folder = pathlib.Path(default_gems_folder).resolve()
    if not default_gems_folder.is_dir():
        logger.error(f'Default gems folder {default_gems_folder} does not exist.')
        return 1

    default_gems_folder = default_gems_folder.as_posix()
    json_data['default_gems_folder'] = default_gems_folder

    return 0


def register_default_templates_folder(json_data: dict,
                                      default_templates_folder: str or pathlib.Path,
                                      remove: bool = False) -> int:
    if remove:
        default_templates_folder = manifest.get_o3de_templates_folder()

    # make sure the path exists
    default_templates_folder = pathlib.Path(default_templates_folder).resolve()
    if not default_templates_folder.is_dir():
        logger.error(f'Default templates folder {default_templates_folder} does not exist.')
        return 1

    default_templates_folder = default_templates_folder.as_posix()
    json_data['default_templates_folder'] = default_templates_folder

    return 0


def register_default_restricted_folder(json_data: dict,
                                       default_restricted_folder: str or pathlib.Path,
                                       remove: bool = False) -> int:
    if remove:
        default_restricted_folder = manifest.get_o3de_restricted_folder()

    # make sure the path exists
    default_restricted_folder = pathlib.Path(default_restricted_folder).resolve()
    if not default_restricted_folder.is_dir():
        logger.error(f'Default restricted folder {default_restricted_folder} does not exist.')
        return 1

    default_restricted_folder = default_restricted_folder.as_posix()
    json_data['default_restricted_folder'] = default_restricted_folder

    return 0


def register(engine_path: str or pathlib.Path = None,
             project_path: str or pathlib.Path = None,
             gem_path: str or pathlib.Path = None,
             template_path: str or pathlib.Path = None,
             restricted_path: str or pathlib.Path = None,
             repo_uri: str or pathlib.Path = None,
             default_engines_folder: str or pathlib.Path = None,
             default_projects_folder: str or pathlib.Path = None,
             default_gems_folder: str or pathlib.Path = None,
             default_templates_folder: str or pathlib.Path = None,
             default_restricted_folder: str or pathlib.Path = None,
             remove: bool = False,
             force: bool = False
             ) -> int:
    """
    Adds/Updates entries to the .o3de/o3de_manifest.json

    :param engine_path: if engine folder is supplied the path will be added to the engine if it can, if not global
    :param project_path: project folder
    :param gem_path: gem folder
    :param template_path: template folder
    :param restricted_path: restricted folder
    :param repo_uri: repo uri
    :param default_engines_folder: default engines folder
    :param default_projects_folder: default projects folder
    :param default_gems_folder: default gems folder
    :param default_templates_folder: default templates folder
    :param default_restricted_folder: default restricted code folder
    :param remove: add/remove the entries
    :param force: force update of the engine_path for specified "engine_name" from the engine.json file

    :return: 0 for success or non 0 failure code
    """

    json_data = manifest.load_o3de_manifest()

    result = 0

    # do anything that could require a engine context first
    if isinstance(project_path, str) or isinstance(project_path, pathlib.PurePath):
        if not project_path:
            logger.error(f'Project path cannot be empty.')
            return 1
        result = register_project_path(json_data, project_path, remove, engine_path)

    elif isinstance(gem_path, str) or isinstance(gem_path, pathlib.PurePath):
        if not gem_path:
            logger.error(f'Gem path cannot be empty.')
            return 1
        result = register_gem_path(json_data, gem_path, remove, engine_path)

    elif isinstance(template_path, str) or isinstance(template_path, pathlib.PurePath):
        if not template_path:
            logger.error(f'Template path cannot be empty.')
            return 1
        result = register_template_path(json_data, template_path, remove, engine_path)

    elif isinstance(restricted_path, str) or isinstance(restricted_path, pathlib.PurePath):
        if not restricted_path:
            logger.error(f'Restricted path cannot be empty.')
            return 1
        result = register_restricted_path(json_data, restricted_path, remove, engine_path)

    elif isinstance(repo_uri, str) or isinstance(repo_uri, pathlib.PurePath):
        if not repo_uri:
            logger.error(f'Repo URI cannot be empty.')
            return 1
        result = register_repo(json_data, repo_uri, remove)

    elif isinstance(default_engines_folder, str) or isinstance(default_engines_folder, pathlib.PurePath):
        result = register_default_engines_folder(json_data, default_engines_folder, remove)

    elif isinstance(default_projects_folder, str) or isinstance(default_projects_folder, pathlib.PurePath):
        result = register_default_projects_folder(json_data, default_projects_folder, remove)

    elif isinstance(default_gems_folder, str) or isinstance(default_gems_folder, pathlib.PurePath):
        result = register_default_gems_folder(json_data, default_gems_folder, remove)

    elif isinstance(default_templates_folder, str) or isinstance(default_templates_folder, pathlib.PurePath):
        result = register_default_templates_folder(json_data, default_templates_folder, remove)

    elif isinstance(default_restricted_folder, str) or isinstance(default_restricted_folder, pathlib.PurePath):
        result = register_default_restricted_folder(json_data, default_restricted_folder, remove)

    # engine is done LAST
    # Now that everything that could have an engine context is done, if the engine is supplied that means this is
    # registering the engine itself
    elif isinstance(engine_path, str) or isinstance(engine_path, pathlib.PurePath):
        if not engine_path:
            logger.error(f'Engine path cannot be empty.')
            return 1
        result = register_engine_path(json_data, engine_path, remove, force)

    if not result:
        manifest.save_o3de_manifest(json_data)

    return result


def remove_invalid_o3de_objects() -> None:
    json_data = manifest.load_o3de_manifest()

    for engine_object in json_data['engines']:
        engine_path = engine_object['path']
        if not validation.valid_o3de_engine_json(pathlib.Path(engine_path).resolve() / 'engine.json'):
            logger.warn(f"Engine path {engine_path} is invalid.")
            register(engine_path=engine_path, remove=True)
        else:
            for project in engine_object['projects']:
                if not validation.valid_o3de_project_json(pathlib.Path(project).resolve() / 'project.json'):
                    logger.warn(f"Project path {project} is invalid.")
                    register(engine_path=engine_path, project_path=project, remove=True)

            for gem_path in engine_object['gems']:
                if not validation.valid_o3de_gem_json(pathlib.Path(gem_path).resolve() / 'gem.json'):
                    logger.warn(f"Gem path {gem_path} is invalid.")
                    register(engine_path=engine_path, gem_path=gem_path, remove=True)

            for template_path in engine_object['templates']:
                if not validation.valid_o3de_template_json(pathlib.Path(template_path).resolve() / 'template.json'):
                    logger.warn(f"Template path {template_path} is invalid.")
                    register(engine_path=engine_path, template_path=template_path, remove=True)

            for restricted in engine_object['restricted']:
                if not validation.valid_o3de_restricted_json(pathlib.Path(restricted).resolve() / 'restricted.json'):
                    logger.warn(f"Restricted path {restricted} is invalid.")
                    register(engine_path=engine_path, restricted_path=restricted, remove=True)

            for external in engine_object['external_subdirectories']:
                external = pathlib.Path(external).resolve()
                if not external.is_dir():
                    logger.warn(f"External subdirectory {external} is invalid.")
                    remove_external_subdirectory.remove_external_subdirectory(external)

    for project in json_data['projects']:
        if not validation.valid_o3de_project_json(pathlib.Path(project).resolve() / 'project.json'):
            logger.warn(f"Project path {project} is invalid.")
            register(project_path=project, remove=True)

    for gem in json_data['gems']:
        if not validation.valid_o3de_gem_json(pathlib.Path(gem).resolve() / 'gem.json'):
            logger.warn(f"Gem path {gem} is invalid.")
            register(gem_path=gem, remove=True)

    for template in json_data['templates']:
        if not validation.valid_o3de_template_json(pathlib.Path(template).resolve() / 'template.json'):
            logger.warn(f"Template path {template} is invalid.")
            register(template_path=template, remove=True)

    for restricted in json_data['restricted']:
        if not validation.valid_o3de_restricted_json(pathlib.Path(restricted).resolve() / 'restricted.json'):
            logger.warn(f"Restricted path {restricted} is invalid.")
            register(restricted_path=restricted, remove=True)

    default_engines_folder = pathlib.Path(json_data['default_engines_folder']).resolve()
    if not default_engines_folder.is_dir():
        new_default_engines_folder = manifest.get_o3de_folder() / 'Engines'
        new_default_engines_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default engines folder {default_engines_folder} is invalid. Set default {new_default_engines_folder}")
        register(default_engines_folder=new_default_engines_folder.as_posix())

    default_projects_folder = pathlib.Path(json_data['default_projects_folder']).resolve()
    if not default_projects_folder.is_dir():
        new_default_projects_folder = manifest.get_o3de_folder() / 'Projects'
        new_default_projects_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default projects folder {default_projects_folder} is invalid. Set default {new_default_projects_folder}")
        register(default_projects_folder=new_default_projects_folder.as_posix())

    default_gems_folder = pathlib.Path(json_data['default_gems_folder']).resolve()
    if not default_gems_folder.is_dir():
        new_default_gems_folder = manifest.get_o3de_folder() / 'Gems'
        new_default_gems_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(f"Default gems folder {default_gems_folder} is invalid."
                    f" Set default {new_default_gems_folder}")
        register(default_gems_folder=new_default_gems_folder.as_posix())

    default_templates_folder = pathlib.Path(json_data['default_templates_folder']).resolve()
    if not default_templates_folder.is_dir():
        new_default_templates_folder = manifest.get_o3de_folder() / 'Templates'
        new_default_templates_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default templates folder {default_templates_folder} is invalid."
            f" Set default {new_default_templates_folder}")
        register(default_templates_folder=new_default_templates_folder.as_posix())

    default_restricted_folder = pathlib.Path(json_data['default_restricted_folder']).resolve()
    if not default_restricted_folder.is_dir():
        default_restricted_folder = manifest.get_o3de_folder() / 'Restricted'
        default_restricted_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default restricted folder {default_restricted_folder} is invalid."
            f" Set default {default_restricted_folder}")
        register(default_restricted_folder=default_restricted_folder.as_posix())


def _run_register(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    if args.update:
        remove_invalid_o3de_objects()
        return repo.refresh_repos()
    elif args.this_engine:
        ret_val = register(engine_path=manifest.get_this_engine_path(), force=args.force)
        error_code = register_shipped_engine_o3de_objects(force=args.force)
        if error_code:
            ret_val = error_code
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
                        template_path=args.template_path,
                        restricted_path=args.restricted_path,
                        repo_uri=args.repo_uri,
                        default_engines_folder=args.default_engines_folder,
                        default_projects_folder=args.default_projects_folder,
                        default_gems_folder=args.default_gems_folder,
                        default_templates_folder=args.default_templates_folder,
                        default_restricted_folder=args.default_restricted_folder,
                        remove=args.remove,
                        force=args.force)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python register.py register --gem-path "C:/TestGem"
    OR
    o3de.py can downloadable commands by importing engine_template,
    call add_args and execute: python o3de.py register --gem-path "C:/TestGem"
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    # register
    register_subparser = subparsers.add_parser('register')
    group = register_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('--this-engine', action='store_true', required=False,
                       default=False,
                       help='Registers the engine this script is running from.')
    group.add_argument('-ep', '--engine-path', type=str, required=False,
                       help='Engine path to register/remove.')
    group.add_argument('-pp', '--project-path', type=str, required=False,
                       help='Project path to register/remove.')
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='Gem path to register/remove.')
    group.add_argument('-tp', '--template-path', type=str, required=False,
                       help='Template path to register/remove.')
    group.add_argument('-rp', '--restricted-path', type=str, required=False,
                       help='A restricted folder to register/remove.')
    group.add_argument('-ru', '--repo-uri', type=str, required=False,
                       help='A repo uri to register/remove.')
    group.add_argument('-aep', '--all-engines-path', type=str, required=False,
                       help='All engines under this folder to register/remove.')
    group.add_argument('-app', '--all-projects-path', type=str, required=False,
                       help='All projects under this folder to register/remove.')
    group.add_argument('-agp', '--all-gems-path', type=str, required=False,
                       help='All gems under this folder to register/remove.')
    group.add_argument('-atp', '--all-templates-path', type=str, required=False,
                       help='All templates under this folder to register/remove.')
    group.add_argument('-arp', '--all-restricted-path', type=str, required=False,
                       help='All templates under this folder to register/remove.')
    group.add_argument('-aru', '--all-repo-uri', type=str, required=False,
                       help='All repos under this folder to register/remove.')
    group.add_argument('-def', '--default-engines-folder', type=str, required=False,
                       help='The default engines folder to register/remove.')
    group.add_argument('-dpf', '--default-projects-folder', type=str, required=False,
                       help='The default projects folder to register/remove.')
    group.add_argument('-dgf', '--default-gems-folder', type=str, required=False,
                       help='The default gems folder to register/remove.')
    group.add_argument('-dtf', '--default-templates-folder', type=str, required=False,
                       help='The default templates folder to register/remove.')
    group.add_argument('-drf', '--default-restricted-folder', type=str, required=False,
                       help='The default restricted folder to register/remove.')
    group.add_argument('-u', '--update', action='store_true', required=False,
                       default=False,
                       help='Refresh the repo cache.')

    register_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                    help='By default the home folder is the user folder, override it to this folder.')

    register_subparser.add_argument('-r', '--remove', action='store_true', required=False,
                                    default=False,
                                    help='Remove entry.')
    register_subparser.add_argument('-f', '--force', action='store_true', default=False,
                                    help='For the update of the registration field being modified.')
    register_subparser.set_defaults(func=_run_register)
