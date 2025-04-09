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
from packaging.version import Version
from collections import deque

from o3de import validation, utils, repo, compatibility

logging.basicConfig(format=utils.LOG_FORMAT)
logger = logging.getLogger('o3de.manifest')
logger.setLevel(logging.INFO)

# Directory methods

def get_this_engine_path() -> pathlib.Path:
    # When running from SNAP, __file__ was returning an incorrect (temporary) folder so
    # we manually build the correct path from env variables here when running from snap
    if "SNAP" in os.environ and "SNAP_BUILD" in os.environ:
        return pathlib.Path(os.environ.get('SNAP')) / os.environ.get('SNAP_BUILD')
    else:
        return pathlib.Path(os.path.realpath(__file__)).parents[3].resolve()

def get_home_folder() -> pathlib.Path:
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
    engines_folder = get_o3de_user_folder() / 'Engines'
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
    restricted_folder = get_o3de_user_folder() / 'Restricted'
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
    default_registry_folder = get_o3de_registry_folder()
    default_cache_folder = get_o3de_cache_folder()
    default_downloads_folder = get_o3de_download_folder()
    default_logs_folder = get_o3de_logs_folder()
    default_engines_folder = get_o3de_engines_folder()
    default_projects_folder = get_o3de_projects_folder()
    default_gems_folder = get_o3de_gems_folder()
    default_templates_folder = get_o3de_templates_folder()
    default_restricted_folder = get_o3de_restricted_folder()
    default_third_party_folder = get_o3de_third_party_folder()

    default_restricted_projects_folder = default_restricted_folder / 'Projects'
    default_restricted_projects_folder.mkdir(parents=True, exist_ok=True)
    default_restricted_gems_folder = default_restricted_folder / 'Gems'
    default_restricted_gems_folder.mkdir(parents=True, exist_ok=True)
    default_restricted_engine_folder = default_restricted_folder / 'Engines' / 'o3de'
    default_restricted_engine_folder.mkdir(parents=True, exist_ok=True)
    default_restricted_templates_folder = default_restricted_folder / 'Templates'
    default_restricted_templates_folder.mkdir(parents=True, exist_ok=True)
    default_restricted_engine_folder_json = default_restricted_engine_folder / 'restricted.json'
    if not default_restricted_engine_folder_json.is_file():
        with default_restricted_engine_folder_json.open('w') as s:
            restricted_json_data = {}
            restricted_json_data.update({'restricted_name': 'o3de'})
            s.write(json.dumps(restricted_json_data, indent=4) + '\n')

    json_data = {}
    json_data.update({'o3de_manifest_name': f'{username}'})
    json_data.update({'origin': o3de_folder.as_posix()})
    json_data.update({'default_engines_folder': default_engines_folder.as_posix()})
    json_data.update({'default_projects_folder': default_projects_folder.as_posix()})
    json_data.update({'default_gems_folder': default_gems_folder.as_posix()})
    json_data.update({'default_templates_folder': default_templates_folder.as_posix()})
    json_data.update({'default_restricted_folder': default_restricted_folder.as_posix()})
    json_data.update({'default_third_party_folder': default_third_party_folder.as_posix()})
    json_data.update({'projects': []})
    json_data.update({'external_subdirectories': []})
    json_data.update({'templates': []})
    json_data.update({'restricted': [default_restricted_engine_folder.as_posix()]})
    json_data.update({'repos': []})
    json_data.update({'engines': []})
    return json_data

def get_o3de_manifest() -> pathlib.Path:
    return get_o3de_folder() / 'o3de_manifest.json'


def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
    """
    Loads supplied manifest file or ~/.o3de/o3de_manifest.json if None
    If the supplied manifest_path is None and  ~/.o3de/o3de_manifest.json doesn't exist default manifest data
    is instead returned.
    Note: There is a difference between supplying a manifest_path parameter of None and '~/.o3de/o3de_manifest.json'
    In the former if the o3de_manifest.json doesn't exist, default o3de manifest data is returned.
    In the later that the o3de_manifest.json must exist as the caller explicitly specified the manifest path

    raises Json.JSONDecodeError if manifest data could not be decoded to JSON
    :param manifest_path: optional path to manifest file to load
    """
    if not manifest_path:
        manifest_path = get_o3de_manifest()
        # If the default o3de manifest file doesn't exist and the manifest_path parameter was not supplied
        # return the default o3de manifest data
        if not manifest_path.is_file():
            logger.info(f'A manifest path of None has been supplied and the {manifest_path} does not exist.'
                        ' The default o3de manifest data dictionary will be returned.')
            return get_default_o3de_manifest_json_data()

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


def get_gems_from_external_subdirectories(external_subdirs: list) -> list:
    '''
    Helper Method for scanning a set of external subdirectories for gem.json files
    '''
    gem_directories = []

    if external_subdirs:
        for subdirectory in external_subdirs:
            gem_json_path = pathlib.Path(subdirectory).resolve() / 'gem.json'
            if gem_json_path.is_file():
                gem_directories.append(pathlib.PurePath(subdirectory).as_posix())

    return gem_directories


