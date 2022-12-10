#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains version utilities
"""
from packaging.version import Version, InvalidVersion
from packaging.specifiers import SpecifierSet
import pathlib
import logging
from o3de import manifest, utils, cmake

logger = logging.getLogger('o3de.compatibility')
logging.basicConfig(format=utils.LOG_FORMAT)

def get_project_engine_incompatible_objects(project_path:pathlib.Path, project_json_data:dict, engine_json_data:dict) -> set:
    """
    Returns any incompatible objects for this project and engine.
    :param project_path: path to the project
    :param project_json_data: project json data dictionary
    :param engine_json_data: engine json data dictionary
    """
    incompatible_objects = get_incompatible_objects_for_engine(project_json_data, engine_json_data)

    # verify project -> gem -> engine compatibility
    active_gem_names = project_json_data.get('gem_names',[])
    enabled_gems_file = cmake.get_enabled_gem_cmake_file(project_path=project_path)
    active_gem_names.extend(cmake.get_enabled_gems(enabled_gems_file))
    active_gem_names = utils.get_gem_names_set(active_gem_names)

    # it's much more efficient to get all gem data once than to query them by name one by one
    gem_paths = manifest.get_all_gems(project_path)
    gem_name_to_json_data = {}
    for gem_path in gem_paths:
        gem_path = pathlib.Path(gem_path)
        gem_json_data = manifest.get_gem_json_data(gem_path=gem_path)
        # only store the json data if it's a gem we're looking for and has compatibility data
        candidate_gem_name = gem_json_data['gem_name']
        if candidate_gem_name in active_gem_names and\
            (gem_json_data.get('compatible_engines') or gem_json_data.get('engine_api_dependencies')):
            gem_name_to_json_data[candidate_gem_name] = gem_json_data

    for gem_name in active_gem_names:
        if gem_name not in gem_name_to_json_data:
            continue
        gem_incompatible_objects = get_gem_engine_incompatible_objects(gem_name_to_json_data[gem_name], engine_json_data, gem_paths=gem_paths)
        incompatible_objects = incompatible_objects.union(gem_incompatible_objects)

    return incompatible_objects


def get_incompatible_gem_dependencies(gem_json_data:dict, project_path:pathlib.Path = None, gem_paths:list = [], check:bool = False) -> set:
    """
    Returns True if the gem is compatible with the gems registered with the engine and optional project.
    :param gem_json_data: gem json data dictionary
    :param project_path: path to the project (optional)
    :param gem_paths: paths to all gems to use for dependency checks (optional)
    :param check: if True always return True and ouput log info about the result
    """
    # try to avoid gem compatibility checks which incur the cost of 
    # opening many gem.json files to get version information
    gem_dependencies = gem_json_data.get('dependencies','')
    if not gem_dependencies:
        return set()

    if not gem_paths:
        gem_paths = manifest.get_engine_gems()
        if project_path:
            gem_paths.extend(manifest.get_project_gems(project_path))
        # Convert each path to pathlib.Path object and filter out duplicates using dict.fromkeys
        gem_paths = list(dict.fromkeys(map(lambda gem_path_string: pathlib.Path(gem_path_string), gem_paths)))

    return get_incompatible_gem_version_specifiers(project_path, gem_dependencies, gem_paths)


def get_gem_project_incompatible_objects(gem_json_data:dict, project_path:pathlib.Path, gem_paths:list = [], check:bool = False) -> bool:
    """
    Returns True if the gem is compatible with the indicated project.
    :param gem_json_data: gem json data dictionary
    :param project_path: path to the project
    :param gem_paths: paths to all gems to use for dependency checks (optional)
    :param check: if True always return True and ouput log info about the result
    """
    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Failed to load project.json data from {project_path} needed for checking compatibility')
        return set(f'project.json') 

    engine_path = manifest.get_project_engine_path(project_path=project_path)
    if not engine_path:
        # the project is not registered with an engine
        # in the future we should check if the gem and project depend on conflicting
        # engines and/or apis
        return get_incompatible_gem_dependencies(gem_json_data, gem_paths=gem_paths, check=check)

    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error(f'Failed to load engine.json data based on the engine field in project.json or detect the engine from the current folder')
        return set(f'engine.json') 

    # compatibility will be based on the engine the project uses and the gems visible to
    # the engine and project
    return get_gem_engine_incompatible_objects(gem_json_data, engine_json_data, gem_paths=gem_paths)


def get_gem_engine_incompatible_objects(gem_json_data:dict, engine_json_data:dict, gem_paths:list = None, check:bool = False) -> bool:
    """
    Returns True if the gem is compatible with the indicated engine.
    :param gem_json_data: gem json data dictionary
    :param engine_json_data: engine json data dictionary
    :param gem_paths: paths to all gems to use for dependency checks (optional)
    :param check: if True always return True and ouput log info about the result
    """
    incompatible_objects = get_incompatible_objects_for_engine(gem_json_data, engine_json_data)

    if not gem_paths:
        gem_paths = manifest.get_engine_gems()

    return incompatible_objects.union(get_incompatible_gem_dependencies(gem_json_data, gem_paths=gem_paths, check=check))

def get_incompatible_objects_for_engine(object_json_data:dict, engine_json_data:dict) -> set:
    """
    Returns True if the engine is compatible with the provided object's compatible_engines and engine_api_versions information.
    If a compatible_engine entry only has an engine name, it is assumed compatible with every engine version with that name.
    If an engine_api_version entry only has an api name, it is assumed compatible with every engine api version with that name.
    :param object_json_data: the json data for the object 
    :param engine_json_data: the json data for the engine
    """
    compatible_engines = object_json_data.get('compatible_engines','')
    engine_api_version_specifiers = object_json_data.get('engine_api_dependencies','')

    # early out if there are no restrictions
    if not compatible_engines and not engine_api_version_specifiers:
        return set() 

    engine_name = engine_json_data['engine_name']
    engine_version = engine_json_data.get('version','')

    incompatible_objects = set()
    if compatible_engines:
        if engine_version and has_compatible_version(compatible_engines, engine_name, engine_version):
            return set() 
        elif not engine_version and has_compatible_name(compatible_engines, engine_name):
            # assume an engine with no version is compatible
            return set()

        # object is not known compatible with this engine
        incompatible_objects.add(f"{engine_name} {engine_version} does not match any version specifiers in the list of compatible engines: {compatible_engines}")

    if engine_api_version_specifiers:
        engine_api_versions = engine_json_data.get('api_versions','')
        if not engine_api_versions:
            # assume not compatible if no engine api version information is available
            return incompatible_objects.union(set(engine_api_version_specifiers))

        incompatible_apis = set()
        for api_version_specifier in engine_api_version_specifiers:
            api_name, unused_version_specifiers = utils.get_object_name_and_optional_version_specifier(api_version_specifier)
            if not has_compatible_version([api_version_specifier], api_name, engine_api_versions.get(api_name,'')):
                incompatible_apis.add(api_version_specifier)
        
        # if an object is compatible with all APIs then it's compatible even
        # if that engine is not listed in the `compatible_engines` field
        if not incompatible_apis:
            return set()
        else:
            incompatible_objects.update(incompatible_apis)

    return incompatible_objects

def get_incompatible_gem_version_specifiers(project_path:pathlib.Path, gem_version_specifier_list:list, gem_paths:list) -> set:
    """
    Returns a list of gem version specifiers that are not compatible with the gem's provided
    If a gem_version_specifier_list entry only has a gem name, it is assumed compatible with every gem version with that name.
    :param project_path: the path to the project
    :param gem_version_specifier_list: a list of gem names and (optional)version specifiers
    :param gem_paths: a list of gem paths
    """
    if not gem_version_specifier_list:
        return set()

    if not gem_paths:
        # no gems are available
        return set(gem_version_specifier_list)

    # create a cache of all gem versions so we aren't opening files multiple times
    project_gem_versions = {} 
    for gem_path in gem_paths:
        json_data = manifest.get_gem_json_data(gem_path=gem_path, project_path=project_path)
        if json_data:
            project_gem_versions[json_data['gem_name']] = json_data.get('gem_version','')

    incompatible_gem_version_specifiers = set()
    for gem_version_specifier in gem_version_specifier_list:
        gem_name, version_specifier = utils.get_object_name_and_optional_version_specifier(gem_version_specifier)
        if not gem_name in project_gem_versions:
            incompatible_gem_version_specifiers.add(gem_version_specifier)
            continue

        if not version_specifier:
            # when no version specifier is provided we assume compatibility with any version
            continue
        
        gem_version = project_gem_versions[gem_name]
        if gem_version and not has_compatible_version([gem_version_specifier], gem_name, gem_version):
            incompatible_gem_version_specifiers.add(gem_version_specifier)
            continue

    return incompatible_gem_version_specifiers

def has_compatible_name(name_and_version_specifier_list:list, object_name:str) -> bool:
    """
    Returns True if the object_name matches an entry in the name_and_version_specifier_list
    :param name_and_version_specifier_list: a list of names and (optional)version specifiers
    :param object_name: the object name
    """
    for name_and_version_specifier in name_and_version_specifier_list:
        # only accept a name without a version specifier
        if object_name == name_and_version_specifier:
            return True

    return False 

def has_compatible_version(name_and_version_specifier_list:list, object_name:str, object_version:str) -> bool:
    """
    returns True if the object_name matches an entry in the name_and_version_specifier_list that has no version,
    or if the object_name matches and the object_version is compatible with the version specifier.
    :param name_and_version_specifier_list: a list of names and (optional)version specifiers
    :param object_name: the object name
    :param object_version: the object version
    """
    try:
        version = Version(object_version)
    except InvalidVersion as e:
        return False

    for name_and_version_specifier in name_and_version_specifier_list:
        # it's possible we received a name without a version specifier
        if object_name == name_and_version_specifier:
            return True

        try:
            name, version_specifier = utils.get_object_name_and_version_specifier(name_and_version_specifier)
            if name == object_name and version in SpecifierSet(version_specifier):
                return True
        except (utils.InvalidObjectNameException, utils.InvalidVersionSpecifierException):
            # skip invalid specifiers
            pass

    return False 
