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
Contains functions for data from json files such as the o3de_manifests.json, engine.json, project.json, etc...
"""

import json
import logging
import os
import pathlib

from o3de import validation

logger = logging.getLogger()
logging.basicConfig()

# Directory methods
override_home_folder = None


def get_this_engine_path() -> pathlib.Path:
    return pathlib.Path(os.path.realpath(__file__)).parents[3].resolve()


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
    logs_folder = get_o3de_folder() / 'Logs'
    logs_folder.mkdir(parents=True, exist_ok=True)
    return logs_folder


# o3de manifest file methods
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
            return {}
        else:
            return json_data


def save_o3de_manifest(json_data: dict) -> None:
    with get_o3de_manifest().open('w') as s:
        try:
            s.write(json.dumps(json_data, indent=4))
        except Exception as e:
            logger.error(f'Manifest json failed to save: {str(e)}')


# Data query methods
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


def get_engine_projects() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                      engine_object['projects'])) if 'projects' in engine_object else []


def get_engine_gems() -> list:
    def is_gem_subdirectory(subdir):
        return (pathlib.Path(subdir) / 'gem.json').exists()

    external_subdirs = get_engine_external_subdirectories()
    return list(filter(is_gem_subdirectory, external_subdirs)) if external_subdirs else []


def get_engine_templates() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                      engine_object['templates']))


def get_engine_restricted() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                    engine_object['restricted'])) if 'restricted' in engine_object else []


def get_engine_external_subdirectories() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
               engine_object['external_subdirectories'])) if 'external_subdirectories' in engine_object else []


def get_all_projects() -> list:
    engine_projects = get_engine_projects()
    projects_data = get_projects()
    projects_data.extend(engine_projects)
    return projects_data


def get_all_gems() -> list:
    engine_gems = get_engine_gems()
    gems_data = get_gems()
    gems_data.extend(engine_gems)
    return gems_data


def get_all_templates() -> list:
    engine_templates = get_engine_templates()
    templates_data = get_templates()
    templates_data.extend(engine_templates)
    return templates_data


def get_all_restricted() -> list:
    engine_restricted = get_engine_restricted()
    restricted_data = get_restricted()
    restricted_data.extend(engine_restricted)
    return restricted_data


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


def get_engine_json_data(engine_name: str = None,
                         engine_path: str or pathlib.Path = None) -> dict or None:
    if not engine_name and not engine_path:
        logger.error('Must specify either a Engine name or Engine Path.')
        return None

    if engine_name and not engine_path:
        engine_path = get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Engine Path {engine_path} has not been registered.')
        return None

    engine_path = pathlib.Path(engine_path).resolve()
    engine_json = engine_path / 'engine.json'
    if not engine_json.is_file():
        logger.error(f'Engine json {engine_json} is not present.')
        return None
    if not validation.valid_o3de_engine_json(engine_json):
        logger.error(f'Engine json {engine_json} is not valid.')
        return None

    with engine_json.open('r') as f:
        try:
            engine_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{engine_json} failed to load: {str(e)}')
        else:
            return engine_json_data

    return None


def get_project_json_data(project_name: str = None,
                          project_path: str or pathlib.Path = None) -> dict or None:
    if not project_name and not project_path:
        logger.error('Must specify either a Project name or Project Path.')
        return None

    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)

    if not project_path:
        logger.error(f'Project Path {project_path} has not been registered.')
        return None

    project_path = pathlib.Path(project_path).resolve()
    project_json = project_path / 'project.json'
    if not project_json.is_file():
        logger.error(f'Project json {project_json} is not present.')
        return None
    if not validation.valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return None

    with project_json.open('r') as f:
        try:
            project_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{project_json} failed to load: {str(e)}')
        else:
            return project_json_data

    return None


def get_gem_json_data(gem_name: str = None,
                      gem_path: str or pathlib.Path = None) -> dict or None:
    if not gem_name and not gem_path:
        logger.error('Must specify either a Gem name or Gem Path.')
        return None

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name)

    if not gem_path:
        logger.error(f'Gem Path {gem_path} has not been registered.')
        return None

    gem_path = pathlib.Path(gem_path).resolve()
    gem_json = gem_path / 'gem.json'
    if not gem_json.is_file():
        logger.error(f'Gem json {gem_json} is not present.')
        return None
    if not validation.valid_o3de_gem_json(gem_json):
        logger.error(f'Gem json {gem_json} is not valid.')
        return None

    with gem_json.open('r') as f:
        try:
            gem_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{gem_json} failed to load: {str(e)}')
        else:
            return gem_json_data

    return None


def get_template_json_data(template_name: str = None,
                           template_path: str or pathlib.Path = None) -> dict or None:
    if not template_name and not template_path:
        logger.error('Must specify either a Template name or Template Path.')
        return None

    if template_name and not template_path:
        template_path = get_registered(template_name=template_name)

    if not template_path:
        logger.error(f'Template Path {template_path} has not been registered.')
        return None

    template_path = pathlib.Path(template_path).resolve()
    template_json = template_path / 'template.json'
    if not template_json.is_file():
        logger.error(f'Template json {template_json} is not present.')
        return None
    if not validation.valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_json} is not valid.')
        return None

    with template_json.open('r') as f:
        try:
            template_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{template_json} failed to load: {str(e)}')
        else:
            return template_json_data

    return None


def get_restricted_data(restricted_name: str = None,
                        restricted_path: str or pathlib.Path = None) -> dict or None:
    if not restricted_name and not restricted_path:
        logger.error('Must specify either a Restricted name or Restricted Path.')
        return None

    if restricted_name and not restricted_path:
        restricted_path = get_registered(restricted_name=restricted_name)

    if not restricted_path:
        logger.error(f'Restricted Path {restricted_path} has not been registered.')
        return None

    restricted_path = pathlib.Path(restricted_path).resolve()
    restricted_json = restricted_path / 'restricted.json'
    if not restricted_json.is_file():
        logger.error(f'Restricted json {restricted_json} is not present.')
        return None
    if not validation.valid_o3de_restricted_json(restricted_json):
        logger.error(f'Restricted json {restricted_json} is not valid.')
        return None

    with restricted_json.open('r') as f:
        try:
            restricted_json_data = json.load(f)
        except Exception as e:
            logger.warn(f'{restricted_json} failed to load: {str(e)}')
        else:
            return restricted_json_data

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
        enging_projects = get_engine_projects()
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
        engine_gems = get_engine_gems()
        gems = json_data['gems'].copy()
        gems.extend(engine_gems)
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
        engine_templates = get_engine_templates()
        templates = json_data['templates'].copy()
        templates.extend(engine_templates)
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
        engine_restricted = get_engine_restricted()
        restricted = json_data['restricted'].copy()
        restricted.extend(engine_restricted)
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
