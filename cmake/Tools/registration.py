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
import logging
import os
import sys
import json
import pathlib
import hashlib
import shutil
import zipfile
import urllib.parse
import urllib.request

logger = logging.getLogger()
logging.basicConfig()


def backup_file(file_name: str or pathlib.Path) -> None:
    index = 0
    renamed = False
    while not renamed:
        backup_file_name = pathlib.Path(str(file_name) + '.bak' + str(index)).resolve()
        index += 1
        if not backup_file_name.is_file():
            file_name = pathlib.Path(file_name).resolve()
            file_name.rename(backup_file_name)
            if backup_file_name.is_file():
                renamed = True


def backup_folder(folder: str or pathlib.Path) -> None:
    index = 0
    renamed = False
    while not renamed:
        backup_folder_name = pathlib.Path(str(folder) + '.bak' + str(index)).resolve()
        index += 1
        if not backup_folder_name.is_dir():
            folder = pathlib.Path(folder).resolve()
            folder.rename(backup_folder_name)
            if backup_folder_name.is_dir():
                renamed = True


def get_this_engine_path() -> pathlib.Path:
    return pathlib.Path(os.path.realpath(__file__)).parents[2].resolve()


override_home_folder = None


def get_home_folder() -> pathlib.Path:
    if override_home_folder:
        return pathlib.Path(override_home_folder).resolve()
    else:
        return pathlib.Path(os.path.expanduser("~")).resolve()


def get_o3de_folder() -> pathlib.Path:
    o3de_folder = get_home_folder() / '.o3de'
    o3de_folder.mkdir(parents=True, exist_ok=True)
    return o3de_folder


def get_o3de_registry_folder() -> pathlib.Path:
    registry_folder = get_o3de_folder() / 'Registry'
    registry_folder.mkdir(parents=True, exist_ok=True)
    return registry_folder


def get_o3de_cache_folder() -> pathlib.Path:
    cache_folder = get_o3de_folder() / 'Cache'
    cache_folder.mkdir(parents=True, exist_ok=True)
    return cache_folder


def get_o3de_download_folder() -> pathlib.Path:
    download_folder = get_o3de_folder() / 'Download'
    download_folder.mkdir(parents=True, exist_ok=True)
    return download_folder


def get_o3de_engines_folder() -> pathlib.Path:
    engines_folder = get_o3de_folder() / 'Engines'
    engines_folder.mkdir(parents=True, exist_ok=True)
    return engines_folder


def get_o3de_projects_folder() -> pathlib.Path:
    projects_folder = get_o3de_folder() / 'Projects'
    projects_folder.mkdir(parents=True, exist_ok=True)
    return projects_folder


def get_o3de_gems_folder() -> pathlib.Path:
    gems_folder = get_o3de_folder() / 'Gems'
    gems_folder.mkdir(parents=True, exist_ok=True)
    return gems_folder


def get_o3de_templates_folder() -> pathlib.Path:
    templates_folder = get_o3de_folder() / 'Templates'
    templates_folder.mkdir(parents=True, exist_ok=True)
    return templates_folder


def get_o3de_restricted_folder() -> pathlib.Path:
    restricted_folder = get_o3de_folder() / 'Restricted'
    restricted_folder.mkdir(parents=True, exist_ok=True)
    return restricted_folder


def get_o3de_logs_folder() -> pathlib.Path:
    restricted_folder = get_o3de_folder() / 'Logs'
    restricted_folder.mkdir(parents=True, exist_ok=True)
    return restricted_folder