# Data query methods
def get_manifest_engines() -> list:
    json_data = load_o3de_manifest()
    engine_list = json_data['engines'] if 'engines' in json_data else []
    # Convert each engine dict entry into a string entry
    return list(map(
        lambda engine_object: engine_object.get('path', '') if isinstance(engine_object, dict) else engine_object,
        engine_list))


def get_manifest_projects() -> list:
    json_data = load_o3de_manifest()
    return json_data['projects'] if 'projects' in json_data else []


def get_manifest_gems() -> list:
    return get_gems_from_external_subdirectories(get_manifest_external_subdirectories())


def get_manifest_external_subdirectories() -> list:
    json_data = load_o3de_manifest()
    return json_data['external_subdirectories'] if 'external_subdirectories' in json_data else []


def get_manifest_templates() -> list:
    json_data = load_o3de_manifest()
    return json_data['templates'] if 'templates' in json_data else []


def get_manifest_restricted() -> list:
    json_data = load_o3de_manifest()
    return json_data['restricted'] if 'restricted' in json_data else []


def get_manifest_repos(project_path: pathlib.Path = None) -> list:
    repos = set()

    if project_path:
        project_json_data = get_project_json_data(project_path=project_path)
        if project_json_data:
            repos.update(set(project_json_data.get('repos', [])))

        # if a project is provided we only want the repos from the project's engine
        engine_path = get_project_engine_path(project_path=project_path)
    else:
        # no project path provided, use the current engine's repos
        engine_path = get_this_engine_path()

    if engine_path:
        engine_json_data = get_engine_json_data(engine_path=engine_path)
        if engine_json_data:
            repos.update(set(engine_json_data.get('repos', [])))

    manifest_json_data = load_o3de_manifest()
    if manifest_json_data:
        repos.update(set(manifest_json_data.get('repos', [])))


    return list(repos)


# engine.json queries
def get_engine_projects(engine_path:pathlib.Path = None) -> list:
    engine_path = engine_path or get_this_engine_path()
    engine_object = get_engine_json_data(engine_path=engine_path)
    if engine_object:
        return list(map(lambda rel_path: (pathlib.Path(engine_path) / rel_path).as_posix(),
                        engine_object['projects'])) if 'projects' in engine_object else []
    return []


def get_engine_gems(engine_path:pathlib.Path = None, recurse:bool = False, gems_json_data_by_path:dict = None) -> list:
    """
    Get gems from the external subdirectories in engine.json
    :param engine_path: the path to the engine
    :param recurse: whether to include gems in gems or not
    :param gems_json_data_by_path: optional dictionary of known gem json data by path to use instead of opening files
    :return: list of engine gem paths
    """
    engine_gem_paths = get_gems_from_external_subdirectories(get_engine_external_subdirectories(engine_path))
    if recurse:
        for path in engine_gem_paths:
            engine_gem_paths.extend(get_gem_external_subdirectories(path, list(), gems_json_data_by_path))
    
    return engine_gem_paths


def get_engine_external_subdirectories(engine_path:pathlib.Path = None) -> list:
    engine_path = engine_path or get_this_engine_path()
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

def is_sdk_engine(engine_name:str = None, engine_path:str or pathlib.Path = None) -> bool:
    """
    Queries if the engine is an SDK engine that was created via the CMake install command
    :param engine_name: Name of a registered engine to lookup in the ~/.o3de/o3de_manifest.json file
    :param engine_path: The path to the engine
    :return True if the engine.json contains an "sdk_engine" field which is True
    """
    engine_json_data = get_engine_json_data(engine_name, engine_path)
    if engine_json_data:
        return engine_json_data.get('sdk_engine', False)

    if engine_path:
        logger.warning('Failed to retrieve engine json data for the engine at '
                     f'"{engine_path}". Treating engine as source engine.')
    elif engine_name:
        logger.warning('Failed to retrieve engine json data for registered engine '
                     f'"{engine_name}". Treating engine as source engine.')
    else:
        logger.warning('Failed to retrieve engine json data as neither an engine name nor path were given.'
                     'Treating engine as source engine')
    return False


# project.json queries
def get_project_gems(project_path:pathlib.Path, recurse:bool = False, gems_json_data_by_path:dict = None) -> list:
    """
    Get gems from the external subdirectories in project.json
    :param project_path: the path to the project
    :param recurse: whether to include gems in gems or not
    :param gems_json_data_by_path: optional dictionary of known gem json data by path to use instead of opening files
    :return: list of project gem paths
    """
    project_gem_paths = get_gems_from_external_subdirectories(get_project_external_subdirectories(project_path))
    if recurse:
        for path in project_gem_paths:
            project_gem_paths.extend(get_gem_external_subdirectories(path, list(), gems_json_data_by_path))
    
    return project_gem_paths

