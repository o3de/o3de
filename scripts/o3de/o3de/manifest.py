#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Contains functions for data from json files such as the o3de_manifests.json, engine.json, project.json, etc...
"""

import json
import logging
import os
import pathlib
import shutil
import hashlib

from o3de import validation, utils

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

def get_o3de_user_folder() -> pathlib.Path:
    o3de_user_folder = get_home_folder() / 'O3DE'
    o3de_user_folder.mkdir(parents=True, exist_ok=True)
    return o3de_user_folder


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
    projects_folder = get_o3de_user_folder() / 'Projects'
    projects_folder.mkdir(parents=True, exist_ok=True)
    return projects_folder


def get_o3de_gems_folder() -> pathlib.Path:
    gems_folder = get_o3de_user_folder() / 'Gems'
    gems_folder.mkdir(parents=True, exist_ok=True)
    return gems_folder


def get_o3de_templates_folder() -> pathlib.Path:
    templates_folder = get_o3de_user_folder() / 'Templates'
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


def get_o3de_third_party_folder() -> pathlib.Path:
    third_party_folder = get_o3de_folder() / '3rdParty'
    third_party_folder.mkdir(parents=True, exist_ok=True)
    return third_party_folder


# o3de manifest file methods
def get_default_o3de_manifest_json_data() -> dict:
    """
    Returns dict with default values suitable for storing
    in the o3de_manifests.json
    """
    username = os.path.split(get_home_folder())[-1]

    o3de_folder = get_o3de_folder()
    default_engines_folder = get_o3de_engines_folder()
    default_projects_folder = get_o3de_projects_folder()
    default_gems_folder = get_o3de_gems_folder()
    default_templates_folder = get_o3de_templates_folder()
    default_restricted_folder = get_o3de_restricted_folder()
    default_third_party_folder = get_o3de_third_party_folder()

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
    json_data.update({'default_third_party_folder': default_third_party_folder.as_posix()})

    json_data.update({'engines': []})
    json_data.update({'projects': []})
    json_data.update({'external_subdirectories': []})
    json_data.update({'templates': []})
    json_data.update({'restricted': []})
    json_data.update({'repos': []})

    default_restricted_folder_json = default_restricted_folder / 'restricted.json'
    if not default_restricted_folder_json.is_file():
        with default_restricted_folder_json.open('w') as s:
            restricted_json_data = {}
            restricted_json_data.update({'restricted_name': 'o3de'})
            s.write(json.dumps(restricted_json_data, indent=4) + '\n')

    default_projects_restricted_folder_json = default_projects_restricted_folder / 'restricted.json'
    if not default_projects_restricted_folder_json.is_file():
        with default_projects_restricted_folder_json.open('w') as s:
            restricted_json_data = {}
            restricted_json_data.update({'restricted_name': 'projects'})
            s.write(json.dumps(restricted_json_data, indent=4) + '\n')

    default_gems_restricted_folder_json = default_gems_restricted_folder / 'restricted.json'
    if not default_gems_restricted_folder_json.is_file():
        with default_gems_restricted_folder_json.open('w') as s:
            restricted_json_data = {}
            restricted_json_data.update({'restricted_name': 'gems'})
            s.write(json.dumps(restricted_json_data, indent=4) + '\n')

    default_templates_restricted_folder_json = default_templates_restricted_folder / 'restricted.json'
    if not default_templates_restricted_folder_json.is_file():
        with default_templates_restricted_folder_json.open('w') as s:
            restricted_json_data = {}
            restricted_json_data.update({'restricted_name': 'templates'})
            s.write(json.dumps(restricted_json_data, indent=4) + '\n')

    return json_data

def get_o3de_manifest() -> pathlib.Path:
    manifest_path = get_o3de_folder() / 'o3de_manifest.json'
    if not manifest_path.is_file():
        json_data = get_default_o3de_manifest_json_data()

        with manifest_path.open('w') as s:
            s.write(json.dumps(json_data, indent=4) + '\n')

    return manifest_path


def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
    """
    Loads supplied manifest file or ~/.o3de/o3de_manifest.json if None

    raises Json.JSONDecodeError if manifest data could not be decoded to JSON
    :param manifest_path: optional path to manifest file to load
    """
    if not manifest_path:
        manifest_path = get_o3de_manifest()
    with manifest_path.open('r') as f:
        try:
            json_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'Manifest json failed to load at path "{manifest_path}": {str(e)}')
            # Re-raise the exception and let the caller
            # determine if they can proceed
            raise
        else:
            return json_data


def save_o3de_manifest(json_data: dict, manifest_path: pathlib.Path = None) -> bool:
    """
    Save the json dictionary to the supplied manifest file or ~/.o3de/o3de_manifest.json if None

    :param json_data: dictionary to save in json format at the file path
    :param manifest_path: optional path to manifest file to save
    """
    if not manifest_path:
        manifest_path = get_o3de_manifest()
    with manifest_path.open('w') as s:
        try:
            s.write(json.dumps(json_data, indent=4) + '\n')
            return True
        except OSError as e:
            logger.error(f'Manifest json failed to save: {str(e)}')
            return False


def get_gems_from_subdirectories(external_subdirs: list) -> list:
    '''
    Helper Method for scanning a set of external subdirectories for gem.json files
    '''
    def is_gem_subdirectory(subdir_files):
        for name in files:
            if name == 'gem.json':
                return True
        return False

    gem_directories = []
    # Locate all subfolders with gem.json files within them
    if external_subdirs:
        for subdirectory in external_subdirs:
            for root, dirs, files in os.walk(pathlib.Path(subdirectory).resolve()):
                if is_gem_subdirectory(files):
                    gem_directories.append(pathlib.PurePath(root).as_posix())

    return gem_directories


def get_engines() -> list:
    json_data = load_o3de_manifest()
    engine_list = json_data['engines'] if 'engines' in json_data else []
    # Convert each engine dict entry into a string entry
    return list(map(
        lambda engine_object: engine_object.get('path', '') if isinstance(engine_object, dict) else engine_object,
        engine_list))


def get_projects() -> list:
    json_data = load_o3de_manifest()
    return json_data['projects'] if 'projects' in json_data else []


def get_gems() -> list:
    return get_gems_from_subdirectories(get_external_subdirectories())


def get_external_subdirectories() -> list:
    json_data = load_o3de_manifest()
    return json_data['external_subdirectories'] if 'external_subdirectories' in json_data else []


def get_templates() -> list:
    json_data = load_o3de_manifest()
    return json_data['templates'] if 'templates' in json_data else []


def get_restricted() -> list:
    json_data = load_o3de_manifest()
    return json_data['restricted'] if 'restricted' in json_data else []


def get_repos() -> list:
    json_data = load_o3de_manifest()
    return json_data['repos'] if 'repos' in json_data else []

# engine.json queries
def get_engine_projects() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    if engine_object:
        return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                          engine_object['projects'])) if 'projects' in engine_object else []
    return []


def get_engine_gems() -> list:
    return get_gems_from_subdirectories(get_engine_external_subdirectories())


def get_engine_external_subdirectories() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    if engine_object:
        return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                        engine_object['external_subdirectories'])) if 'external_subdirectories' in engine_object else []
    return []


def get_engine_templates() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    if engine_object:
        return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                          engine_object['templates'])) if 'templates' in engine_object else []
    return []


def get_engine_restricted() -> list:
    engine_path = get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    if engine_object:
        return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                        engine_object['restricted'])) if 'restricted' in engine_object else []
    return []


# project.json queries
def get_project_gems(project_path: pathlib.Path) -> list:
    return get_gems_from_subdirectories(get_project_external_subdirectories(project_path))


def get_project_external_subdirectories(project_path: pathlib.Path) -> list:
    project_object = get_project_json_data(project_path=project_path)
    if project_object:
        return list(map(lambda rel_path: (pathlib.Path(project_path) / rel_path).as_posix(),
                   project_object['external_subdirectories'])) if 'external_subdirectories' in project_object else []
    return []


def get_project_templates(project_path: pathlib.Path) -> list:
    project_object = get_project_json_data(project_path=project_path)
    if project_object:
        return list(map(lambda rel_path: (pathlib.Path(project_path) / rel_path).as_posix(),
                          project_object['templates'])) if 'templates' in project_object else []
    return []


def get_project_restricted(project_path: pathlib.Path) -> list:
    project_object = get_project_json_data(project_path=project_path)
    if project_object:
        return list(map(lambda rel_path: (pathlib.Path(project_path) / rel_path).as_posix(),
                        project_object['restricted'])) if 'restricted' in project_object else []
    return []


# Combined manifest queries
def get_all_projects() -> list:
    projects_data = get_projects()
    projects_data.extend(get_engine_projects())
    # Remove duplicates from the list
    return list(dict.fromkeys(projects_data))


def get_all_gems(project_path: pathlib.Path = None) -> list:
    gems_data = get_gems()
    gems_data.extend(get_engine_gems())
    if project_path:
        gems_data.extend(get_project_gems(project_path))
    return list(dict.fromkeys(gems_data))


def get_all_external_subdirectories(project_path: pathlib.Path = None) -> list:
    external_subdirectories_data = get_external_subdirectories()
    external_subdirectories_data.extend(get_engine_external_subdirectories())
    if project_path:
        external_subdirectories_data.extend(get_project_external_subdirectories(project_path))
    return list(dict.fromkeys(external_subdirectories_data))


def get_all_templates(project_path: pathlib.Path = None) -> list:
    templates_data = get_templates()
    templates_data.extend(get_engine_templates())
    if project_path:
        templates_data.extend(get_project_templates(project_path))
    return list(dict.fromkeys(templates_data))


def get_all_restricted(project_path: pathlib.Path = None) -> list:
    restricted_data = get_restricted()
    restricted_data.extend(get_engine_restricted())
    if project_path:
        restricted_data.extend(get_project_restricted(project_path))
    return list(dict.fromkeys(restricted_data))


# Template functions
def get_templates_for_project_creation():
    project_templates = []
    for template_path in get_all_templates():
        template_path = pathlib.Path(template_path)
        template_json_path = pathlib.Path(template_path) / 'template.json'
        if not validation.valid_o3de_template_json(template_json_path):
            continue

        project_json_path = template_path / 'Template' / 'project.json'
        if validation.valid_o3de_project_json(project_json_path):
            project_templates.append(template_path)
    return project_templates


def get_templates_for_gem_creation():
    gem_templates = []
    for template_path in get_all_templates():
        template_path = pathlib.Path(template_path)
        template_json_path = pathlib.Path(template_path) / 'template.json'
        if not validation.valid_o3de_template_json(template_json_path):
            continue

        gem_json_path = template_path / 'Template' / 'gem.json'
        if validation.valid_o3de_gem_json(gem_json_path):
            gem_templates.append(template_path)
    return gem_templates


def get_templates_for_generic_creation():  # temporary until we have a better way to do this... maybe template_type element
    def filter_project_and_gem_templates_out(template_path,
                                             templates_for_project_creation = get_templates_for_project_creation(),
                                             templates_for_gem_creation = get_templates_for_gem_creation()):
        template_path = pathlib.Path(template_path)
        return template_path not in templates_for_project_creation and template_path not in templates_for_gem_creation

    return list(filter(filter_project_and_gem_templates_out, get_all_templates()))

def get_json_file_path(object_typename: str,
                       object_path: str or pathlib.Path) -> pathlib.Path:
    if not object_typename or not object_path:
        logger.error('Must specify an object typename and object path.')
        return None

    object_path = pathlib.Path(object_path).resolve()
    return object_path / f'{object_typename}.json'


def get_json_data_file(object_json: pathlib.Path,
                       object_typename: str,
                       object_validator: callable) -> dict or None:
    if not object_typename:
        logger.error('Missing object typename.')
        return None

    if not object_json or not object_json.is_file():
        logger.error(f'Invalid {object_typename} json {object_json} supplied or file missing.')
        return None

    if not object_validator or not object_validator(object_json):
        logger.error(f'{object_typename} json {object_json} is not valid or could not be validated.')
        return None

    with object_json.open('r') as f:
        try:
            object_json_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.warning(f'{object_json} failed to load: {e}')
        else:
            return object_json_data

    return None

def get_json_data(object_typename: str,
                  object_path: str or pathlib.Path,
                  object_validator: callable) -> dict or None:
    object_json = get_json_file_path(object_typename, object_path)

    return get_json_data_file(object_json, object_typename, object_validator)


def get_engine_json_data(engine_name: str = None,
                         engine_path: str or pathlib.Path = None) -> dict or None:
    if not engine_name and not engine_path:
        logger.error('Must specify either a Engine name or Engine Path.')
        return None

    if engine_name and not engine_path:
        engine_path = get_registered(engine_name=engine_name)

    return get_json_data('engine', engine_path, validation.valid_o3de_engine_json)


def get_project_json_data(project_name: str = None,
                          project_path: str or pathlib.Path = None) -> dict or None:
    if not project_name and not project_path:
        logger.error('Must specify either a Project name or Project Path.')
        return None

    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)

    return get_json_data('project', project_path, validation.valid_o3de_project_json)


def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                      project_path: pathlib.Path = None) -> dict or None:
    if not gem_name and not gem_path:
        logger.error('Must specify either a Gem name or Gem Path.')
        return None

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name, project_path=project_path)

    if pathlib.Path(gem_path).is_file():
        return get_json_data_file(gem_path, 'gem', validation.valid_o3de_gem_json)
    else:
        return get_json_data('gem', gem_path, validation.valid_o3de_gem_json)


def get_template_json_data(template_name: str = None, template_path: str or pathlib.Path = None,
                           project_path: pathlib.Path = None) -> dict or None:
    if not template_name and not template_path:
        logger.error('Must specify either a Template name or Template Path.')
        return None

    if template_name and not template_path:
        template_path = get_registered(template_name=template_name, project_path=project_path)

    return get_json_data('template', template_path, validation.valid_o3de_template_json)


def get_restricted_json_data(restricted_name: str = None, restricted_path: str or pathlib.Path = None,
                             project_path: pathlib.Path = None) -> dict or None:
    if not restricted_name and not restricted_path:
        logger.error('Must specify either a Restricted name or Restricted Path.')
        return None

    if restricted_name and not restricted_path:
        restricted_path = get_registered(restricted_name=restricted_name, project_path=project_path)

    return get_json_data('restricted', restricted_path, validation.valid_o3de_restricted_json)

def get_repo_json_data(repo_uri: str) -> dict or None:
    if not repo_uri:
        logger.error('Must specify a Repo Uri.')
        return None

    repo_json = get_repo_path(repo_uri=repo_uri)

    return get_json_data_file(repo_json, "Repo", validation.valid_o3de_repo_json)

def get_repo_path(repo_uri: str, cache_folder: str = None) -> pathlib.Path:
    if not cache_folder:
        cache_folder = get_o3de_cache_folder()

    repo_manifest = f'{repo_uri}/repo.json'
    repo_sha256 = hashlib.sha256(repo_manifest.encode())
    return cache_folder / str(repo_sha256.hexdigest() + '.json')

def get_registered(engine_name: str = None,
                   project_name: str = None,
                   gem_name: str = None,
                   template_name: str = None,
                   default_folder: str = None,
                   repo_name: str = None,
                   restricted_name: str = None,
                   project_path: pathlib.Path = None) -> pathlib.Path or None:
    """
       Looks up a registered entry in either the  ~/.o3de/o3de_manifest.json, <this-engine-root>/engine.json
       or the <project-path>/project.json (if the project_path parameter is supplied)

       :param engine_name: Name of a registered engine to lookup in the ~/.o3de/o3de_manifest.json file
       :param project_name: Name of a project to lookup in either the ~/.o3de/o3de_manifest.json or
              <this-engine-root>/engine.json file
       :param gem_name: Name of a gem to lookup in either the ~/.o3de/o3de_manifest.json, <this-engine-root>/engine.json
            or <project-path>/project.json. NOTE: The project_path parameter must be supplied to lookup the registration
            with the project.json
       :param template_name: Name of a template to lookup in either the ~/.o3de/o3de_manifest.json, <this-engine-root>/engine.json
            or <project-path>/project.json. NOTE: The project_path parameter must be supplied to lookup the registration
            with the project.json
       :param repo_name: Name of a repo to lookup in the ~/.o3de/o3de_manifest.json
       :param default_folder: Type of "default" folder to lookup in the ~/.o3de/o3de_manifest.json
              Valid values are "engines", "projects", "gems", "templates,", "restricted"
       :param restricted_name: Name of a restricted directory object to lookup in either the ~/.o3de/o3de_manifest.json,
            <this-engine-root>/engine.json or <project-path>/project.json.
            NOTE: The project_path parameter must be supplied to lookup the registration with the project.json
       :param project_path: Path to project root, which is used to examined the project.json file in order to
              query either gems, templates or restricted directories registered with the project

       :return path value associated with the registered object name if found. Otherwise None is returned
    """
    json_data = load_o3de_manifest()

    # check global first then this engine
    if isinstance(engine_name, str):
        engines = get_engines()
        for engine in engines:
            if isinstance(engine, dict):
                engine_path = pathlib.Path(engine['path']).resolve()
            else:
                engine_path = pathlib.Path(engine).resolve()

            engine_json = engine_path / 'engine.json'
            with engine_json.open('r') as f:
                try:
                    engine_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{engine_json} failed to load: {str(e)}')
                else:
                    this_engines_name = engine_json_data['engine_name']
                    if this_engines_name == engine_name:
                        return engine_path

    elif isinstance(project_name, str):
        projects = get_all_projects()
        for project_path in projects:
            project_path = pathlib.Path(project_path).resolve()
            project_json = project_path / 'project.json'
            with project_json.open('r') as f:
                try:
                    project_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{project_json} failed to load: {str(e)}')
                else:
                    this_projects_name = project_json_data['project_name']
                    if this_projects_name == project_name:
                        return project_path

    elif isinstance(gem_name, str):
        gems = get_all_gems(project_path)
        for gem_path in gems:
            gem_path = pathlib.Path(gem_path).resolve()
            gem_json = gem_path / 'gem.json'
            with gem_json.open('r') as f:
                try:
                    gem_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{gem_json} failed to load: {str(e)}')
                else:
                    this_gems_name = gem_json_data['gem_name']
                    if this_gems_name == gem_name:
                        return gem_path

    elif isinstance(template_name, str):
        templates = get_all_templates(project_path)
        for template_path in templates:
            template_path = pathlib.Path(template_path).resolve()
            template_json = template_path / 'template.json'
            with template_json.open('r') as f:
                try:
                    template_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{template_path} failed to load: {str(e)}')
                else:
                    this_templates_name = template_json_data['template_name']
                    if this_templates_name == template_name:
                        return template_path

    elif isinstance(restricted_name, str):
        restricted = get_all_restricted(project_path)
        for restricted_path in restricted:
            restricted_path = pathlib.Path(restricted_path).resolve()
            restricted_json = restricted_path / 'restricted.json'
            with restricted_json.open('r') as f:
                try:
                    restricted_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{restricted_json} failed to load: {str(e)}')
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
            cache_file = get_repo_path(repo_uri=repo_uri, cache_folder=cache_folder)
            if cache_file.is_file():
                repo = pathlib.Path(cache_file).resolve()
                with repo.open('r') as f:
                    try:
                        repo_json_data = json.load(f)
                    except json.JSONDecodeError as e:
                        logger.warning(f'{cache_file} failed to load: {str(e)}')
                    else:
                        this_repos_name = repo_json_data['repo_name']
                        if this_repos_name == repo_name:
                            return repo_uri
    return None