def register_shipped_engine_o3de_objects() -> int:
    engine_path = get_this_engine_path()

    ret_val = 0

    # directories with engines
    starting_engines_directories = [
    ]
    for engines_directory in sorted(starting_engines_directories, reverse=True):
        error_code = register_all_engines_in_folder(engines_path=engines_directory)
        if error_code:
            ret_val = error_code

    # specific engines
    starting_engines = [
    ]
    for engine_path in sorted(starting_engines):
        error_code = register(engine_path=engine_path)
        if error_code:
            ret_val = error_code

    # directories with projects
    starting_projects_directories = [
    ]
    for projects_directory in sorted(starting_projects_directories, reverse=True):
        error_code = register_all_projects_in_folder(engine_path=engine_path, projects_path=projects_directory)
        if error_code:
            ret_val = error_code

    # specific projects
    starting_projects = [
        f'{engine_path}/AutomatedTesting'
    ]
    for project_path in sorted(starting_projects, reverse=True):
        error_code = register(engine_path=engine_path, project_path=project_path)
        if error_code:
            ret_val = error_code

    # directories with gems
    starting_gems_directories = [
        f'{engine_path}/Gems'
    ]
    for gems_directory in sorted(starting_gems_directories, reverse=True):
        error_code = register_all_gems_in_folder(engine_path=engine_path, gems_path=gems_directory)
        if error_code:
            ret_val = error_code

    # specific gems
    starting_gems = [
    ]
    for gem_path in sorted(starting_gems, reverse=True):
        error_code = register(engine_path=engine_path, gem_path=gem_path)
        if error_code:
            ret_val = error_code

    # directories with templates
    starting_templates_directories = [
        f'{engine_path}/Templates'
    ]
    for templates_directory in sorted(starting_templates_directories, reverse=True):
        error_code = register_all_templates_in_folder(engine_path=engine_path, templates_path=templates_directory)
        if error_code:
            ret_val = error_code

    # specific templates
    starting_templates = [
    ]
    for template_path in sorted(starting_templates, reverse=True):
        error_code = register(engine_path=engine_path, template_path=template_path)
        if error_code:
            ret_val = error_code

    # directories with restricted
    starting_restricted_directories = [
    ]
    for restricted_directory in sorted(starting_restricted_directories, reverse=True):
        error_code = register_all_restricted_in_folder(engine_path=engine_path, restricted_path=restricted_directory)
        if error_code:
            ret_val = error_code

    # specific restricted
    starting_restricted = [
    ]
    for restricted_path in sorted(starting_restricted, reverse=True):
        error_code = register(engine_path=engine_path, restricted_path=restricted_path)
        if error_code:
            ret_val = error_code

    # directories with repos
    starting_repo_directories = [
    ]
    for repos_directory in sorted(starting_repo_directories, reverse=True):
        error_code = register_all_repos_in_folder(engine_path=engine_path, repos_path=repos_directory)
        if error_code:
            ret_val = error_code

    # specific repos
    starting_repos = [
    ]
    for repo_uri in sorted(starting_repos, reverse=True):
        error_code = register(repo_uri=repo_uri)
        if error_code:
            ret_val = error_code

    # register anything in the users default folders globally
    error_code = register_all_engines_in_folder(get_registered(default_folder='engines'))
    if error_code:
        ret_val = error_code
    error_code = register_all_projects_in_folder(get_registered(default_folder='projects'))
    if error_code:
        ret_val = error_code
    error_code = register_all_gems_in_folder(get_registered(default_folder='gems'))
    if error_code:
        ret_val = error_code
    error_code = register_all_templates_in_folder(get_registered(default_folder='templates'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(get_registered(default_folder='restricted'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(get_registered(default_folder='projects'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(get_registered(default_folder='gems'))
    if error_code:
        ret_val = error_code
    error_code = register_all_restricted_in_folder(get_registered(default_folder='templates'))
    if error_code:
        ret_val = error_code

    starting_external_subdirectories = [
        f'{engine_path}/Gems/Atom',
        f'{engine_path}/Gems/AtomLyIntegration'
    ]
    for external_subdir in sorted(starting_external_subdirectories, reverse=True):
        error_code = add_external_subdirectory(engine_path=engine_path, external_subdir=external_subdir)
        if error_code:
            ret_val = error_code

    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    gems = json_data['gems'].copy()
    gems.extend(engine_object['gems'])
    for gem_path in sorted(gems, key=len):
        gem_path = pathlib.Path(gem_path).resolve()
        gem_cmake_lists_txt = gem_path / 'CMakeLists.txt'
        if gem_cmake_lists_txt.is_file():
            add_gem_to_cmake(engine_path=engine_path, gem_path=gem_path, supress_errors=True)  # don't care about errors

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
                                   remove: bool = False) -> int:
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
                engines_set.add(name)

    for engine in sorted(engines_set, reverse=True):
        error_code = register(engine_path=engine, remove=remove)
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


def get_o3de_manifest() -> pathlib.Path:
    manifest_path = get_o3de_folder() / 'o3de_manifest.json'
    if not manifest_path.is_file():
        username = os.path.split(get_home_folder())[-1]

        o3de_folder = get_o3de_folder()
        default_registry_folder = get_o3de_registry_folder()
        default_cache_folder = get_o3de_cache_folder()
        default_downloads_folder = get_o3de_download_folder()
        default_logs_folder = get_o3de_logs_folder()
        default_engines_folder = get_o3de_engines_folder()
        default_projects_folder = get_o3de_projects_folder()
        default_gems_folder = get_o3de_gems_folder()
        default_templates_folder = get_o3de_templates_folder()
        default_restricted_folder = get_o3de_restricted_folder()

        default_projects_restricted_folder = default_projects_folder / 'Restricted'
        default_projects_restricted_folder.mkdir(parents=True, exist_ok=True)
        default_gems_restricted_folder = default_gems_folder / 'Restricted'
        default_gems_restricted_folder.mkdir(parents=True, exist_ok=True)
        default_templates_restricted_folder = default_templates_folder / 'Restricted'
        default_templates_restricted_folder.mkdir(parents=True, exist_ok=True)

        json_data = {}
        json_data.update({'o3de_manifest_name': f'{username}'})
        json_data.update({'origin': o3de_folder.as_posix()})
        json_data.update({'default_engines_folder': default_engines_folder.as_posix()})
        json_data.update({'default_projects_folder': default_projects_folder.as_posix()})
        json_data.update({'default_gems_folder': default_gems_folder.as_posix()})
        json_data.update({'default_templates_folder': default_templates_folder.as_posix()})
        json_data.update({'default_restricted_folder': default_restricted_folder.as_posix()})

        json_data.update({'projects': []})
        json_data.update({'gems': []})
        json_data.update({'templates': []})
        json_data.update({'restricted': []})
        json_data.update({'repos': []})
        json_data.update({'engines': []})

        default_restricted_folder_json = default_restricted_folder / 'restricted.json'
        if not default_restricted_folder_json.is_file():
            with default_restricted_folder_json.open('w') as s:
                restricted_json_data = {}
                restricted_json_data.update({'restricted_name': 'o3de'})
                s.write(json.dumps(restricted_json_data, indent=4))
        json_data.update({'default_restricted_folder': default_restricted_folder.as_posix()})

        default_projects_restricted_folder_json = default_projects_restricted_folder / 'restricted.json'
        if not default_projects_restricted_folder_json.is_file():
            with default_projects_restricted_folder_json.open('w') as s:
                restricted_json_data = {}
                restricted_json_data.update({'restricted_name': 'projects'})
                s.write(json.dumps(restricted_json_data, indent=4))

        default_gems_restricted_folder_json = default_gems_restricted_folder / 'restricted.json'
        if not default_gems_restricted_folder_json.is_file():
            with default_gems_restricted_folder_json.open('w') as s:
                restricted_json_data = {}
                restricted_json_data.update({'restricted_name': 'gems'})
                s.write(json.dumps(restricted_json_data, indent=4))

        default_templates_restricted_folder_json = default_templates_restricted_folder / 'restricted.json'
        if not default_templates_restricted_folder_json.is_file():
            with default_templates_restricted_folder_json.open('w') as s:
                restricted_json_data = {}
                restricted_json_data.update({'restricted_name': 'templates'})
                s.write(json.dumps(restricted_json_data, indent=4))

        with manifest_path.open('w') as s:
            s.write(json.dumps(json_data, indent=4))

    return manifest_path


def load_o3de_manifest() -> dict:
    with get_o3de_manifest().open('r') as f:
        try:
            json_data = json.load(f)
        except Exception as e:
            logger.error(f'Manifest json failed to load: {str(e)}')
        else:
            return json_data


def save_o3de_manifest(json_data: dict) -> None:
    with get_o3de_manifest().open('w') as s:
        try:
            s.write(json.dumps(json_data, indent=4))
        except Exception as e:
            logger.error(f'Manifest json failed to save: {str(e)}')


def register_engine_path(json_data: dict,
                         engine_path: str or pathlib.Path,
                         remove: bool = False) -> int:
    if not engine_path:
        logger.error(f'Engine path cannot be empty.')
        return 1
    engine_path = pathlib.Path(engine_path).resolve()

    for engine_object in json_data['engines']:
        engine_object_path = pathlib.Path(engine_object['path']).resolve()
        if engine_object_path == engine_path:
            json_data['engines'].remove(engine_object)

    if remove:
        return 0

    if not engine_path.is_dir():
        logger.error(f'Engine path {engine_path} does not exist.')
        return 1

    engine_json = engine_path / 'engine.json'
    if not valid_o3de_engine_json(engine_json):
        logger.error(f'Engine json {engine_json} is not valid.')
        return 1

    engine_object = {}
    engine_object.update({'path': engine_path.as_posix()})
    engine_object.update({'projects': []})
    engine_object.update({'gems': []})
    engine_object.update({'templates': []})
    engine_object.update({'restricted': []})
    engine_object.update({'external_subdirectories': []})

    json_data['engines'].insert(0, engine_object)

    return 0


def register_gem_path(json_data: dict,
                      gem_path: str or pathlib.Path,
                      remove: bool = False,
                      engine_path: str or pathlib.Path = None) -> int:
    if not gem_path:
        logger.error(f'Gem path cannot be empty.')
        return 1
    gem_path = pathlib.Path(gem_path).resolve()

    if engine_path:
        engine_data = find_engine_data(json_data, engine_path)
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
    if not valid_o3de_gem_json(gem_json):
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
        engine_data = find_engine_data(json_data, engine_path)
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
    if not valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return 1

    if engine_path:
        engine_data['projects'].insert(0, project_path.as_posix())
    else:
        json_data['projects'].insert(0, project_path.as_posix())

    # registering a project has the additional step of setting the project.json 'engine' field
    this_engine_json = get_this_engine_path() / 'engine.json'
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
        backup_file(project_json)
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
        engine_data = find_engine_data(json_data, engine_path)
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
    if not valid_o3de_template_json(template_json):
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
        engine_data = find_engine_data(json_data, engine_path)
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
    if not valid_o3de_restricted_json(restricted_json):
        logger.error(f'Restricted json {restricted_json} is not valid.')
        return 1

    if engine_path:
        engine_data['restricted'].insert(0, restricted_path.as_posix())
    else:
        json_data['restricted'].insert(0, restricted_path.as_posix())

    return 0


def valid_o3de_repo_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['repo_name']
            test = json_data['origin']
        except Exception as e:
            return False

    return True


def valid_o3de_engine_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['engine_name']
            # test = json_data['origin'] # will be required soon
        except Exception as e:
            return False
    return True


def valid_o3de_project_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['project_name']
            # test = json_data['origin'] # will be required soon
        except Exception as e:
            return False
    return True


def valid_o3de_gem_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['gem_name']
            # test = json_data['origin'] # will be required soon
        except Exception as e:
            return False
    return True


def valid_o3de_template_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False
    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['template_name']
            # test = json_data['origin'] # will be required soon
        except Exception as e:
            return False
    return True


def valid_o3de_restricted_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False
    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['restricted_name']
            # test = json_data['origin'] # will be required soon
        except Exception as e:
            return False
    return True


def process_add_o3de_repo(file_name: str or pathlib.Path,
                          repo_set: set) -> int:
    file_name = pathlib.Path(file_name).resolve()
    if not valid_o3de_repo_json(file_name):
        return 1

    cache_folder = get_o3de_cache_folder()

    with file_name.open('r') as f:
        try:
            repo_data = json.load(f)
        except Exception as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return 1

        for engine_uri in repo_data['engines']:
            engine_uri = f'{engine_uri}/engine.json'
            engine_sha256 = hashlib.sha256(engine_uri.encode())
            cache_file = cache_folder / str(engine_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(engine_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(engine_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    engine_json = pathlib.Path(engine_uri).resolve()
                    if not engine_json.is_file():
                        return 1
                    shutil.copy(engine_json, cache_file)

        for project_uri in repo_data['projects']:
            project_uri = f'{project_uri}/project.json'
            project_sha256 = hashlib.sha256(project_uri.encode())
            cache_file = cache_folder / str(project_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(project_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(project_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    project_json = pathlib.Path(project_uri).resolve()
                    if not project_json.is_file():
                        return 1
                    shutil.copy(project_json, cache_file)

        for gem_uri in repo_data['gems']:
            gem_uri = f'{gem_uri}/gem.json'
            gem_sha256 = hashlib.sha256(gem_uri.encode())
            cache_file = cache_folder / str(gem_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(gem_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(gem_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    gem_json = pathlib.Path(gem_uri).resolve()
                    if not gem_json.is_file():
                        return 1
                    shutil.copy(gem_json, cache_file)

        for template_uri in repo_data['templates']:
            template_uri = f'{template_uri}/template.json'
            template_sha256 = hashlib.sha256(template_uri.encode())
            cache_file = cache_folder / str(template_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(template_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(template_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    template_json = pathlib.Path(template_uri).resolve()
                    if not template_json.is_file():
                        return 1
                    shutil.copy(template_json, cache_file)

        for repo_uri in repo_data['repos']:
            if repo_uri not in repo_set:
                repo_set.add(repo_uri)
                repo_uri = f'{repo_uri}/repo.json'
                repo_sha256 = hashlib.sha256(repo_uri.encode())
                cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
                if not cache_file.is_file():
                    if parsed_uri.scheme == 'http' or \
                            parsed_uri.scheme == 'https' or \
                            parsed_uri.scheme == 'ftp' or \
                            parsed_uri.scheme == 'ftps':
                        with urllib.request.urlopen(repo_uri) as s:
                            with cache_file.open('wb') as f:
                                shutil.copyfileobj(s, f)
                    else:
                        repo_json = pathlib.Path(repo_uri).resolve()
                        if not repo_json.is_file():
                            return 1
                        shutil.copy(repo_json, cache_file)
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
    cache_file = get_o3de_cache_folder() / str(repo_sha256.hexdigest() + '.json')

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
    result = process_add_o3de_repo(cache_file, repo_set)

    return result


def register_default_engines_folder(json_data: dict,
                                    default_engines_folder: str or pathlib.Path,
                                    remove: bool = False) -> int:
    if remove:
        default_engines_folder = get_o3de_engines_folder()

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
        default_projects_folder = get_o3de_projects_folder()

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
        default_gems_folder = get_o3de_gems_folder()

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
        default_templates_folder = get_o3de_templates_folder()

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
        default_restricted_folder = get_o3de_restricted_folder()

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
             remove: bool = False
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

    :return: 0 for success or non 0 failure code
    """

    json_data = load_o3de_manifest()

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
        result = register_engine_path(json_data, engine_path, remove)

    if not result:
        save_o3de_manifest(json_data)

    return result


def remove_invalid_o3de_objects() -> None:
    json_data = load_o3de_manifest()

    for engine_object in json_data['engines']:
        engine_path = engine_object['path']
        if not valid_o3de_engine_json(pathlib.Path(engine_path).resolve() / 'engine.json'):
            logger.warn(f"Engine path {engine_path} is invalid.")
            register(engine_path=engine_path, remove=True)
        else:
            for project in engine_object['projects']:
                if not valid_o3de_project_json(pathlib.Path(project).resolve() / 'project.json'):
                    logger.warn(f"Project path {project} is invalid.")
                    register(engine_path=engine_path, project_path=project, remove=True)

            for gem_path in engine_object['gems']:
                if not valid_o3de_gem_json(pathlib.Path(gem_path).resolve() / 'gem.json'):
                    logger.warn(f"Gem path {gem_path} is invalid.")
                    register(engine_path=engine_path, gem_path=gem_path, remove=True)

            for template_path in engine_object['templates']:
                if not valid_o3de_template_json(pathlib.Path(template_path).resolve() / 'template.json'):
                    logger.warn(f"Template path {template_path} is invalid.")
                    register(engine_path=engine_path, template_path=template_path, remove=True)

            for restricted in engine_object['restricted']:
                if not valid_o3de_restricted_json(pathlib.Path(restricted).resolve() / 'restricted.json'):
                    logger.warn(f"Restricted path {restricted} is invalid.")
                    register(engine_path=engine_path, restricted_path=restricted, remove=True)

            for external in engine_object['external_subdirectories']:
                external = pathlib.Path(external).resolve()
                if not external.is_dir():
                    logger.warn(f"External subdirectory {external} is invalid.")
                    remove_external_subdirectory(external)

    for project in json_data['projects']:
        if not valid_o3de_project_json(pathlib.Path(project).resolve() / 'project.json'):
            logger.warn(f"Project path {project} is invalid.")
            register(project_path=project, remove=True)

    for gem in json_data['gems']:
        if not valid_o3de_gem_json(pathlib.Path(gem).resolve() / 'gem.json'):
            logger.warn(f"Gem path {gem} is invalid.")
            register(gem_path=gem, remove=True)

    for template in json_data['templates']:
        if not valid_o3de_template_json(pathlib.Path(template).resolve() / 'template.json'):
            logger.warn(f"Template path {template} is invalid.")
            register(template_path=template, remove=True)

    for restricted in json_data['restricted']:
        if not valid_o3de_restricted_json(pathlib.Path(restricted).resolve() / 'restricted.json'):
            logger.warn(f"Restricted path {restricted} is invalid.")
            register(restricted_path=restricted, remove=True)

    default_engines_folder = pathlib.Path(json_data['default_engines_folder']).resolve()
    if not default_engines_folder.is_dir():
        new_default_engines_folder = get_o3de_folder() / 'Engines'
        new_default_engines_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default engines folder {default_engines_folder} is invalid. Set default {new_default_engines_folder}")
        register(default_engines_folder=new_default_engines_folder.as_posix())

    default_projects_folder = pathlib.Path(json_data['default_projects_folder']).resolve()
    if not default_projects_folder.is_dir():
        new_default_projects_folder = get_o3de_folder() / 'Projects'
        new_default_projects_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default projects folder {default_projects_folder} is invalid. Set default {new_default_projects_folder}")
        register(default_projects_folder=new_default_projects_folder.as_posix())

    default_gems_folder = pathlib.Path(json_data['default_gems_folder']).resolve()
    if not default_gems_folder.is_dir():
        new_default_gems_folder = get_o3de_folder() / 'Gems'
        new_default_gems_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(f"Default gems folder {default_gems_folder} is invalid."
                    f" Set default {new_default_gems_folder}")
        register(default_gems_folder=new_default_gems_folder.as_posix())

    default_templates_folder = pathlib.Path(json_data['default_templates_folder']).resolve()
    if not default_templates_folder.is_dir():
        new_default_templates_folder = get_o3de_folder() / 'Templates'
        new_default_templates_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default templates folder {default_templates_folder} is invalid."
            f" Set default {new_default_templates_folder}")
        register(default_templates_folder=new_default_templates_folder.as_posix())

    default_restricted_folder = pathlib.Path(json_data['default_restricted_folder']).resolve()
    if not default_restricted_folder.is_dir():
        default_restricted_folder = get_o3de_folder() / 'Restricted'
        default_restricted_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(
            f"Default restricted folder {default_restricted_folder} is invalid."
            f" Set default {default_restricted_folder}")
        register(default_restricted_folder=default_restricted_folder.as_posix())


def refresh_repos() -> int:
    json_data = load_o3de_manifest()

    # clear the cache
    cache_folder = get_o3de_cache_folder()
    shutil.rmtree(cache_folder)
    cache_folder = get_o3de_cache_folder()  # will recreate it

    result = 0

    # set will stop circular references
    repo_set = set()

    for repo_uri in json_data['repos']:
        if repo_uri not in repo_set:
            repo_set.add(repo_uri)

            repo_uri = f'{repo_uri}/repo.json'
            repo_sha256 = hashlib.sha256(repo_uri.encode())
            cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(repo_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(repo_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    origin_file = pathlib.Path(repo_uri).resolve()
                    if not origin_file.is_file():
                        return 1
                    shutil.copy(origin_file, cache_file)

                if not valid_o3de_repo_json(cache_file):
                    logger.error(f'Repo json {repo_uri} is not valid.')
                    cache_file.unlink()
                    return 1

                last_failure = process_add_o3de_repo(cache_file, repo_set)
                if last_failure:
                    result = last_failure

    return result


def search_repo(repo_set: set,
                repo_json_data: dict,
                engine_name: str = None,
                project_name: str = None,
                gem_name: str = None,
                template_name: str = None,
                restricted_name: str = None) -> dict or None:
    cache_folder = get_o3de_cache_folder()

    if isinstance(engine_name, str) or isinstance(engine_name, pathlib.PurePath):
        for engine_uri in repo_json_data['engines']:
            engine_uri = f'{engine_uri}/engine.json'
            engine_sha256 = hashlib.sha256(engine_uri.encode())
            engine_cache_file = cache_folder / str(engine_sha256.hexdigest() + '.json')
            if engine_cache_file.is_file():
                with engine_cache_file.open('r') as f:
                    try:
                        engine_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{engine_cache_file} failed to load: {str(e)}')
                    else:
                        if engine_json_data['engine_name'] == engine_name:
                            return engine_json_data

    elif isinstance(project_name, str) or isinstance(project_name, pathlib.PurePath):
        for project_uri in repo_json_data['projects']:
            project_uri = f'{project_uri}/project.json'
            project_sha256 = hashlib.sha256(project_uri.encode())
            project_cache_file = cache_folder / str(project_sha256.hexdigest() + '.json')
            if project_cache_file.is_file():
                with project_cache_file.open('r') as f:
                    try:
                        project_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{project_cache_file} failed to load: {str(e)}')
                    else:
                        if project_json_data['project_name'] == project_name:
                            return project_json_data

    elif isinstance(gem_name, str) or isinstance(gem_name, pathlib.PurePath):
        for gem_uri in repo_json_data['gems']:
            gem_uri = f'{gem_uri}/gem.json'
            gem_sha256 = hashlib.sha256(gem_uri.encode())
            gem_cache_file = cache_folder / str(gem_sha256.hexdigest() + '.json')
            if gem_cache_file.is_file():
                with gem_cache_file.open('r') as f:
                    try:
                        gem_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{gem_cache_file} failed to load: {str(e)}')
                    else:
                        if gem_json_data['gem_name'] == gem_name:
                            return gem_json_data

    elif isinstance(template_name, str) or isinstance(template_name, pathlib.PurePath):
        for template_uri in repo_json_data['templates']:
            template_uri = f'{template_uri}/template.json'
            template_sha256 = hashlib.sha256(template_uri.encode())
            template_cache_file = cache_folder / str(template_sha256.hexdigest() + '.json')
            if template_cache_file.is_file():
                with template_cache_file.open('r') as f:
                    try:
                        template_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{template_cache_file} failed to load: {str(e)}')
                    else:
                        if template_json_data['template_name'] == template_name:
                            return template_json_data

    elif isinstance(restricted_name, str) or isinstance(restricted_name, pathlib.PurePath):
        for restricted_uri in repo_json_data['restricted']:
            restricted_uri = f'{restricted_uri}/restricted.json'
            restricted_sha256 = hashlib.sha256(restricted_uri.encode())
            restricted_cache_file = cache_folder / str(restricted_sha256.hexdigest() + '.json')
            if restricted_cache_file.is_file():
                with restricted_cache_file.open('r') as f:
                    try:
                        restricted_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{restricted_cache_file} failed to load: {str(e)}')
                    else:
                        if restricted_json_data['restricted_name'] == restricted_name:
                            return restricted_json_data
    # recurse
    else:
        for repo_repo_uri in repo_json_data['repos']:
            if repo_repo_uri not in repo_set:
                repo_set.add(repo_repo_uri)
                repo_repo_uri = f'{repo_repo_uri}/repo.json'
                repo_repo_sha256 = hashlib.sha256(repo_repo_uri.encode())
                repo_repo_cache_file = cache_folder / str(repo_repo_sha256.hexdigest() + '.json')
                if repo_repo_cache_file.is_file():
                    with repo_repo_cache_file.open('r') as f:
                        try:
                            repo_repo_json_data = json.load(f)
                        except Exception as e:
                            logger.warn(f'{repo_repo_cache_file} failed to load: {str(e)}')
                        else:
                            item = search_repo(repo_set,
                                               repo_repo_json_data,
                                               engine_name,
                                               project_name,
                                               gem_name,
                                               template_name)
                            if item:
                                return item
    return None


def get_downloadable(engine_name: str = None,
                     project_name: str = None,
                     gem_name: str = None,
                     template_name: str = None,
                     restricted_name: str = None) -> dict or None:
    json_data = load_o3de_manifest()
    cache_folder = get_o3de_cache_folder()
    repo_set = set()
    for repo_uri in json_data['repos']:
        if repo_uri not in repo_set:
            repo_set.add(repo_uri)
            repo_uri = f'{repo_uri}/repo.json'
            repo_sha256 = hashlib.sha256(repo_uri.encode())
            repo_cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
            if repo_cache_file.is_file():
                with repo_cache_file.open('r') as f:
                    try:
                        repo_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{repo_cache_file} failed to load: {str(e)}')
                    else:
                        item = search_repo(repo_set,
                                           repo_json_data,
                                           engine_name,
                                           project_name,
                                           gem_name,
                                           template_name,
                                           restricted_name)
                        if item:
                            return item
    return None


def get_registered(engine_name: str = None,
                   project_name: str = None,
                   gem_name: str = None,
                   template_name: str = None,
                   default_folder: str = None,
                   repo_name: str = None,
                   restricted_name: str = None) -> pathlib.Path or None:
    json_data = load_o3de_manifest()

    # check global first then this engine
    if isinstance(engine_name, str):
        for engine in json_data['engines']:
            engine_path = pathlib.Path(engine['path']).resolve()
            engine_json = engine_path / 'engine.json'
            with engine_json.open('r') as f:
                try:
                    engine_json_data = json.load(f)
                except Exception as e:
                    logger.warn(f'{engine_json} failed to load: {str(e)}')
                else:
                    this_engines_name = engine_json_data['engine_name']
                    if this_engines_name == engine_name:
                        return engine_path

    elif isinstance(project_name, str):
        engine_object = find_engine_data(json_data)
        projects = json_data['projects'].copy()
        projects.extend(engine_object['projects'])
        for project_path in projects:
            project_path = pathlib.Path(project_path).resolve()
            project_json = project_path / 'project.json'
            with project_json.open('r') as f:
                try:
                    project_json_data = json.load(f)
                except Exception as e:
                    logger.warn(f'{project_json} failed to load: {str(e)}')
                else:
                    this_projects_name = project_json_data['project_name']
                    if this_projects_name == project_name:
                        return project_path

    elif isinstance(gem_name, str):
        engine_object = find_engine_data(json_data)
        gems = json_data['gems'].copy()
        gems.extend(engine_object['gems'])
        for gem_path in gems:
            gem_path = pathlib.Path(gem_path).resolve()
            gem_json = gem_path / 'gem.json'
            with gem_json.open('r') as f:
                try:
                    gem_json_data = json.load(f)
                except Exception as e:
                    logger.warn(f'{gem_json} failed to load: {str(e)}')
                else:
                    this_gems_name = gem_json_data['gem_name']
                    if this_gems_name == gem_name:
                        return gem_path

    elif isinstance(template_name, str):
        engine_object = find_engine_data(json_data)
        templates = json_data['templates'].copy()
        templates.extend(engine_object['templates'])
        for template_path in templates:
            template_path = pathlib.Path(template_path).resolve()
            template_json = template_path / 'template.json'
            with template_json.open('r') as f:
                try:
                    template_json_data = json.load(f)
                except Exception as e:
                    logger.warn(f'{template_path} failed to load: {str(e)}')
                else:
                    this_templates_name = template_json_data['template_name']
                    if this_templates_name == template_name:
                        return template_path

    elif isinstance(restricted_name, str):
        engine_object = find_engine_data(json_data)
        restricted = json_data['restricted'].copy()
        restricted.extend(engine_object['restricted'])
        for restricted_path in restricted:
            restricted_path = pathlib.Path(restricted_path).resolve()
            restricted_json = restricted_path / 'restricted.json'
            with restricted_json.open('r') as f:
                try:
                    restricted_json_data = json.load(f)
                except Exception as e:
                    logger.warn(f'{restricted_json} failed to load: {str(e)}')
                else:
                    this_restricted_name = restricted_json_data['restricted_name']
                    if this_restricted_name == restricted_name:
                        return restricted_path

    elif isinstance(default_folder, str):
        if default_folder == 'engines':
            default_engines_folder = pathlib.Path(json_data['default_engines_folder']).resolve()
            return default_engines_folder
        elif default_folder == 'projects':
            default_projects_folder = pathlib.Path(json_data['default_projects_folder']).resolve()
            return default_projects_folder
        elif default_folder == 'gems':
            default_gems_folder = pathlib.Path(json_data['default_gems_folder']).resolve()
            return default_gems_folder
        elif default_folder == 'templates':
            default_templates_folder = pathlib.Path(json_data['default_templates_folder']).resolve()
            return default_templates_folder
        elif default_folder == 'restricted':
            default_restricted_folder = pathlib.Path(json_data['default_restricted_folder']).resolve()
            return default_restricted_folder

    elif isinstance(repo_name, str):
        cache_folder = get_o3de_cache_folder()
        for repo_uri in json_data['repos']:
            repo_uri = pathlib.Path(repo_uri).resolve()
            repo_sha256 = hashlib.sha256(repo_uri.encode())
            cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
            if cache_file.is_file():
                repo = pathlib.Path(cache_file).resolve()
                with repo.open('r') as f:
                    try:
                        repo_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{cache_file} failed to load: {str(e)}')
                    else:
                        this_repos_name = repo_json_data['repo_name']
                        if this_repos_name == repo_name:
                            return repo_uri
    return None


def print_engines_data(engines_data: dict) -> None:
    print('\n')
    print("Engines================================================")
    for engine_object in engines_data:
        # if it's not local it should be in the cache
        engine_uri = engine_object['path']
        parsed_uri = urllib.parse.urlparse(engine_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(engine_uri.encode())
            cache_folder = get_o3de_cache_folder()
            engine = cache_folder / str(repo_sha256.hexdigest() + '.json')
            print(f'{engine_uri}/engine.json cached as:')
        else:
            engine_json = pathlib.Path(engine_uri).resolve() / 'engine.json'

        with engine_json.open('r') as f:
            try:
                engine_json_data = json.load(f)
            except Exception as e:
                logger.warn(f'{engine_json} failed to load: {str(e)}')
            else:
                print(engine_json)
                print(json.dumps(engine_json_data, indent=4))
        print('\n')


def print_projects_data(projects_data: dict) -> None:
    print('\n')
    print("Projects================================================")
    for project_uri in projects_data:
        # if it's not local it should be in the cache
        parsed_uri = urllib.parse.urlparse(project_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(project_uri.encode())
            cache_folder = get_o3de_cache_folder()
            project_json = cache_folder / str(repo_sha256.hexdigest() + '.json')
        else:
            project_json = pathlib.Path(project_uri).resolve() / 'project.json'

        with project_json.open('r') as f:
            try:
                project_json_data = json.load(f)
            except Exception as e:
                logger.warn(f'{project_json} failed to load: {str(e)}')
            else:
                print(project_json)
                print(json.dumps(project_json_data, indent=4))
        print('\n')


def print_gems_data(gems_data: dict) -> None:
    print('\n')
    print("Gems================================================")
    for gem_uri in gems_data:
        # if it's not local it should be in the cache
        parsed_uri = urllib.parse.urlparse(gem_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(gem_uri.encode())
            cache_folder = get_o3de_cache_folder()
            gem_json = cache_folder / str(repo_sha256.hexdigest() + '.json')
        else:
            gem_json = pathlib.Path(gem_uri).resolve() / 'gem.json'

        with gem_json.open('r') as f:
            try:
                gem_json_data = json.load(f)
            except Exception as e:
                logger.warn(f'{gem_json} failed to load: {str(e)}')
            else:
                print(gem_json)
                print(json.dumps(gem_json_data, indent=4))
        print('\n')


def print_templates_data(templates_data: dict) -> None:
    print('\n')
    print("Templates================================================")
    for template_uri in templates_data:
        # if it's not local it should be in the cache
        parsed_uri = urllib.parse.urlparse(template_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(template_uri.encode())
            cache_folder = get_o3de_cache_folder()
            template_json = cache_folder / str(repo_sha256.hexdigest() + '.json')
        else:
            template_json = pathlib.Path(template_uri).resolve() / 'template.json'

        with template_json.open('r') as f:
            try:
                template_json_data = json.load(f)
            except Exception as e:
                logger.warn(f'{template_json} failed to load: {str(e)}')
            else:
                print(template_json)
                print(json.dumps(template_json_data, indent=4))
        print('\n')


def print_repos_data(repos_data: dict) -> None:
    print('\n')
    print("Repos================================================")
    cache_folder = get_o3de_cache_folder()
    for repo_uri in repos_data:
        repo_sha256 = hashlib.sha256(repo_uri.encode())
        cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
        if valid_o3de_repo_json(cache_file):
            with cache_file.open('r') as s:
                try:
                    repo_json_data = json.load(s)
                except Exception as e:
                    logger.warn(f'{cache_file} failed to load: {str(e)}')
                else:
                    print(f'{repo_uri}/repo.json cached as:')
                    print(cache_file)
                    print(json.dumps(repo_json_data, indent=4))
        print('\n')


def print_restricted_data(restricted_data: dict) -> None:
    print('\n')
    print("Restricted================================================")
    for restricted_path in restricted_data:
        restricted_json = pathlib.Path(restricted_path).resolve() / 'restricted.json'
        with restricted_json.open('r') as f:
            try:
                restricted_json_data = json.load(f)
            except Exception as e:
                logger.warn(f'{restricted_json} failed to load: {str(e)}')
            else:
                print(restricted_json)
                print(json.dumps(restricted_json_data, indent=4))
        print('\n')


def get_this_engine() -> dict:
    json_data = load_o3de_manifest()
    engine_data = find_engine_data(json_data)
    return engine_data


def get_engines() -> dict:
    json_data = load_o3de_manifest()
    return json_data['engines']


def get_projects() -> dict:
    json_data = load_o3de_manifest()
    return json_data['projects']


def get_gems() -> dict:
    json_data = load_o3de_manifest()
    return json_data['gems']


def get_templates() -> dict:
    json_data = load_o3de_manifest()
    return json_data['templates']


def get_restricted() -> dict:
    json_data = load_o3de_manifest()
    return json_data['restricted']


def get_repos() -> dict:
    json_data = load_o3de_manifest()
    return json_data['repos']


def get_engine_projects() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    return engine_object['projects']


def get_engine_gems() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    return engine_object['gems']


def get_engine_templates() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    return engine_object['templates']


def get_engine_restricted() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    return engine_object['restricted']


def get_external_subdirectories() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    return engine_object['external_subdirectories']


def get_all_projects() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    projects_data = json_data['projects'].copy()
    projects_data.extend(engine_object['projects'])
    return projects_data


def get_all_gems() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    gems_data = json_data['gems'].copy()
    gems_data.extend(engine_object['gems'])
    return gems_data


def get_all_templates() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    templates_data = json_data['templates'].copy()
    templates_data.extend(engine_object['templates'])
    return templates_data


def get_all_restricted() -> dict:
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data)
    restricted_data = json_data['restricted'].copy()
    restricted_data.extend(engine_object['restricted'])
    return restricted_data


def print_this_engine(verbose: int) -> None:
    engine_data = get_this_engine()
    print(json.dumps(engine_data, indent=4))
    if verbose > 0:
        print_engines_data(engine_data)


def print_engines(verbose: int) -> None:
    engines_data = get_engines()
    print(json.dumps(engines_data, indent=4))
    if verbose > 0:
        print_engines_data(engines_data)


def print_projects(verbose: int) -> None:
    projects_data = get_projects()
    print(json.dumps(projects_data, indent=4))
    if verbose > 0:
        print_projects_data(projects_data)


def print_gems(verbose: int) -> None:
    gems_data = get_gems()
    print(json.dumps(gems_data, indent=4))
    if verbose > 0:
        print_gems_data(gems_data)


def print_templates(verbose: int) -> None:
    templates_data = get_templates()
    print(json.dumps(templates_data, indent=4))
    if verbose > 0:
        print_templates_data(templates_data)


def print_restricted(verbose: int) -> None:
    restricted_data = get_restricted()
    print(json.dumps(restricted_data, indent=4))
    if verbose > 0:
        print_restricted_data(restricted_data)


def register_show_repos(verbose: int) -> None:
    repos_data = get_repos()
    print(json.dumps(repos_data, indent=4))
    if verbose > 0:
        print_repos_data(repos_data)


def print_engine_projects(verbose: int) -> None:
    engine_projects_data = get_engine_projects()
    print(json.dumps(engine_projects_data, indent=4))
    if verbose > 0:
        print_projects_data(engine_projects_data)


def print_engine_gems(verbose: int) -> None:
    engine_gems_data = get_engine_gems()
    print(json.dumps(engine_gems_data, indent=4))
    if verbose > 0:
        print_gems_data(engine_gems_data)


def print_engine_templates(verbose: int) -> None:
    engine_templates_data = get_engine_templates()
    print(json.dumps(engine_templates_data, indent=4))
    if verbose > 0:
        print_templates_data(engine_templates_data)


def print_engine_restricted(verbose: int) -> None:
    engine_restricted_data = get_engine_restricted()
    print(json.dumps(engine_restricted_data, indent=4))
    if verbose > 0:
        print_restricted_data(engine_restricted_data)


def print_external_subdirectories(verbose: int) -> None:
    external_subdirs_data = get_external_subdirectories()
    print(json.dumps(external_subdirs_data, indent=4))


def print_all_projects(verbose: int) -> None:
    all_projects_data = get_all_projects()
    print(json.dumps(all_projects_data, indent=4))
    if verbose > 0:
        print_projects_data(all_projects_data)


def print_all_gems(verbose: int) -> None:
    all_gems_data = get_all_gems()
    print(json.dumps(all_gems_data, indent=4))
    if verbose > 0:
        print_gems_data(all_gems_data)


def print_all_templates(verbose: int) -> None:
    all_templates_data = get_all_templates()
    print(json.dumps(all_templates_data, indent=4))
    if verbose > 0:
        print_templates_data(all_templates_data)


def print_all_restricted(verbose: int) -> None:
    all_restricted_data = get_all_restricted()
    print(json.dumps(all_restricted_data, indent=4))
    if verbose > 0:
        print_restricted_data(all_restricted_data)


def register_show(verbose: int) -> None:
    json_data = load_o3de_manifest()
    print(f"{get_o3de_manifest()}:")
    print(json.dumps(json_data, indent=4))

    if verbose > 0:
        print_engines_data(get_engines())
        print_projects_data(get_all_projects())
        print_gems_data(get_gems())
        print_templates_data(get_all_templates())
        print_restricted_data(get_all_restricted())
        print_repos_data(get_repos())


def find_engine_data(json_data: dict,
                     engine_path: str or pathlib.Path = None) -> dict or None:
    if not engine_path:
        engine_path = get_this_engine_path()
    engine_path = pathlib.Path(engine_path).resolve()

    for engine_object in json_data['engines']:
        engine_object_path = pathlib.Path(engine_object['path']).resolve()
        if engine_path == engine_object_path:
            return engine_object

    return None


def get_engine_data(engine_name: str = None,
                    engine_path: str or pathlib.Path = None, ) -> dict or None:
    if not engine_name and not engine_path:
        logger.error('Must specify either a Engine name or Engine Path.')
        return 1

    if engine_name and not engine_path:
        engine_path = get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Engine Path {engine_path} has not been registered.')
        return 1

    engine_path = pathlib.Path(engine_path).resolve()
    engine_json = engine_path / 'engine.json'
    if not engine_json.is_file():
        logger.error(f'Engine json {engine_json} is not present.')
        return 1
    if not valid_o3de_engine_json(engine_json):
        logger.error(f'Engine json {engine_json} is not valid.')
        return 1

    with engine_json.open('r') as f:
        try:
            engine_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{engine_json} failed to load: {str(e)}')
        else:
            return engine_json_data

    return None


def get_project_data(project_name: str = None,
                     project_path: str or pathlib.Path = None, ) -> dict or None:
    if not project_name and not project_path:
        logger.error('Must specify either a Project name or Project Path.')
        return 1

    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)

    if not project_path:
        logger.error(f'Project Path {project_path} has not been registered.')
        return 1

    project_path = pathlib.Path(project_path).resolve()
    project_json = project_path / 'project.json'
    if not project_json.is_file():
        logger.error(f'Project json {project_json} is not present.')
        return 1
    if not valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return 1

    with project_json.open('r') as f:
        try:
            project_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{project_json} failed to load: {str(e)}')
        else:
            return project_json_data

    return None


def get_gem_data(gem_name: str = None,
                 gem_path: str or pathlib.Path = None, ) -> dict or None:
    if not gem_name and not gem_path:
        logger.error('Must specify either a Gem name or Gem Path.')
        return 1

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)

    if not gem_path:
        logger.error(f'Gem Path {gem_path} has not been registered.')
        return 1

    gem_path = pathlib.Path(gem_path).resolve()
    gem_json = gem_path / 'gem.json'
    if not gem_json.is_file():
        logger.error(f'Gem json {gem_json} is not present.')
        return 1
    if not valid_o3de_gem_json(gem_json):
        logger.error(f'Gem json {gem_json} is not valid.')
        return 1

    with gem_json.open('r') as f:
        try:
            gem_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{gem_json} failed to load: {str(e)}')
        else:
            return gem_json_data

    return None


def get_template_data(template_name: str = None,
                      template_path: str or pathlib.Path = None, ) -> dict or None:
    if not template_name and not template_path:
        logger.error('Must specify either a Template name or Template Path.')
        return 1

    if template_name and not template_path:
        template_path = get_registered(template_name=template_name)

    if not template_path:
        logger.error(f'Template Path {template_path} has not been registered.')
        return 1

    template_path = pathlib.Path(template_path).resolve()
    template_json = template_path / 'template.json'
    if not template_json.is_file():
        logger.error(f'Template json {template_json} is not present.')
        return 1
    if not valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_json} is not valid.')
        return 1

    with template_json.open('r') as f:
        try:
            template_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{template_json} failed to load: {str(e)}')
        else:
            return template_json_data

    return None


def get_restricted_data(restricted_name: str = None,
                        restricted_path: str or pathlib.Path = None, ) -> dict or None:
    if not restricted_name and not restricted_path:
        logger.error('Must specify either a Restricted name or Restricted Path.')
        return 1

    if restricted_name and not restricted_path:
        restricted_path = get_registered(restricted_name=restricted_name)

    if not restricted_path:
        logger.error(f'Restricted Path {restricted_path} has not been registered.')
        return 1

    restricted_path = pathlib.Path(restricted_path).resolve()
    restricted_json = restricted_path / 'restricted.json'
    if not restricted_json.is_file():
        logger.error(f'Restricted json {restricted_json} is not present.')
        return 1
    if not valid_o3de_restricted_json(restricted_json):
        logger.error(f'Restricted json {restricted_json} is not valid.')
        return 1

    with restricted_json.open('r') as f:
        try:
            restricted_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{restricted_json} failed to load: {str(e)}')
        else:
            return restricted_json_data

    return None


def get_downloadables() -> dict:
    json_data = load_o3de_manifest()
    downloadable_data = {}
    downloadable_data.update({'engines': []})
    downloadable_data.update({'projects': []})
    downloadable_data.update({'gems': []})
    downloadable_data.update({'templates': []})
    downloadable_data.update({'restricted': []})

    def recurse_downloadables(repo_uri: str or pathlib.Path) -> None:
        cache_folder = get_o3de_cache_folder()
        repo_sha256 = hashlib.sha256(repo_uri.encode())
        cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
        if valid_o3de_repo_json(cache_file):
            with cache_file.open('r') as s:
                try:
                    repo_json_data = json.load(s)
                except Exception as e:
                    logger.warn(f'{cache_file} failed to load: {str(e)}')
                else:
                    for engine in repo_json_data['engines']:
                        if engine not in downloadable_data['engines']:
                            downloadable_data['engines'].append(engine)

                    for project in repo_json_data['projects']:
                        if project not in downloadable_data['projects']:
                            downloadable_data['projects'].append(project)

                    for gem in repo_json_data['gems']:
                        if gem not in downloadable_data['gems']:
                            downloadable_data['gems'].append(gem)

                    for template in repo_json_data['templates']:
                        if template not in downloadable_data['templates']:
                            downloadable_data['templates'].append(template)

                    for restricted in repo_json_data['restricted']:
                        if restricted not in downloadable_data['restricted']:
                            downloadable_data['restricted'].append(restricted)

                    for repo in repo_json_data['repos']:
                        if repo not in downloadable_data['repos']:
                            downloadable_data['repos'].append(repo)

                    for repo in downloadable_data['repos']:
                        recurse_downloadables(repo)

    for repo_entry in json_data['repos']:
        recurse_downloadables(repo_entry)
    return downloadable_data


def get_downloadable_engines() -> dict:
    downloadable_data = get_downloadables()
    return downloadable_data['engines']


def get_downloadable_projects() -> dict:
    downloadable_data = get_downloadables()
    return downloadable_data['projects']


def get_downloadable_gems() -> dict:
    downloadable_data = get_downloadables()
    return downloadable_data['gems']


def get_downloadable_templates() -> dict:
    downloadable_data = get_downloadables()
    return downloadable_data['templates']


def get_downloadable_restricted() -> dict:
    downloadable_data = get_downloadables()
    return downloadable_data['restricted']


def print_downloadable_engines(verbose: int) -> None:
    downloadable_engines = get_downloadable_engines()
    for engine_data in downloadable_engines:
        print(json.dumps(engine_data, indent=4))
    if verbose > 0:
        print_engines_data(downloadable_engines)


def print_downloadable_projects(verbose: int) -> None:
    downloadable_projects = get_downloadable_projects()
    for projects_data in downloadable_projects:
        print(json.dumps(projects_data, indent=4))
    if verbose > 0:
        print_projects_data(downloadable_projects)


def print_downloadable_gems(verbose: int) -> None:
    downloadable_gems = get_downloadable_gems()
    for gem_data in downloadable_gems:
        print(json.dumps(gem_data, indent=4))
    if verbose > 0:
        print_gems_data(downloadable_gems)


def print_downloadable_templates(verbose: int) -> None:
    downloadable_templates = get_downloadable_templates()
    for template_data in downloadable_templates:
        print(json.dumps(template_data, indent=4))
    if verbose > 0:
        print_engines_data(downloadable_templates)


def print_downloadable_restricted(verbose: int) -> None:
    downloadable_restricted = get_downloadable_restricted()
    for restricted_data in downloadable_restricted:
        print(json.dumps(restricted_data, indent=4))
    if verbose > 0:
        print_engines_data(downloadable_restricted)


def print_downloadables(verbose: int) -> None:
    downloadable_data = get_downloadables()
    print(json.dumps(downloadable_data, indent=4))
    if verbose > 0:
        print_engines_data(downloadable_data['engines'])
        print_projects_data(downloadable_data['projects'])
        print_gems_data(downloadable_data['gems'])
        print_templates_data(downloadable_data['templates'])
        print_restricted_data(downloadable_data['templates'])


def download_engine(engine_name: str,
                    dest_path: str) -> int:
    if not dest_path:
        dest_path = get_registered(default_folder='engines')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True)

    download_path = get_o3de_download_folder() / 'engines' / engine_name
    download_path.mkdir(exist_ok=True)
    download_zip_path = download_path / 'engine.zip'

    downloadable_engine_data = get_downloadable(engine_name=engine_name)
    if not downloadable_engine_data:
        logger.error(f'Downloadable engine {engine_name} not found.')
        return 1

    origin = downloadable_engine_data['origin']
    url = f'{origin}/project.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Engine zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the engine.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_engine_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised engine you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised engine!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded engine.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the engine.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_engine_folder = dest_path / engine_name
    if dest_engine_folder.is_dir():
        backup_folder(dest_engine_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_engine_json = dest_engine_folder / 'engine.json'
    if not unzipped_engine_json.is_file():
        logger.error(f'Engine json {unzipped_engine_json} is missing.')
        return 1

    if not valid_o3de_engine_json(unzipped_engine_json):
        logger.error(f'Engine json {unzipped_engine_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable engine.json
    # then compare it to the engine.json in the zip, they should now be identical
    try:
        del downloadable_engine_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_engine_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_engine_json.open('r') as s:
        try:
            unzipped_engine_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read engine json {unzipped_engine_json}. Unable to confirm this'
                         f' is the same template that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_engine_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded engine.json does not match'
                     f' the advertised engine.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_project(project_name: str,
                     dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = get_registered(default_folder='projects')
        if not dest_path:
            logger.error(f'Destination path not specified and not default projects path.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = get_o3de_download_folder() / 'projects' / project_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'project.zip'

    downloadable_project_data = get_downloadable(project_name=project_name)
    if not downloadable_project_data:
        logger.error(f'Downloadable project {project_name} not found.')
        return 1

    origin = downloadable_project_data['origin']
    url = f'{origin}/project.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Project zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the project.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_project_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised project you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised project!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded project.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the project.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_project_folder = dest_path / project_name
    if dest_project_folder.is_dir():
        backup_folder(dest_project_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_project_folder)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_project_json = dest_project_folder / 'project.json'
    if not unzipped_project_json.is_file():
        logger.error(f'Project json {unzipped_project_json} is missing.')
        return 1

    if not valid_o3de_project_json(unzipped_project_json):
        logger.error(f'Project json {unzipped_project_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable project.json
    # then compare it to the project.json in the zip, they should now be identical
    try:
        del downloadable_project_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_project_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_project_json.open('r') as s:
        try:
            unzipped_project_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read Project json {unzipped_project_json}. Unable to confirm this'
                         f' is the same project that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_project_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded project.json does not match'
                     f' the advertised project.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_gem(gem_name: str,
                 dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = get_registered(default_folder='gems')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = get_o3de_download_folder() / 'gems' / gem_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'gem.zip'

    downloadable_gem_data = get_downloadable(gem_name=gem_name)
    if not downloadable_gem_data:
        logger.error(f'Downloadable gem {gem_name} not found.')
        return 1

    origin = downloadable_gem_data['origin']
    url = f'{origin}/gem.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Gem zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the gem.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_gem_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised gem you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised gem!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded gem.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the gem.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_gem_folder = dest_path / gem_name
    if dest_gem_folder.is_dir():
        backup_folder(dest_gem_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as gem_zip:
        try:
            gem_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_gem_json = dest_gem_folder / 'gem.json'
    if not unzipped_gem_json.is_file():
        logger.error(f'Engine json {unzipped_gem_json} is missing.')
        return 1

    if not valid_o3de_engine_json(unzipped_gem_json):
        logger.error(f'Engine json {unzipped_gem_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable gem.json
    # then compare it to the gem.json in the zip, they should now be identical
    try:
        del downloadable_gem_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_gem_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_gem_json.open('r') as s:
        try:
            unzipped_gem_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read gem json {unzipped_gem_json}. Unable to confirm this'
                         f' is the same gem that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_gem_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded gem.json does not match'
                     f' the advertised gem.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_template(template_name: str,
                      dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = get_registered(default_folder='templates')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = get_o3de_download_folder() / 'templates' / template_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'template.zip'

    downloadable_template_data = get_downloadable(template_name=template_name)
    if not downloadable_template_data:
        logger.error(f'Downloadable template {template_name} not found.')
        return 1

    origin = downloadable_template_data['origin']
    url = f'{origin}/project.zip'
    parsed_uri = urllib.parse.urlparse(url)

    result = 0

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Template zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the template.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_template_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised template you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised template!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded template.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the template.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_template_folder = dest_path / template_name
    if dest_template_folder.is_dir():
        backup_folder(dest_template_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_template_json = dest_template_folder / 'template.json'
    if not unzipped_template_json.is_file():
        logger.error(f'Template json {unzipped_template_json} is missing.')
        return 1

    if not valid_o3de_engine_json(unzipped_template_json):
        logger.error(f'Template json {unzipped_template_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable template.json
    # then compare it to the template.json in the zip, they should now be identical
    try:
        del downloadable_template_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_template_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_template_json.open('r') as s:
        try:
            unzipped_template_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read Template json {unzipped_template_json}. Unable to confirm this'
                         f' is the same template that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_template_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded template.json does not match'
                     f' the advertised template.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_restricted(restricted_name: str,
                        dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = get_registered(default_folder='restricted')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = get_o3de_download_folder() / 'restricted' / restricted_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'restricted.zip'

    downloadable_restricted_data = get_downloadable(restricted_name=restricted_name)
    if not downloadable_restricted_data:
        logger.error(f'Downloadable Restricted {restricted_name} not found.')
        return 1

    origin = downloadable_restricted_data['origin']
    url = f'{origin}/restricted.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Restricted already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Restricted zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the restricted.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_restricted_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised restricted you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised restricted!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded restricted.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the restricted.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_restricted_folder = dest_path / restricted_name
    if dest_restricted_folder.is_dir():
        backup_folder(dest_restricted_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_restricted_json = dest_restricted_folder / 'restricted.json'
    if not unzipped_restricted_json.is_file():
        logger.error(f'Restricted json {unzipped_restricted_json} is missing.')
        return 1

    if not valid_o3de_engine_json(unzipped_restricted_json):
        logger.error(f'Restricted json {unzipped_restricted_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable restricted.json
    # then compare it to the restricted.json in the zip, they should now be identical
    try:
        del downloadable_restricted_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_restricted_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_restricted_json.open('r') as s:
        try:
            unzipped_restricted_json_data = json.load(s)
        except Exception as e:
            logger.error(
                f'Failed to read Restricted json {unzipped_restricted_json}. Unable to confirm this'
                f' is the same restricted that was advertised.')
            return 1
    sha256B = hashlib.sha256(
        json.dumps(unzipped_restricted_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded restricted.json does not match'
                     f' the advertised restricted.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def add_gem_dependency(cmake_file: str or pathlib.Path,
                       gem_target: str) -> int:
    """
    adds a gem dependency to a cmake file
    :param cmake_file: path to the cmake file
    :param gem_target: name of the cmake target
    :return: 0 for success or non 0 failure code
    """
    if not os.path.isfile(cmake_file):
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return 1

    # on a line by basis, see if there already is Gem::{gem_name}
    # find the first occurrence of a gem, copy its formatting and replace
    # the gem name with the new one and append it
    # if the gem is already present fail
    t_data = []
    added = False
    with open(cmake_file, 'r') as s:
        for line in s:
            if f'Gem::{gem_target}' in line:
                logger.warning(f'{gem_target} is already a gem dependency.')
                return 0
            if not added and r'Gem::' in line:
                new_gem = ' ' * line.find(r'Gem::') + f'Gem::{gem_target}\n'
                t_data.append(new_gem)
                added = True
            t_data.append(line)

    # if we didn't add it the set gem dependencies could be empty so
    # add a new gem, if empty the correct format is 1 tab=4spaces
    if not added:
        index = 0
        for line in t_data:
            index = index + 1
            if r'set(GEM_DEPENDENCIES' in line:
                t_data.insert(index, f'    Gem::{gem_target}\n')
                added = True
                break

    # if we didn't add it then it's not here, add a whole new one
    if not added:
        t_data.append('\n')
        t_data.append('set(GEM_DEPENDENCIES\n')
        t_data.append(f'    Gem::{gem_target}\n')
        t_data.append(')\n')

    # write the cmake
    os.unlink(cmake_file)
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def get_project_runtime_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))


def get_project_tool_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))


def get_project_server_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))


def get_project_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    runtime_gems = get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))
    tool_gems = get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))
    server_gems = get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))
    return runtime_gems.union(tool_gems.union(server_gems))


def get_gem_targets_from_cmake_file(cmake_file: str or pathlib.Path) -> set:
    """
    Gets a list of declared gem targets dependencies of a cmake file
    :param cmake_file: path to the cmake file
    :return: set of gem targets found
    """
    cmake_file = pathlib.Path(cmake_file).resolve()

    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    gem_target_set = set()
    with cmake_file.open('r') as s:
        for line in s:
            gem_name = line.split('Gem::')
            if len(gem_name) > 1:
                # Only take the name as everything leading up to the first '.' if found
                # Gem naming conventions will have GemName.Editor, GemName.Server, and GemName
                # as different targets of the GemName Gem
                gem_target_set.add(gem_name[1].replace('\n', ''))
    return gem_target_set


def get_project_runtime_gem_names(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))


def get_project_tool_gem_names(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))


def get_project_server_gem_names(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))


def get_project_gem_names(project_path: str or pathlib.Path,
                     platform: str = 'Common') -> set:
    runtime_gem_names = get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))
    tool_gem_names = get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))
    server_gem_names = get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))
    return runtime_gem_names.union(tool_gem_names.union(server_gem_names))


def get_gem_names_from_cmake_file(cmake_file: str or pathlib.Path) -> set:
    """
    Gets a list of declared gem dependencies of a cmake file
    :param cmake_file: path to the cmake file
    :return: set of gems found
    """
    cmake_file = pathlib.Path(cmake_file).resolve()

    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    gem_set = set()
    with cmake_file.open('r') as s:
        for line in s:
            gem_name = line.split('Gem::')
            if len(gem_name) > 1:
                # Only take the name as everything leading up to the first '.' if found
                # Gem naming conventions will have GemName.Editor, GemName.Server, and GemName
                # as different targets of the GemName Gem
                gem_set.add(gem_name[1].split('.')[0].replace('\n', ''))
    return gem_set


def get_project_runtime_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_runtime_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(get_registered(gem_name=gem_name))
    return gem_paths


def get_project_tool_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_tool_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(get_registered(gem_name=gem_name))
    return gem_paths


def get_project_server_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_server_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(get_registered(gem_name=gem_name))
    return gem_paths


def get_project_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(get_registered(gem_name=gem_name))
    return gem_paths


def remove_gem_dependency(cmake_file: str or pathlib.Path,
                          gem_target: str) -> int:
    """
    removes a gem dependency from a cmake file
    :param cmake_file: path to the cmake file
    :param gem_target: cmake target name
    :return: 0 for success or non 0 failure code
    """
    if not os.path.isfile(cmake_file):
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return 1

    # on a line by basis, remove any line with Gem::{gem_name}
    t_data = []
    # Remove the gem from the cmake_dependencies file by skipping the gem name entry
    removed = False
    with open(cmake_file, 'r') as s:
        for line in s:
            if f'Gem::{gem_target}' in line:
                removed = True
            else:
                t_data.append(line)

    if not removed:
        logger.error(f'Failed to remove Gem::{gem_target} from cmake file {cmake_file}')
        return 1

    # write the cmake
    os.unlink(cmake_file)
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def get_project_templates():  # temporary until we have a better way to do this... maybe template_type element
    project_templates = []
    for template in get_all_templates():
        if 'Project' in template:
            project_templates.append(template)
    return project_templates


def get_gem_templates():  # temporary until we have a better way to do this... maybe template_type element
    gem_templates = []
    for template in get_all_templates():
        if 'Gem' in template:
            gem_templates.append(template)
    return gem_templates


def get_generic_templates():  # temporary until we have a better way to do this... maybe template_type element
    generic_templates = []
    for template in get_all_templates():
        if 'Project' not in template and  'Gem' not in template:
            generic_templates.append(template)
    return generic_templates


def get_dependencies_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                dependency_type: str = 'runtime',
                                platform: str = 'Common') -> str or None:
    """
    get the standard cmake file name for a particular type of dependency
    :param gem_name: name of the gem, resolves gem_path
    :param gem_path: path of the gem
    :return: list of gem targets
    """
    if not project_name and not project_path:
        logger.error(f'Must supply either a Project Name or Project Path.')
        return None

    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)

    project_path = pathlib.Path(project_path).resolve()

    if platform == 'Common':
        dependencies_file = f'{dependency_type}_dependencies.cmake'
        dependencies_file_path = project_path / 'Gem/Code' / dependencies_file
        if dependencies_file_path.is_file():
            return dependencies_file_path
        return project_path / 'Code' / dependencies_file
    else:
        dependencies_file = f'{platform.lower()}_{dependency_type}_dependencies.cmake'
        dependencies_file_path = project_path / 'Gem/Code/Platform' / platform / dependencies_file
        if dependencies_file_path.is_file():
            return dependencies_file_path
        return project_path / 'Code/Platform' / platform / dependencies_file


def get_all_gem_targets() -> list:
    modules = []
    for gem_path in get_all_gems():
        this_gems_targets = get_gem_targets(gem_path=gem_path)
        modules.extend(this_gems_targets)
    return modules


def get_gem_targets(gem_name: str = None,
                    gem_path: str or pathlib.Path = None) -> list:
    """
    Finds gem targets in a gem
    :param gem_name: name of the gem, resolves gem_path
    :param gem_path: path of the gem
    :return: list of gem targets
    """
    if not gem_name and not gem_path:
        return []

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)

    if not gem_path:
        return []

    gem_path = pathlib.Path(gem_path).resolve()
    gem_json = gem_path / 'gem.json'
    if not valid_o3de_gem_json(gem_json):
        return []

    module_identifiers = [
        'MODULE',
        'GEM_MODULE',
        '${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}'
    ]
    modules = []
    for root, dirs, files in os.walk(gem_path):
        for file in files:
            if file == 'CMakeLists.txt':
                with open(os.path.join(root, file), 'r') as s:
                    for line in s:
                        trimmed = line.lstrip()
                        if trimmed.startswith('NAME '):
                            trimmed = trimmed.rstrip(' \n')
                            split_trimmed = trimmed.split(' ')
                            if len(split_trimmed) == 3 and split_trimmed[2] in module_identifiers:
                                modules.append(split_trimmed[1])
    return modules


def add_external_subdirectory(external_subdir: str or pathlib.Path,
                              engine_path: str or pathlib.Path = None,
                              supress_errors: bool = False) -> int:
    """
    add external subdirectory to a cmake
    :param external_subdir: external subdirectory to add to cmake
    :param engine_path: optional engine path, defaults to this engine
    :param supress_errors: optional silence errors
    :return: 0 for success or non 0 failure code
    """
    external_subdir = pathlib.Path(external_subdir).resolve()
    if not external_subdir.is_dir():
        if not supress_errors:
            logger.error(f'Add External Subdirectory Failed: {external_subdir} does not exist.')
        return 1

    external_subdir_cmake = external_subdir / 'CMakeLists.txt'
    if not external_subdir_cmake.is_file():
        if not supress_errors:
            logger.error(f'Add External Subdirectory Failed: {external_subdir} does not contain a CMakeLists.txt.')
        return 1

    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data, engine_path)
    if not engine_object:
        if not supress_errors:
            logger.error(f'Add External Subdirectory Failed: {engine_path} not registered.')
        return 1

    while external_subdir.as_posix() in engine_object['external_subdirectories']:
        engine_object['external_subdirectories'].remove(external_subdir.as_posix())

    def parse_cmake_file(cmake: str or pathlib.Path,
                         files: set()):
        cmake_path = pathlib.Path(cmake).resolve()
        cmake_file = cmake_path
        if cmake_path.is_dir():
            files.add(cmake_path)
            cmake_file = cmake_path / 'CMakeLists.txt'
        elif cmake_path.is_file():
            cmake_path = cmake_path.parent
        else:
            return

        with cmake_file.open('r') as s:
            lines = s.readlines()
            for line in lines:
                line = line.strip()
                start = line.find('include(')
                if start == 0:
                    end = line.find(')', start)
                    if end > start + len('include('):
                        try:
                            include_cmake_file = pathlib.Path(engine_path / line[start + len('include('): end]).resolve()
                        except Exception as e:
                            pass
                        else:
                            parse_cmake_file(include_cmake_file, files)
                else:
                    start = line.find('add_subdirectory(')
                    if start == 0:
                        end = line.find(')', start)
                        if end > start + len('add_subdirectory('):
                            try:
                                include_cmake_file = pathlib.Path(
                                    cmake_path / line[start + len('add_subdirectory('): end]).resolve()
                            except Exception as e:
                                pass
                            else:
                                parse_cmake_file(include_cmake_file, files)

    cmake_files = set()
    parse_cmake_file(engine_path, cmake_files)
    for external in engine_object["external_subdirectories"]:
        parse_cmake_file(external, cmake_files)

    if external_subdir in cmake_files:
        save_o3de_manifest(json_data)
        if not supress_errors:
            logger.error(f'External subdirectory {external_subdir.as_posix()} already included by add_subdirectory().')
        return 1

    engine_object['external_subdirectories'].insert(0, external_subdir.as_posix())
    engine_object['external_subdirectories'] = sorted(engine_object['external_subdirectories'])

    save_o3de_manifest(json_data)

    return 0


def remove_external_subdirectory(external_subdir: str or pathlib.Path,
                                 engine_path: str or pathlib.Path = None) -> int:
    """
    remove external subdirectory from cmake
    :param external_subdir: external subdirectory to add to cmake
    :param engine_path: optional engine path, defaults to this engine
    :return: 0 for success or non 0 failure code
    """
    json_data = load_o3de_manifest()
    engine_object = find_engine_data(json_data, engine_path)
    if not engine_object:
        logger.error(f'Remove External Subdirectory Failed: {engine_path} not registered.')
        return 1

    external_subdir = pathlib.Path(external_subdir).resolve()
    while external_subdir.as_posix() in engine_object['external_subdirectories']:
        engine_object['external_subdirectories'].remove(external_subdir.as_posix())

    save_o3de_manifest(json_data)

    return 0


def add_gem_to_cmake(gem_name: str = None,
                     gem_path: str or pathlib.Path = None,
                     engine_name: str = None,
                     engine_path: str or pathlib.Path = None,
                     supress_errors: bool = False) -> int:
    """
    add a gem to a cmake as an external subdirectory for an engine
    :param gem_name: name of the gem to add to cmake
    :param gem_path: the path of the gem to add to cmake
    :param engine_name: name of the engine to add to cmake
    :param engine_path: the path of the engine to add external subdirectory to, default to this engine
    :param supress_errors: optional silence errors
    :return: 0 for success or non 0 failure code
    """
    if not gem_name and not gem_path:
        if not supress_errors:
            logger.error('Must specify either a Gem name or Gem Path.')
        return 1

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)

    if not gem_path:
        if not supress_errors:
            logger.error(f'Gem Path {gem_path} has not been registered.')
        return 1

    gem_path = pathlib.Path(gem_path).resolve()
    gem_json = gem_path / 'gem.json'
    if not gem_json.is_file():
        if not supress_errors:
            logger.error(f'Gem json {gem_json} is not present.')
        return 1
    if not valid_o3de_gem_json(gem_json):
        if not supress_errors:
            logger.error(f'Gem json {gem_json} is not valid.')
        return 1

    if not engine_name and not engine_path:
        engine_path = get_this_engine_path()

    if engine_name and not engine_path:
        engine_path = get_registered(engine_name=engine_name)

    if not engine_path:
        if not supress_errors:
            logger.error(f'Engine Path {engine_path} has not been registered.')
        return 1

    engine_json = engine_path / 'engine.json'
    if not engine_json.is_file():
        if not supress_errors:
            logger.error(f'Engine json {engine_json} is not present.')
        return 1
    if not valid_o3de_engine_json(engine_json):
        if not supress_errors:
            logger.error(f'Engine json {engine_json} is not valid.')
        return 1

    return add_external_subdirectory(external_subdir=gem_path, engine_path=engine_path, supress_errors=supress_errors)


def remove_gem_from_cmake(gem_name: str = None,
                          gem_path: str or pathlib.Path = None,
                          engine_name: str = None,
                          engine_path: str or pathlib.Path = None) -> int:
    """
    remove a gem to cmake as an external subdirectory
    :param gem_name: name of the gem to remove from cmake
    :param gem_path: the path of the gem to add to cmake
    :param engine_name: optional name of the engine to remove from cmake
    :param engine_path: the path of the engine to remove external subdirectory from, defaults to this engine
    :return: 0 for success or non 0 failure code
    """
    if not gem_name and not gem_path:
        logger.error('Must specify either a Gem name or Gem Path.')
        return 1

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)

    if not gem_path:
        logger.error(f'Gem Path {gem_path} has not been registered.')
        return 1

    if not engine_name and not engine_path:
        engine_path = get_this_engine_path()

    if engine_name and not engine_path:
        engine_path = get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Engine Path {engine_path} is not registered.')
        return 1

    return remove_external_subdirectory(external_subdir=gem_path, engine_path=engine_path)


def add_gem_to_project(gem_name: str = None,
                       gem_path: str or pathlib.Path = None,
                       gem_target: str = None,
                       project_name: str = None,
                       project_path: str or pathlib.Path = None,
                       dependencies_file: str or pathlib.Path = None,
                       runtime_dependency: bool = False,
                       tool_dependency: bool = False,
                       server_dependency: bool = False,
                       platforms: str = 'Common',
                       add_to_cmake: bool = True) -> int:
    """
    add a gem to a project
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param gem_target: the name of the cmake gem module
    :param project_name: name of to the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param dependencies_file: if this dependency goes/is in a specific file
    :param runtime_dependency: bool to specify this is a runtime gem for the game
    :param tool_dependency: bool to specify this is a tool gem for the editor
    :param server_dependency: bool to specify this is a server gem for the server
    :param platforms: str to specify common or which specific platforms
    :param add_to_cmake: bool to specify that this gem should be added to cmake
    :return: 0 for success or non 0 failure code
    """
    # we need either a project name or path
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return 1

    # if project name resolve it into a path
    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)
    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    # get the engine name this project is associated with
    # and resolve that engines path
    project_json = project_path / 'project.json'
    if not valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return 1
    with project_json.open('r') as s:
        try:
            project_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Error loading Project json {project_json}: {str(e)}')
            return 1
        else:
            try:
                engine_name = project_json_data['engine']
            except Exception as e:
                logger.error(f'Project json {project_json} "engine" not found: {str(e)}')
                return 1
            else:
                engine_path = get_registered(engine_name=engine_name)
                if not engine_path:
                    logger.error(f'Engine {engine_name} is not registered.')
                    return 1

    # we need either a gem name or path
    if not gem_name and not gem_path:
        logger.error(f'Must either specify a Gem path or Gem Name.')
        return 1

    # if gem name resolve it into a path
    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)
    gem_path = pathlib.Path(gem_path).resolve()
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if not gem_path.is_dir():
        logger.error(f'Gem Path {gem_path} does not exist.')
        return 1

    # if add to cmake, make sure the gem.json exists and valid before we proceed
    if add_to_cmake:
        gem_json = gem_path / 'gem.json'
        if not gem_json.is_file():
            logger.error(f'Gem json {gem_json} is not present.')
            return 1
        if not valid_o3de_gem_json(gem_json):
            logger.error(f'Gem json {gem_json} is not valid.')
            return 1

    # find all available modules in this gem_path
    modules = get_gem_targets(gem_path=gem_path)
    if len(modules) == 0:
        logger.error(f'No gem modules found under {gem_path}.')
        return 1

    # if the gem has no modules and the user has specified a target fail
    if gem_target and not modules:
        logger.error(f'Gem has no targets, but gem target {gem_target} was specified.')
        return 1

    # if the gem target is not in the modules
    if gem_target not in modules:
        logger.error(f'Gem target not in gem modules: {modules}')
        return 1

    if gem_target:
        # if the user has not specified either we will assume they meant the most common which is runtime
        if not runtime_dependency and not tool_dependency and not server_dependency and not dependencies_file:
            logger.warning("Dependency type not specified: Assuming '--runtime-dependency'")
            runtime_dependency = True

        ret_val = 0

        # if the user has specified the dependencies file then ignore the runtime_dependency and tool_dependency flags
        if dependencies_file:
            dependencies_file = pathlib.Path(dependencies_file).resolve()
            # make sure this is a project has a dependencies_file
            if not dependencies_file.is_file():
                logger.error(f'Dependencies file {dependencies_file} is not present.')
                return 1
            # add the dependency
            ret_val = add_gem_dependency(dependencies_file, gem_target)

        else:
            if ',' in platforms:
                platforms = platforms.split(',')
            else:
                platforms = [platforms]
            for platform in platforms:
                if runtime_dependency:
                    # make sure this is a project has a runtime_dependencies.cmake file
                    project_runtime_dependencies_file = pathlib.Path(
                        get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform)).resolve()
                    if not project_runtime_dependencies_file.is_file():
                        logger.error(f'Runtime dependencies file {project_runtime_dependencies_file} is not present.')
                        return 1
                    # add the dependency
                    ret_val = add_gem_dependency(project_runtime_dependencies_file, gem_target)

                if (ret_val == 0) and tool_dependency:
                    # make sure this is a project has a tool_dependencies.cmake file
                    project_tool_dependencies_file = pathlib.Path(
                        get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform)).resolve()
                    if not project_tool_dependencies_file.is_file():
                        logger.error(f'Tool dependencies file {project_tool_dependencies_file} is not present.')
                        return 1
                    # add the dependency
                    ret_val = add_gem_dependency(project_tool_dependencies_file, gem_target)

                if (ret_val == 0) and server_dependency:
                    # make sure this is a project has a tool_dependencies.cmake file
                    project_server_dependencies_file = pathlib.Path(
                        get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform)).resolve()
                    if not project_server_dependencies_file.is_file():
                        logger.error(f'Server dependencies file {project_server_dependencies_file} is not present.')
                        return 1
                    # add the dependency
                    ret_val = add_gem_dependency(project_server_dependencies_file, gem_target)

    if not ret_val and add_to_cmake:
        ret_val = add_gem_to_cmake(gem_path=gem_path, engine_path=engine_path)

    return ret_val


def remove_gem_from_project(gem_name: str = None,
                            gem_path: str or pathlib.Path = None,
                            gem_target: str = None,
                            project_name: str = None,
                            project_path: str or pathlib.Path = None,
                            dependencies_file: str or pathlib.Path = None,
                            runtime_dependency: bool = False,
                            tool_dependency: bool = False,
                            server_dependency: bool = False,
                            platforms: str = 'Common',
                            remove_from_cmake: bool = False) -> int:
    """
    remove a gem from a project
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param gem_target: the name of teh cmake gem module
    :param project_name: name of the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param dependencies_file: if this dependency goes/is in a specific file
    :param runtime_dependency: bool to specify this is a runtime gem for the game
    :param tool_dependency: bool to specify this is a tool gem for the editor
    :param server_dependency: bool to specify this is a server gem for the server
    :param platforms: str to specify common or which specific platforms
    :param remove_from_cmake: bool to specify that this gem should be removed from cmake
    :return: 0 for success or non 0 failure code
    """

    # we need either a project name or path
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return 1

    # if project name resolve it into a path
    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)
    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    # We need either a gem name or path
    if not gem_name and not gem_path:
        logger.error(f'Must either specify a Gem path or Gem Name.')
        return 1

    # if gem name resolve it into a path
    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)
    gem_path = pathlib.Path(gem_path).resolve()
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if not gem_path.is_dir():
        logger.error(f'Gem Path {gem_path} does not exist.')
        return 1

    # find all available modules in this gem_path
    modules = get_gem_targets(gem_path=gem_path)
    if len(modules) == 0:
        logger.error(f'No gem modules found.')
        return 1

    # if the user has not set a specific gem target remove all of them

    # if gem target not specified, see if there is only 1 module
    if not gem_target:
        if len(modules) == 1:
            gem_target = modules[0]
        else:
            logger.error(f'Gem target not specified: {modules}')
            return 1
    elif gem_target not in modules:
        logger.error(f'Gem target not in gem modules: {modules}')
        return 1

    # if the user has not specified either we will assume they meant the most common which is runtime
    if not runtime_dependency and not tool_dependency and not server_dependency and not dependencies_file:
        logger.warning("Dependency type not specified: Assuming '--runtime-dependency'")
        runtime_dependency = True

    # when removing we will try to do as much as possible even with failures so ret_val will be the last error code
    ret_val = 0

    # if the user has specified the dependencies file then ignore the runtime_dependency and tool_dependency flags
    if dependencies_file:
        dependencies_file = pathlib.Path(dependencies_file).resolve()
        # make sure this is a project has a dependencies_file
        if not dependencies_file.is_file():
            logger.error(f'Dependencies file {dependencies_file} is not present.')
            return 1
        # remove the dependency
        error_code = remove_gem_dependency(dependencies_file, gem_target)
        if error_code:
            ret_val = error_code
    else:
        if ',' in platforms:
            platforms = platforms.split(',')
        else:
            platforms = [platforms]
        for platform in platforms:
            if runtime_dependency:
                # make sure this is a project has a runtime_dependencies.cmake file
                project_runtime_dependencies_file = pathlib.Path(
                    get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform)).resolve()
                if not project_runtime_dependencies_file.is_file():
                    logger.error(f'Runtime dependencies file {project_runtime_dependencies_file} is not present.')
                else:
                    # remove the dependency
                    error_code = remove_gem_dependency(project_runtime_dependencies_file, gem_target)
                    if error_code:
                        ret_val = error_code

            if tool_dependency:
                # make sure this is a project has a tool_dependencies.cmake file
                project_tool_dependencies_file = pathlib.Path(
                    get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform)).resolve()
                if not project_tool_dependencies_file.is_file():
                    logger.error(f'Tool dependencies file {project_tool_dependencies_file} is not present.')
                else:
                    # remove the dependency
                    error_code = remove_gem_dependency(project_tool_dependencies_file, gem_target)
                    if error_code:
                        ret_val = error_code

            if server_dependency:
                # make sure this is a project has a tool_dependencies.cmake file
                project_server_dependencies_file = pathlib.Path(
                    get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform)).resolve()
                if not project_server_dependencies_file.is_file():
                    logger.error(f'Server dependencies file {project_server_dependencies_file} is not present.')
                else:
                    # remove the dependency
                    error_code = remove_gem_dependency(project_server_dependencies_file, gem_target)
                    if error_code:
                        ret_val = error_code

    if remove_from_cmake:
        error_code = remove_gem_from_cmake(gem_path=gem_path)
        if error_code:
            ret_val = error_code

    return ret_val


def sha256(file_path: str or pathlib.Path,
           json_path: str or pathlib.Path = None) -> int:
    if not file_path:
        logger.error(f'File path cannot be empty.')
        return 1
    file_path = pathlib.Path(file_path).resolve()
    if not file_path.is_file():
        logger.error(f'File path {file_path} does not exist.')
        return 1

    if json_path:
        json_path = pathlib.Path(json_path).resolve()
        if not json_path.is_file():
            logger.error(f'Json path {json_path} does not exist.')
            return 1

    sha256 = hashlib.sha256(file_path.open('rb').read()).hexdigest()

    if json_path:
        with json_path.open('r') as s:
            try:
                json_data = json.load(s)
            except Exception as e:
                logger.error(f'Failed to read Json path {json_path}: {str(e)}')
                return 1
        json_data.update({"sha256": sha256})
        backup_file(json_path)
        with json_path.open('w') as s:
            try:
                s.write(json.dumps(json_data, indent=4))
            except Exception as e:
                logger.error(f'Failed to write Json path {json_path}: {str(e)}')
                return 1
    else:
        print(sha256)
    return 0


def _run_get_registered(args: argparse) -> str or pathlib.Path:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return get_registered(args.engine_name,
                          args.project_name,
                          args.gem_name,
                          args.template_name,
                          args.default_folder,
                          args.repo_name,
                          args.restricted_name)


def _run_register_show(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    if args.this_engine:
        print_this_engine(args.verbose)
        return 0

    elif args.engines:
        print_engines(args.verbose)
        return 0
    elif args.projects:
        print_projects(args.verbose)
        return 0
    elif args.gems:
        print_gems(args.verbose)
        return 0
    elif args.templates:
        print_templates(args.verbose)
        return 0
    elif args.repos:
        register_show_repos(args.verbose)
        return 0
    elif args.restricted:
        print_restricted(args.verbose)
        return 0

    elif args.engine_projects:
        print_engine_projects(args.verbose)
        return 0
    elif args.engine_gems:
        print_engine_gems(args.verbose)
        return 0
    elif args.engine_templates:
        print_engine_templates(args.verbose)
        return 0
    elif args.engine_restricted:
        print_engine_restricted(args.verbose)
        return 0
    elif args.external_subdirectories:
        print_external_subdirectories(args.verbose)
        return 0

    elif args.all_projects:
        print_all_projects(args.verbose)
        return 0
    elif args.all_gems:
        print_all_gems(args.verbose)
        return 0
    elif args.all_templates:
        print_all_templates(args.verbose)
        return 0
    elif args.all_restricted:
        print_all_restricted(args.verbose)
        return 0

    elif args.downloadables:
        print_downloadables(args.verbose)
        return 0
    if args.downloadable_engines:
        print_downloadable_engines(args.verbose)
        return 0
    elif args.downloadable_projects:
        print_downloadable_projects(args.verbose)
        return 0
    elif args.downloadable_gems:
        print_downloadable_gems(args.verbose)
        return 0
    elif args.downloadable_templates:
        print_downloadable_templates(args.verbose)
        return 0
    else:
        register_show(args.verbose)
        return 0


def _run_download(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    if args.engine_name:
        return download_engine(args.engine_name,
                               args.dest_path)
    elif args.project_name:
        return download_project(args.project_name,
                                args.dest_path)
    elif args.gem_nanme:
        return download_gem(args.gem_name,
                            args.dest_path)
    elif args.template_name:
        return download_template(args.template_name,
                                 args.dest_path)


def _run_register(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    if args.update:
        remove_invalid_o3de_objects()
        return refresh_repos()
    elif args.this_engine:
        ret_val = register(engine_path=get_this_engine_path())
        error_code = register_shipped_engine_o3de_objects()
        if error_code:
            ret_val = error_code
        return ret_val
    elif args.all_engines_path:
        return register_all_engines_in_folder(args.all_engines_path, args.remove)
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
                        remove=args.remove)


def _run_add_external_subdirectory(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return add_external_subdirectory(args.external_subdirectory)


def _run_remove_external_subdirectory(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return remove_external_subdirectory(args.external_subdirectory)


def _run_add_gem_to_cmake(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return add_gem_to_cmake(gem_name=args.gem_name, gem_path=args.gem_path)


def _run_remove_gem_from_cmake(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return remove_gem_from_cmake(args.gem_name, args.gem_path)


def _run_add_gem_to_project(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return add_gem_to_project(args.gem_name,
                              args.gem_path,
                              args.gem_target,
                              args.project_name,
                              args.project_path,
                              args.dependencies_file,
                              args.runtime_dependency,
                              args.tool_dependency,
                              args.server_dependency,
                              args.platforms,
                              args.add_to_cmake)


def _run_remove_gem_from_project(args: argparse) -> int:
    if args.override_home_folder:
        global override_home_folder
        override_home_folder = args.override_home_folder

    return remove_gem_from_project(args.gem_name,
                                   args.gem_path,
                                   args.gem_target,
                                   args.project_path,
                                   args.project_name,
                                   args.dependencies_file,
                                   args.runtime_dependency,
                                   args.tool_dependency,
                                   args.server_dependency,
                                   args.platforms,
                                   args.remove_from_cmake)


def _run_sha256(args: argparse) -> int:
    return sha256(args.file_path,
                  args.json_path)


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
    register_subparser.set_defaults(func=_run_register)

    # show
    register_show_subparser = subparsers.add_parser('register-show')
    group = register_show_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-te', '--this-engine', action='store_true', required=False,
                       default=False,
                       help='Just the local engines.')

    group.add_argument('-e', '--engines', action='store_true', required=False,
                       default=False,
                       help='Just the local engines.')
    group.add_argument('-p', '--projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects.')
    group.add_argument('-g', '--gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems.')
    group.add_argument('-t', '--templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates.')
    group.add_argument('-r', '--repos', action='store_true', required=False,
                       default=False,
                       help='Just the local repos. Ignores repos.')
    group.add_argument('-rs', '--restricted', action='store_true', required=False,
                       default=False,
                       help='The local restricted folders.')

    group.add_argument('-ep', '--engine-projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects. Ignores repos.')
    group.add_argument('-eg', '--engine-gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems. Ignores repos')
    group.add_argument('-et', '--engine-templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates. Ignores repos.')
    group.add_argument('-ers', '--engine-restricted', action='store_true', required=False,
                       default=False,
                       help='The restricted folders.')
    group.add_argument('-x', '--external-subdirectories', action='store_true', required=False,
                       default=False,
                       help='The external subdirectories.')

    group.add_argument('-ap', '--all-projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects. Ignores repos.')
    group.add_argument('-ag', '--all-gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems. Ignores repos')
    group.add_argument('-at', '--all-templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates. Ignores repos.')
    group.add_argument('-ars', '--all-restricted', action='store_true', required=False,
                       default=False,
                       help='The restricted folders.')

    group.add_argument('-d', '--downloadables', action='store_true', required=False,
                       default=False,
                       help='Combine all repos into a single list of resources.')
    group.add_argument('-de', '--downloadable-engines', action='store_true', required=False,
                       default=False,
                       help='Combine all repos engines into a single list of resources.')
    group.add_argument('-dp', '--downloadable-projects', action='store_true', required=False,
                       default=False,
                       help='Combine all repos projects into a single list of resources.')
    group.add_argument('-dg', '--downloadable-gems', action='store_true', required=False,
                       default=False,
                       help='Combine all repos gems into a single list of resources.')
    group.add_argument('-dt', '--downloadable-templates', action='store_true', required=False,
                       default=False,
                       help='Combine all repos templates into a single list of resources.')

    register_show_subparser.add_argument('-v', '--verbose', action='count', required=False,
                                         default=0,
                                         help='How verbose do you want the output to be.')

    register_show_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                         help='By default the home folder is the user folder, override it to this folder.')

    register_show_subparser.set_defaults(func=_run_register_show)

    # get-registered
    get_registered_subparser = subparsers.add_parser('get-registered')
    group = get_registered_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-en', '--engine-name', type=str, required=False,
                       help='Engine name.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='Project name.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='Gem name.')
    group.add_argument('-tn', '--template-name', type=str, required=False,
                       help='Template name.')
    group.add_argument('-df', '--default-folder', type=str, required=False,
                       choices=['engines', 'projects', 'gems', 'templates', 'restricted'],
                       help='The default folders for o3de.')
    group.add_argument('-rn', '--repo-name', type=str, required=False,
                       help='Repo name.')
    group.add_argument('-rsn', '--restricted-name', type=str, required=False,
                       help='Restricted name.')

    get_registered_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                          help='By default the home folder is the user folder, override it to this folder.')

    get_registered_subparser.set_defaults(func=_run_get_registered)

    # download
    download_subparser = subparsers.add_parser('download')
    group = download_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--engine-name', type=str, required=False,
                       help='Downloadable engine name.')
    group.add_argument('-p', '--project-name', type=str, required=False,
                       help='Downloadable project name.')
    group.add_argument('-g', '--gem-name', type=str, required=False,
                       help='Downloadable gem name.')
    group.add_argument('-t', '--template-name', type=str, required=False,
                       help='Downloadable template name.')
    download_subparser.add_argument('-dp', '--dest-path', type=str, required=False,
                                    default=None,
                                    help='Optional destination folder to download into.'
                                         ' i.e. download --project-name "StarterGame" --dest-path "C:/projects"'
                                         ' will result in C:/projects/StarterGame'
                                         ' If blank will download to default object type folder')

    download_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                    help='By default the home folder is the user folder, override it to this folder.')

    download_subparser.set_defaults(func=_run_download)

    # add external subdirectories
    add_external_subdirectory_subparser = subparsers.add_parser('add-external-subdirectory')
    add_external_subdirectory_subparser.add_argument('external_subdirectory', metavar='external_subdirectory', type=str,
                                                     help='add an external subdirectory to cmake')

    add_external_subdirectory_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                                     help='By default the home folder is the user folder, override it to this folder.')

    add_external_subdirectory_subparser.set_defaults(func=_run_add_external_subdirectory)

    # remove external subdirectories
    remove_external_subdirectory_subparser = subparsers.add_parser('remove-external-subdirectory')
    remove_external_subdirectory_subparser.add_argument('external_subdirectory', metavar='external_subdirectory',
                                                        type=str,
                                                        help='remove external subdirectory from cmake')

    remove_external_subdirectory_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                                        help='By default the home folder is the user folder, override it to this folder.')

    remove_external_subdirectory_subparser.set_defaults(func=_run_remove_external_subdirectory)

    # add gems to cmake
    # convenience functions to disambiguate the gem name -> gem_path and call add-external-subdirectory on gem_path
    add_gem_to_cmake_subparser = subparsers.add_parser('add-gem-to-cmake')
    group = add_gem_to_cmake_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')

    add_gem_to_cmake_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                            help='By default the home folder is the user folder, override it to this folder.')

    add_gem_to_cmake_subparser.set_defaults(func=_run_add_gem_to_cmake)

    # remove gems from cmake
    # convenience functions to disambiguate the gem name -> gem_path and call remove-external-subdirectory on gem_path
    remove_gem_from_cmake_subparser = subparsers.add_parser('remove-gem-from-cmake')
    group = remove_gem_from_cmake_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')

    remove_gem_from_cmake_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                                 help='By default the home folder is the user folder, override it to this folder.')

    remove_gem_from_cmake_subparser.set_defaults(func=_run_remove_gem_from_cmake)

    # add a gem to a project
    add_gem_subparser = subparsers.add_parser('add-gem-to-project')
    group = add_gem_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=str, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = add_gem_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')
    add_gem_subparser.add_argument('-gt', '--gem-target', type=str, required=False,
                                   help='The cmake target name to add. If not specified it will assume gem_name')
    add_gem_subparser.add_argument('-df', '--dependencies-file', type=str, required=False,
                                   help='The cmake dependencies file in which the gem dependencies are specified.'
                                        'If not specified it will assume ')
    add_gem_subparser.add_argument('-rd', '--runtime-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a runtime dependency')
    add_gem_subparser.add_argument('-td', '--tool-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a tool dependency')
    add_gem_subparser.add_argument('-sd', '--server-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a server dependency')
    add_gem_subparser.add_argument('-pl', '--platforms', type=str, required=False,
                                   default='Common',
                                   help='Optional list of platforms this gem should be added to.'
                                        ' Ex. --platforms Mac,Windows,Linux')
    add_gem_subparser.add_argument('-a', '--add-to-cmake', type=bool, required=False,
                                   default=True,
                                   help='Automatically call add-gem-to-cmake.')

    add_gem_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                   help='By default the home folder is the user folder, override it to this folder.')

    add_gem_subparser.set_defaults(func=_run_add_gem_to_project)

    # remove a gem from a project
    remove_gem_subparser = subparsers.add_parser('remove-gem-from-project')
    group = remove_gem_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=str, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = remove_gem_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')
    remove_gem_subparser.add_argument('-gt', '--gem-target', type=str, required=False,
                                      help='The cmake target name to add. If not specified it will assume gem_name')
    remove_gem_subparser.add_argument('-df', '--dependencies-file', type=str, required=False,
                                      help='The cmake dependencies file in which the gem dependencies are specified.'
                                           'If not specified it will assume ')
    remove_gem_subparser.add_argument('-rd', '--runtime-dependency', action='store_true', required=False,
                                      default=False,
                                      help='Optional toggle if this gem should be removed as a runtime dependency')
    remove_gem_subparser.add_argument('-td', '--tool-dependency', action='store_true', required=False,
                                      default=False,
                                      help='Optional toggle if this gem should be removed as a server dependency')
    remove_gem_subparser.add_argument('-sd', '--server-dependency', action='store_true', required=False,
                                      default=False,
                                      help='Optional toggle if this gem should be removed as a server dependency')
    remove_gem_subparser.add_argument('-pl', '--platforms', type=str, required=False,
                                      default='Common',
                                      help='Optional list of platforms this gem should be removed from'
                                           ' Ex. --platforms Mac,Windows,Linux')
    remove_gem_subparser.add_argument('-r', '--remove-from-cmake', type=bool, required=False,
                                      default=False,
                                      help='Automatically call remove-from-cmake.')

    remove_gem_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                      help='By default the home folder is the user folder, override it to this folder.')

    remove_gem_subparser.set_defaults(func=_run_remove_gem_from_project)

    # sha256
    sha256_subparser = subparsers.add_parser('sha256')
    sha256_subparser.add_argument('-f', '--file-path', type=str, required=True,
                                  help='The path to the file you want to sha256.')
    sha256_subparser.add_argument('-j', '--json-path', type=str, required=False,
                                  help='optional path to an o3de json file to add the "sha256" element to.')
    sha256_subparser.set_defaults(func=_run_sha256)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help')

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args)

    # return
    sys.exit(ret)