def get_enabled_gem_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                platform: str = 'Common') -> pathlib.Path or None:
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
    enable_gem_filename = "enabled_gems.cmake"

    if platform == 'Common':
        possible_project_enable_gem_filename_paths = [
            pathlib.Path(project_path / 'Gem' / enable_gem_filename),
            pathlib.Path(project_path / 'Gem/Code' / enable_gem_filename),
            pathlib.Path(project_path / 'Code' / enable_gem_filename)
        ]
        for possible_project_enable_gem_filename_path in possible_project_enable_gem_filename_paths:
            if possible_project_enable_gem_filename_path.is_file():
                return possible_project_enable_gem_filename_path.resolve()
        return possible_project_enable_gem_filename_paths[0].resolve()
    else:
        possible_project_platform_enable_gem_filename_paths = [
            pathlib.Path(project_path / 'Gem/Platform' / platform / enable_gem_filename),
            pathlib.Path(project_path / 'Gem/Code/Platform' / platform / enable_gem_filename),
            pathlib.Path(project_path / 'Code/Platform' / platform / enable_gem_filename)
        ]
        for possible_project_platform_enable_gem_filename_path in possible_project_platform_enable_gem_filename_paths:
            if possible_project_platform_enable_gem_filename_path.is_file():
                return possible_project_platform_enable_gem_filename_path.resolve()
        return possible_project_platform_enable_gem_filename_paths[0].resolve()


def get_enabled_gems(cmake_file: pathlib.Path) -> set:
    """
    Gets a list of enabled gems from the cmake file
    :param cmake_file: path to the cmake file
    :return: set of gem targets found
    """
    cmake_file = pathlib.Path(cmake_file).resolve()

    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    enable_gem_start_marker = 'set(ENABLED_GEMS'
    enable_gem_end_marker = ')'

    gem_target_set = set()
    with cmake_file.open('r') as s:
        in_gem_list = False
        for line in s:
            line = line.strip()
            if line.startswith(enable_gem_start_marker):
                # Set the flag to indicate that we are in the ENABLED_GEMS variable
                in_gem_list = True
                # Skip pass the 'set(ENABLED_GEMS' marker just in case their are gems declared on the same line
                line = line[len(enable_gem_start_marker):]
            if in_gem_list:
                # Since we are inside the ENABLED_GEMS variable determine if the line has the end_marker of ')'
                if line.endswith(enable_gem_end_marker):
                    # Strip away the line end marker
                    line = line[:-len(enable_gem_end_marker)]
                    # Set the flag to indicate that we are no longer in the ENABLED_GEMS variable after this line
                    in_gem_list = False
                # Split the rest of the line on whitespace just in case there are multiple gems in a line
                gem_name_list = list(map(lambda gem_name: gem_name.strip('"'), line.split()))
                gem_target_set.update(gem_name_list)

    return gem_target_set


FALLBACK_ENGINE_PROJECT_PATHS_WARNINGS = set()

def get_project_enabled_gems(project_path: pathlib.Path, include_dependencies:bool = True) -> dict or None:
    """
    Returns a dictionary of "<gem name with optional specifier>":"<gem path>"
    Example: {"gemA>=1.2.3":"c:/gemA", "gemB":"c:/gemB"}
    :param project_path The path to the project
    :param include_gem_dependencies True to include all gem dependencies, otherwise just return
    gems listed in project.json and the deprecated enabled_gems.json
    """
    project_json_data = get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f"Failed to get project json data for the project at '{project_path}'")
        return None

    active_gem_names = project_json_data.get('gem_names',[])
    enabled_gems_file = get_enabled_gem_cmake_file(project_path=project_path)
    if enabled_gems_file and enabled_gems_file.is_file():
        active_gem_names.extend(get_enabled_gems(enabled_gems_file))

    gem_names_with_optional_gems = utils.get_gem_names_set(active_gem_names, include_optional=True)
    if not gem_names_with_optional_gems:
        return {}
    
    # We have the gem names but not the resolved paths yet
    result = {gem_name: None for gem_name in gem_names_with_optional_gems}

    engine_path = get_project_engine_path(project_path=project_path)
    if not engine_path:
        engine_path = get_this_engine_path()
        if not engine_path:
            logger.error('Failed to find an engine path for the project at '
                            f'"{project_path}" which is required to resolve gem dependencies.')
            return result

        # Warn about falling back to a default engine once per project
        global FALLBACK_ENGINE_PROJECT_PATHS_WARNINGS
        if project_path not in FALLBACK_ENGINE_PROJECT_PATHS_WARNINGS:
            FALLBACK_ENGINE_PROJECT_PATHS_WARNINGS.add(project_path)
            logger.warning('Failed to determine the correct engine for the project at '
                           f'"{project_path}", falling back to this engine at {engine_path}.')
    
    engine_json_data = get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error('Failed to retrieve engine json data for the engine at '
                     f'"{engine_path}" which is required to resolve gem dependencies.')
        return result 

    all_gems_json_data = get_gems_json_data_by_name(engine_path=engine_path, 
                                                    project_path=project_path, 
                                                    include_manifest_gems=True, 
                                                    include_engine_gems=True)

    # we need a mapping of gem name to gem name with version specifier because
    # the resolver will remove the version specifier
    gem_names_with_version_specifiers = {}
    for gem_name_with_specifier in gem_names_with_optional_gems:
        gem_name_only, _ = utils.get_object_name_and_optional_version_specifier(gem_name_with_specifier)
        gem_names_with_version_specifiers[gem_name_only] = gem_name_with_specifier

    # Try to resolve with optional gems
    resolved_gems, errors = compatibility.resolve_gem_dependencies(gem_names_with_optional_gems, 
                                                             all_gems_json_data, 
                                                             engine_json_data, 
                                                             include_optional=True)
    if errors:
        # Try without optional gems
        gem_names_without_optional = utils.get_gem_names_set(active_gem_names, include_optional=False)
        resolved_gems, errors = compatibility.resolve_gem_dependencies(gem_names_without_optional, 
                                                                 all_gems_json_data, 
                                                                 engine_json_data,
                                                                 include_optional=False)
    if not errors:
        for _, gem in resolved_gems.items():
            gem_name = gem.gem_json_data['gem_name']
            gem_name_with_specifier = gem_names_with_version_specifiers.get(gem_name,gem_name)
            if gem_name_with_specifier in result or include_dependencies:
                result[gem_name_with_specifier] = gem.gem_json_data['path'].as_posix() if gem.gem_json_data['path'] else None
    else:
        # Likely there is no resolution because gems are missing or wrong version
        # Provide the paths for the gems that are available
        for gem_name in result.keys():
            gem_path = get_most_compatible_gem(gem_name, all_gems_json_data)
            if gem_path:
                result[gem_name] = gem_path.resolve().as_posix()
    return result


def get_project_external_subdirectories(project_path: pathlib.Path) -> list:
    project_object = get_project_json_data(project_path=project_path)
    if project_object:
        return list(map(lambda rel_path: (pathlib.Path(project_path) / rel_path).as_posix(),
                        project_object['external_subdirectories'])) if 'external_subdirectories' in project_object else []
    return []

def get_project_engine_path(project_path: pathlib.Path, 
                            project_json_data: dict = None, 
                            user_project_json_data: dict = None, 
                            engines_json_data: dict = None) -> pathlib.Path or None:
    """
    Returns the most compatible engine path for a project based on the project's 'engine' field and taking into account
    <project_path>/user/project.json overrides or the engine the project is registered with.
    :param project_path: Path to the project
    :param project_json_data: Optional json data to use to avoid reloading project.json  
    :param user_project_json_data: Optional json data to use to avoid reloading <project_path>/user/project.json  
    :param engines_json_data: Optional engines json data to use for engines to avoid reloading all engine.json files
    """
    engine_path = compatibility.get_most_compatible_project_engine_path(project_path, 
                                                                        project_json_data, 
                                                                        user_project_json_data, 
                                                                        engines_json_data)
    if engine_path:
        return engine_path

    # check if the project is registered in an engine.json
    # in a parent folder
    resolved_project_path = pathlib.Path(project_path).resolve()
    engine_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('engine.json'), resolved_project_path)
    if engine_path:
        projects = get_engine_projects(engine_path)
        for engine_project_path in projects:
            if resolved_project_path.samefile(pathlib.Path(engine_project_path).resolve()):
                return engine_path

    return None

def get_project_templates(project_path: pathlib.Path) -> list:
    project_object = get_project_json_data(project_path=project_path)
    if project_object:
        return list(map(lambda rel_path: (pathlib.Path(project_path) / rel_path).as_posix(),
                        project_object['templates'])) if 'templates' in project_object else []
    return []


# gem.json queries
def get_gem_gems(gem_path: pathlib.Path) -> list:
    return get_gems_from_external_subdirectories(get_gem_external_subdirectories(gem_path, list(), dict()))


def get_gem_external_subdirectories(gem_path: pathlib.Path, visited_gem_paths: list, gems_json_data_by_path: dict = None) -> list:
    '''
    recursively visit each gems "external_subdirectories" entries and return them in a list
    :param: gem_path path to the gem whose gem.json will be queried for the "external_subdirectories" field
    :param: visited_gem_paths stores the list of gem paths visited so far up until this get_path
    The visited_gem_paths is a list instead of a set to maintain insertion order
    :param: gems_json_data_by_path a cache of gem.json data with the gem_path as the key 
    '''

    # Resolve the path before to make sure it is absolute before adding to the visited_gem_paths set
    gem_path = pathlib.Path(gem_path).resolve()
    if gem_path in visited_gem_paths:
        logger.warning(f'A cycle has been detected when visiting external subdirectories at gem path "{gem_path}". The visited paths are: {visited_gem_paths}')
        return []
    visited_gem_paths.append(gem_path)

    if isinstance(gems_json_data_by_path, dict):
        # Use the cache 
        if gem_path in gems_json_data_by_path:
            gem_object = gems_json_data_by_path[gem_path]
        else:
            gem_object = get_gem_json_data(gem_path=gem_path)
            # store the value even if its None so we don't open the file again
            gems_json_data_by_path[gem_path] = gem_object
    else:
        gem_object = get_gem_json_data(gem_path=gem_path)

    external_subdirectories = []
    if gem_object:
        external_subdirectories = list(map(lambda rel_path: (pathlib.Path(gem_path) / rel_path).resolve().as_posix(),
            gem_object['external_subdirectories'])) if 'external_subdirectories' in gem_object else []

        # recurse into gem subdirectories
        for external_subdirectory in external_subdirectories:
            external_subdirectory = pathlib.Path(external_subdirectory)
            gem_json_path = external_subdirectory / 'gem.json'
            if gem_json_path.is_file():
                external_subdirectories.extend(get_gem_external_subdirectories(external_subdirectory, visited_gem_paths, gems_json_data_by_path))

    # The gem_path has completely visited, remove it from the visit set
    visited_gem_paths.remove(gem_path)

    return list(dict.fromkeys(external_subdirectories))


def get_gem_templates(gem_path: pathlib.Path) -> list:
    gem_object = get_gem_json_data(gem_path=gem_path)
    if gem_object:
        return list(map(lambda rel_path: (pathlib.Path(gem_path) / rel_path).as_posix(),
                        gem_object['templates'])) if 'templates' in gem_object else []
    return []

def get_engines_json_data_by_path():
    # dictionaries will maintain insertion order which we want
    # because when we have engines with the same name and version
    # we pick the first one found in the 'engines' o3de_manifest field
    engines_json_data = {} 
    engines = get_manifest_engines()
    for engine in engines:
        if isinstance(engine, dict):
            engine_path = pathlib.Path(engine['path']).resolve()
        else:
            engine_path = pathlib.Path(engine).resolve()
        engine_json_data = get_engine_json_data(engine_path=engine_path)
        if not engine_json_data:
            continue
        engines_json_data[engine_path] = engine_json_data
    return engines_json_data

# Combined manifest queries
def get_all_projects() -> list:
    projects_data = get_manifest_projects()
    projects_data.extend(get_engine_projects())
    # Remove duplicates from the list
    return list(dict.fromkeys(projects_data))


def get_all_gems(project_path: pathlib.Path = None) -> list:
    return get_gems_from_external_subdirectories(get_all_external_subdirectories(project_path=project_path, gems_json_data_by_path=dict()))


def add_dependency_gem_names(gem_name:str, gems_json_data_by_name:dict, all_gem_names:set):
    """
    Add gem names for all gem dependencies to the all_gem_names set recursively
    param: gem_name the gem name to add with its dependencies
    param: gems_json_data_by_name a dict of all gem json data to use
    param: all_gem_names the set that all dependency gem names are added to
    """
    gem_json_data = gems_json_data_by_name.get(gem_name, None)
    if gem_json_data:
        dependencies = gem_json_data.get('dependencies',[])
        for dependency_gem_name in dependencies:
            if dependency_gem_name not in all_gem_names:
                all_gem_names.add(dependency_gem_name)
                add_dependency_gem_names(dependency_gem_name, gems_json_data_by_name, all_gem_names)


def remove_non_dependency_gem_json_data(gem_names:list, gems_json_data_by_name:dict) -> None:
    """
    Given a list of gem names and a dict of all gem json data, remove all gem entries that are not
    in the list and not dependencies. 
    param: gem_names the list of gem names
    param: gems_json_data_by_name a dict of all gem json data that will be modified
    """
    gem_names_to_keep = set(gem_names)
    for gem_name in set(gem_names):
        add_dependency_gem_names(gem_name, gems_json_data_by_name, gem_names_to_keep)

    gem_names_to_remove = [gem_name for gem_name in gems_json_data_by_name if gem_name not in gem_names_to_keep]
    for gem_name in gem_names_to_remove:
        del gems_json_data_by_name[gem_name]


def get_gems_json_data_by_path(engine_path:pathlib.Path = None, 
                               project_path: pathlib.Path = None, 
                               include_manifest_gems: bool = False,
                               include_engine_gems: bool = False,
                               external_subdirectories: list = None) -> dict:
    """
    Create a dictionary of gem.json data with gem paths as keys based on the provided list of
    external subdirectories, engine_path or project_path.  Optionally, include gems
    found using the o3de manifest.

    param: engine_path optional engine path
    param: project_path optional project path
    param: include_manifest_gems if True, include gems found using the o3de manifest 
    param: include_engine_gems if True, include gems found using the engine, 
    will use the current engine if no engine_path is provided and none can be deduced from
    the project_path
    param: external_subdirectories optional external_subdirectories to include
    return: a dictionary of gem_path -> gem.json data
    """
    all_gems_json_data = {}

    # we don't use a default list() value in the function params
    # because Python will persist changes to this default list across
    # multiple function calls which is FUN to debug
    external_subdirectories = list() if not external_subdirectories else external_subdirectories

    if include_manifest_gems:
        external_subdirectories.extend(get_manifest_external_subdirectories())

    if project_path:
        external_subdirectories.extend(get_project_external_subdirectories(project_path))
        if not engine_path and include_engine_gems:
            engine_path = get_project_engine_path(project_path=project_path)

    if engine_path or include_engine_gems:
        # this will use the current engine if engine_path is None
        external_subdirectories.extend(get_engine_external_subdirectories(engine_path))

    # Filter out duplicate external_subdirectories before querying if they contain gem.json files
    external_subdirectories = list(dict.fromkeys(external_subdirectories))

    gem_paths = get_gems_from_external_subdirectories(external_subdirectories)
    for gem_path in gem_paths:
        get_gem_external_subdirectories(gem_path, list(), all_gems_json_data)
    
    return all_gems_json_data

def get_gems_json_data_by_name(engine_path:pathlib.Path = None, 
                               project_path: pathlib.Path = None, 
                               include_manifest_gems: bool = False,
                               include_engine_gems: bool = False,
                               external_subdirectories: list = None) -> dict:
                        
    """
    Create a dictionary of gem.json data with gem names as keys based on the provided list of
    external subdirectories, engine_path or project_path.  Optionally, include gems
    found using the o3de manifest.

    It's often more efficient to open all gem.json files instead of 
    looking up each by name, which will load many gem.json files multiple times
    It takes about 150ms to populate this structure with 137 gems, 4696 bytes in total

    param: engine_path optional engine path
    param: project_path optional project path
    param: include_manifest_gems if True, include gems found using the o3de manifest 
    param: include_engine_gems if True, include gems found using the engine, 
    will use the current engine if no engine_path is provided and none can be deduced from
    the project_path
    param: external_subdirectories optional external_subdirectories to include
    return: a dictionary of gem_name -> gem.json data
    """
    all_gems_json_data = get_gems_json_data_by_path(engine_path, 
                                                    project_path, 
                                                    include_manifest_gems, 
                                                    include_engine_gems, 
                                                    external_subdirectories)

    # convert from being keyed on gem_path to gem_name and store the paths
    # resulting dictionary format will look like
    # {
    #     '<gem name>': [
    #         {'gem_name':'<gem name>', 'version':'<version>', 'path':'<path>'}
    #     ],
    # }
    #     e.g.
    # {
    #     'gem1': [
    #         {'gem_name':'gem1', 'version':'1.0.0'},
    #         {'gem_name':'gem1', 'version':'2.0.0'},
    #     ],
    #     'gem2': [
    #         {'gem_name':'gem2', 'version':'1.0.0'},
    #         {'gem_name':'gem2', 'version':'2.0.0'},
    #     ],
    # }
    utils.replace_dict_keys_with_value_key(all_gems_json_data, value_key='gem_name', replaced_key_name='path', place_values_in_list=True)

    return all_gems_json_data


def get_all_external_subdirectories(engine_path:pathlib.Path = None, project_path: pathlib.Path = None, gems_json_data_by_path: dict = None) -> list:
    external_subdirectories_data = get_manifest_external_subdirectories()
    external_subdirectories_data.extend(get_engine_external_subdirectories(engine_path))
    if project_path:
        external_subdirectories_data.extend(get_project_external_subdirectories(project_path))

    # Filter out duplicate external_subdirectories before querying if they contain gem.json files
    external_subdirectories_data = list(dict.fromkeys(external_subdirectories_data))

    gem_paths = get_gems_from_external_subdirectories(external_subdirectories_data)
    for gem_path in gem_paths:
        external_subdirectories_data.extend(get_gem_external_subdirectories(gem_path, list(), gems_json_data_by_path))

    # Remove duplicates from the list
    return list(dict.fromkeys(external_subdirectories_data))


def get_all_templates(project_path: pathlib.Path = None) -> list:
    templates_data = get_manifest_templates()
    templates_data.extend(get_engine_templates())
    if project_path:
        templates_data.extend(get_project_templates(project_path))

    gems_data = get_all_gems(project_path)
    for gem_path in gems_data:
        templates_data.extend(get_gem_templates(gem_path))

    # Remove duplicates from the list
    return list(dict.fromkeys(templates_data))


# Template functions
def get_templates_for_project_creation(project_path: pathlib.Path = None) -> list:
    project_templates = []
    for template_path in get_all_templates(project_path):
        template_path = pathlib.Path(template_path)
        template_json_path = template_path / 'template.json'
        if not validation.valid_o3de_template_json(template_json_path):
            continue
        project_json_path = template_path / 'Template' / 'project.json'
        if validation.valid_o3de_project_json(project_json_path):
            project_templates.append(template_path)

    return project_templates


def get_templates_for_gem_creation(project_path: pathlib.Path = None) -> list:
    gem_templates = []
    for template_path in get_all_templates(project_path):
        template_path = pathlib.Path(template_path)
        template_json_path = template_path / 'template.json'
        if not validation.valid_o3de_template_json(template_json_path):
            continue

        gem_json_path = template_path / 'Template' / 'gem.json'
        if validation.valid_o3de_gem_json(gem_json_path):
            gem_templates.append(template_path)
    return gem_templates


def get_templates_for_generic_creation(project_path: pathlib.Path = None) -> list:
    generic_templates = []
    for template_path in get_all_templates(project_path):
        template_path = pathlib.Path(template_path)
        template_json_path = template_path / 'template.json'
        if not validation.valid_o3de_template_json(template_json_path):
            continue
        gem_json_path = template_path / 'Template' / 'gem.json'
        project_json_path = template_path / 'Template' / 'project.json'
        if not validation.valid_o3de_gem_json(gem_json_path) and\
                not validation.valid_o3de_project_json(project_json_path):
            generic_templates.append(template_path)

    return generic_templates


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
                          project_path: str or pathlib.Path = None,
                          user: bool = False) -> dict or None:
    if not project_name and not project_path:
        logger.error('Must specify either a Project name or Project Path.')
        return None

    if project_name and not project_path:
        project_path = get_registered(project_name=project_name)

    if pathlib.Path(project_path).is_file():
        if user:
            # skip validation because a user project.json is only for overrides and can be empty
            return get_json_data_file(project_path, 'project', validation.always_valid)
        else:
            return get_json_data_file(project_path, 'project', validation.valid_o3de_project_json)
    elif user:
        # create the project user folder if it doesn't exist
        user_project_folder = pathlib.Path(project_path) / 'user'
        user_project_folder.mkdir(parents=True, exist_ok=True)

        user_project_json_path = user_project_folder / 'project.json'

        # return an empty json object if no file exists
        if not user_project_json_path.exists():
            return {}
        else:
            # skip validation because a user project.json is only for overrides and can be empty
            return get_json_data('project', user_project_folder, validation.always_valid) or {}
    else:
        return get_json_data('project', project_path, validation.valid_o3de_project_json)


def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                      project_path: pathlib.Path = None) -> dict or None:
    if not gem_name and not gem_path:
        logger.error('Must specify either a Gem name or Gem Path.')
        return None

    if gem_name and not gem_path:
        gem_path = get_registered(gem_name=gem_name, project_path=project_path)

    # Call get_json_data_file if the path is an existing file as get_json_data appends gem.json
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

    # Call get_json_data_file if the path is an existing file as get_json_data appends template.json
    if pathlib.Path(template_path).is_file():
        return get_json_data_file(template_path, 'template', validation.valid_o3de_template_json)
    else:
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


def get_repo_path(repo_uri: str, cache_folder: str or pathlib.Path = None) -> pathlib.Path:
    repo_manifest = f'{repo_uri}/repo.json'
    cache_file, _ = repo.get_cache_file_uri(repo_manifest)
    return cache_file


def get_most_compatible_gem(gem_name: str, 
                            gem_json_data_by_name: dict or None) -> pathlib.Path or None:
    """
    Optimized version of get_most_compatible_object() for gems when we have already
    opened all the gem.json files
    :param gem_name The gem name with optional version specifier, example: o3de>=1.2.3
    :param gem_json_data_by_name Gem data from get_gems_json_data_by_name()
    """
    gem_name_with_version_specifier = gem_name
    gem_name, version_specifier = utils.get_object_name_and_optional_version_specifier(gem_name)
    if not gem_name in gem_json_data_by_name:
        return None

    matching_paths = deque()
    most_compatible_version = Version('0.0.0')
    for gem_json_data in gem_json_data_by_name.get(gem_name, {}):
        if version_specifier:
            candidate_version = gem_json_data.get('version','0.0.0')
            if compatibility.has_compatible_version([gem_name_with_version_specifier], gem_name, candidate_version):
                if not matching_paths:
                    matching_paths.appendleft(gem_json_data['path'])
                    most_compatible_version = Version(candidate_version)
                elif Version(candidate_version) > most_compatible_version:
                    matching_paths.appendleft(gem_json_data['path'])
                    most_compatible_version = Version(candidate_version)
                else:
                    matching_paths.append(gem_json_data['path'])
        else:
            matching_paths.append(gem_json_data['path'])

    return None if not matching_paths else matching_paths[0]


def get_most_compatible_object(object_name: str, 
                              name_key: str, 
                              objects: list) -> dict or None:
    """
    Looks for the most compatible object based on object_name which may contain a version specifier.
    Example: o3de>=1.2.3

    :param object_name: Name of the object with optional version specifier 
    :param name_key: Object name key inside the object's json file e.g. 'engine_name' 
    :param objects: List of object json data to consider 
    :return the most compatible object json data dict or None
    """
    most_compatible_version = Version('0.0.0')
    object_name, version_specifier = utils.get_object_name_and_optional_version_specifier(object_name)
    most_compatible_object = None

    def update_most_compatible(candidate_version:str, json_data:dict):
        nonlocal most_compatible_object
        nonlocal most_compatible_version

        if not most_compatible_object:
            most_compatible_object = json_data 
            most_compatible_version = Version(candidate_version)
        elif Version(candidate_version) > most_compatible_version:
            most_compatible_object = json_data 
            most_compatible_version = Version(candidate_version)

    for json_data in objects:
        candidate_name = json_data.get(name_key,'')
        if candidate_name != object_name:
            continue

        candidate_version = json_data.get('version','0.0.0')
        if version_specifier:
            if compatibility.has_compatible_version([object_name + version_specifier], candidate_name, candidate_version):
                update_most_compatible(candidate_version, json_data)
        else:
            update_most_compatible(candidate_version, json_data)

    return most_compatible_object

def get_most_compatible_object_path(object_name: str, 
                              object_typename: str, 
                              object_validator: callable, 
                              name_key: str, 
                              objects: list) -> pathlib.Path or None:
    """
    Looks for the most compatible object based on object_name which may contain a version specifier.
    Example: o3de>=1.2.3

    :param object_name: Name of the object with optional version specifier 
    :param object_typename: Type of object e.g. 'engine','project' or 'gem' 
    :param object_validator: Validator to use for json file 
    :param name_key: Object name key inside the object's json file e.g. 'engine_name' 
    :param objects: List of paths to search
    :return the most compatible object path or None
    """
    matching_paths = deque()
    most_compatible_version = Version('0.0.0')
    object_name, version_specifier = utils.get_object_name_and_optional_version_specifier(object_name)
    for object in objects:
        if isinstance(object, dict):
            path = pathlib.Path(object['path']).resolve()
        else:
            path = pathlib.Path(object).resolve()

        json_data = get_json_data(object_typename, path, object_validator)
        if json_data:
            candidate_name = json_data.get(name_key,'')
            if version_specifier:
                candidate_version = json_data.get('version','0.0.0')
                if compatibility.has_compatible_version([object_name + version_specifier], candidate_name, candidate_version):
                    if not matching_paths:
                        matching_paths.appendleft(path)
                        most_compatible_version = Version(candidate_version)
                    elif Version(candidate_version) > most_compatible_version:
                        matching_paths.appendleft(path)
                        most_compatible_version = Version(candidate_version)
                    else:
                        matching_paths.append(path)
            elif candidate_name == object_name:
                matching_paths.append(path)
    if matching_paths:
        best_candidate_path = matching_paths[0]
        if len(matching_paths) > 1:
            matches = "\n".join(map(str,matching_paths))
            logger.warning(f"Multiple matches found for: '{object_name}'\n{matches}\nMost compatible match: '{best_candidate_path}'")
        return best_candidate_path

    return None

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

    if isinstance(engine_name, str):
        return get_most_compatible_object_path(engine_name, 'engine', validation.valid_o3de_engine_json, 'engine_name', get_manifest_engines())

    elif isinstance(project_name, str):
        return get_most_compatible_object_path(project_name, 'project', validation.valid_o3de_project_json, 'project_name', get_all_projects())

    elif isinstance(gem_name, str):
        gems = []
        if project_path:
            gems = get_all_gems(project_path)
        else:
            # If project_path is not supplied
            registered_project_paths = get_all_projects()
            if not registered_project_paths:
                # query all gems from this engine if no projects exist
                gems = get_all_gems()
            else:
                # query all registered projects
                for registered_project_path in registered_project_paths:
                    gems.extend(get_all_gems(registered_project_path))
                gems = list(dict.fromkeys(gems))
        return get_most_compatible_object_path(gem_name, 'gem', validation.valid_o3de_gem_json, 'gem_name', gems)

    elif isinstance(template_name, str):
        templates = []
        if project_path:
            templates = get_all_templates(project_path)
        else:
            # If project_path is not supplied
            registered_project_paths = get_all_projects()
            if not registered_project_paths:
                # if no projects exist, query all templates from this engine and gems
                templates = get_all_templates()
            else:
                # query all registered projects
                for registered_project_path in registered_project_paths:
                    templates.extend(get_all_templates(registered_project_path))
                templates = list(dict.fromkeys(templates))

        for template_path in templates:
            template_path = pathlib.Path(template_path).resolve()
            template_json = template_path / 'template.json'
            if not pathlib.Path(template_json).is_file():
                logger.warning(f'{template_json} does not exist')
            else:
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
        restricted = get_manifest_restricted()
        for restricted_path in restricted:
            restricted_path = pathlib.Path(restricted_path).resolve()
            restricted_json = restricted_path / 'restricted.json'
            if not pathlib.Path(restricted_json).is_file():
                logger.warning(f'{restricted_json} does not exist')
            else:
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
            if 'default_engines_folder' in json_data:
                default_engines_folder = pathlib.Path(json_data['default_engines_folder'])
            else:
                default_engines_folder = pathlib.Path(
                    get_default_o3de_manifest_json_data().get('default_engines_folder', None))
            return default_engines_folder.resolve() if default_engines_folder else None
        elif default_folder == 'projects':
            if 'default_projects_folder' in json_data:
                default_projects_folder = pathlib.Path(json_data['default_projects_folder'])
            else:
                default_projects_folder = pathlib.Path(
                    get_default_o3de_manifest_json_data().get('default_projects_folder', None))
            return default_projects_folder.resolve() if default_projects_folder else None
        elif default_folder == 'gems':
            if 'default_gems_folder' in json_data:
                default_gems_folder = pathlib.Path(json_data['default_gems_folder'])
            else:
                default_gems_folder = pathlib.Path(
                    get_default_o3de_manifest_json_data().get('default_gems_folder', None))
            return default_gems_folder.resolve() if default_gems_folder else None
        elif default_folder == 'templates':
            if 'default_templates_folder' in json_data:
                default_templates_folder = pathlib.Path(json_data['default_templates_folder'])
            else:
                default_templates_folder = pathlib.Path(
                    get_default_o3de_manifest_json_data().get('default_templates_folder', None))
            return default_templates_folder.resolve() if default_templates_folder else None
        elif default_folder == 'restricted':
            if 'default_restricted_folder' in json_data:
                default_restricted_folder = pathlib.Path(json_data['default_restricted_folder'])
            else:
                default_restricted_folder = pathlib.Path(
                    get_default_o3de_manifest_json_data().get('default_restricted_folder', None))
            return default_restricted_folder.resolve() if default_restricted_folder else None

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
