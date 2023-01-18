#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains compatibility functions
"""
from packaging.version import Version, InvalidVersion
from packaging.specifiers import SpecifierSet
import pathlib
import logging
from o3de import manifest, utils, cmake

logger = logging.getLogger('o3de.compatibility')
logging.basicConfig(format=utils.LOG_FORMAT)

def get_project_engine_incompatible_objects(project_path:pathlib.Path, engine_path:pathlib.Path = None) -> set:
    """
    Returns any incompatible objects for this project and engine.
    :param project_path: Path to the project
    :param engine_path: Optional path to the engine. If not specified, the current engine path is used
    """
    # use the specified engine path NOT necessarily engine the project is registered to
    engine_path = engine_path or manifest.get_this_engine_path()
    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error(f'Failed to load engine.json data needed for compatibility check from {engine_path}. '
            'Please verify the path is correct, the file exists and is formatted correctly.')
        return set('engine.json (missing)')

    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Failed to load project.json data needed for compatibility check from {project_path}. '
            'Please verify the path is correct, the file exists and is formatted correctly.')
        return set('project.json (missing)')

    incompatible_objects = get_incompatible_objects_for_engine(project_json_data, engine_json_data)

    # verify project -> gem -> engine compatibility
    active_gem_names = project_json_data.get('gem_names',[])
    enabled_gems_file = cmake.get_enabled_gem_cmake_file(project_path=project_path)
    active_gem_names.extend(cmake.get_enabled_gems(enabled_gems_file))
    active_gem_names = utils.get_gem_names_set(active_gem_names)

    # it's much more efficient to get all gem data once than to query them by name one by one
    all_gems_json_data = manifest.get_gems_json_data_by_name(engine_path, project_path, include_manifest_gems=True)

    for gem_name in active_gem_names:
        if gem_name not in all_gems_json_data:
            logger.warning(f'Skipping compatibility check for {gem_name} because no gem.json data was found for it. '
                'Please verify this gem is registered.')
            continue
        incompatible_objects.update(get_gem_engine_incompatible_objects(all_gems_json_data[gem_name], engine_json_data, all_gems_json_data))

    return incompatible_objects


def get_incompatible_gem_dependencies(gem_json_data:dict, all_gems_json_data:dict) -> set:
    """
    Returns incompatible gem dependencies
    :param gem_json_data: gem json data dictionary
    :param all_gems_json_data: json data of all gems to use for compatibility checks
    """
    # try to avoid gem compatibility checks which incur the cost of 
    # opening many gem.json files to get version information
    gem_dependencies = gem_json_data.get('dependencies')
    if not gem_dependencies:
        return set()

    return get_incompatible_gem_version_specifiers(gem_dependencies, all_gems_json_data)


def get_gem_project_incompatible_objects(gem_json_data:dict, project_path:pathlib.Path, all_gems_json_data:dict = None) -> set:
    """
    Returns any incompatible objects for this gem and project.
    :param gem_json_data: gem json data dictionary
    :param project_path: path to the project
    :param all_gems_json_data: optional dictionary containing data for all gems to use in compatibility checks. 
    If not provided, uses all gems from the manifest, engine and project.
    """
    # early out if this project has no assigned engine
    engine_path = manifest.get_project_engine_path(project_path=project_path)
    if not engine_path:
        logger.warning(f'Project at path {project_path} is not registered to an engine and compatibility cannot be checked.')
        return set()

    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Failed to load project.json data from {project_path} needed for checking compatibility')
        return set(f'project.json (missing)') 

    # in the future we should check if the gem and project depend on conflicting engines and/or apis
    # we need some way of doing version specifier overlap checks

    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error(f'Failed to load engine.json data based on the engine field in project.json or detect the engine from the current folder')
        return set(f'engine.json (missing)') 

    if not all_gems_json_data:
        all_gems_json_data = manifest.get_gems_json_data_by_name(engine_path, project_path, include_manifest_gems=True)

    # compatibility will be based on the engine the project uses and the gems visible to
    # the engine and project
    return get_gem_engine_incompatible_objects(gem_json_data, engine_json_data, all_gems_json_data)


def get_gem_engine_incompatible_objects(gem_json_data:dict, engine_json_data:dict, all_gems_json_data:dict) -> set:
    """
    Returns any incompatible objects for this gem and engine.
    :param gem_json_data: gem json data dictionary
    :param engine_json_data: engine json data dictionary
    :param all_gems_json_data: json data of all gems to use for compatibility checks
    """
    incompatible_objects = get_incompatible_objects_for_engine(gem_json_data, engine_json_data)
    return incompatible_objects.union(get_incompatible_gem_dependencies(gem_json_data, all_gems_json_data))


def get_incompatible_objects_for_engine(object_json_data:dict, engine_json_data:dict) -> set:
    """
    Returns any incompatible objects for this object and engine.
    If a compatible_engine entry only has an engine name, it is assumed compatible with every engine version with that name.
    If an engine_api_version entry only has an api name, it is assumed compatible with every engine api version with that name.
    :param object_json_data: the json data for the object 
    :param engine_json_data: the json data for the engine
    """
    compatible_engines = object_json_data.get('compatible_engines')
    engine_api_version_specifiers = object_json_data.get('engine_api_dependencies')

    # early out if there are no restrictions
    if not compatible_engines and not engine_api_version_specifiers:
        return set() 

    engine_name = engine_json_data['engine_name']
    engine_version = engine_json_data.get('version')

    incompatible_objects = set()
    if compatible_engines:
        if engine_version and has_compatible_version(compatible_engines, engine_name, engine_version):
            return set() 
        elif not engine_version and has_compatible_name(compatible_engines, engine_name):
            # assume an engine with no version is compatible
            return set()

        # object is not known to be compatible with this engine
        incompatible_objects.add(f"{engine_name} {engine_version} does not match any version specifiers in the list of compatible engines: {compatible_engines}")

    if engine_api_version_specifiers:
        engine_api_versions = engine_json_data.get('api_versions')
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


def get_incompatible_gem_version_specifiers(gem_version_specifier_list:list, all_gems_json_data:dict) -> set:
    """
    Returns a set of gem version specifiers that are not compatible with the gem's provided
    If a gem_version_specifier_list entry only has a gem name, it is assumed compatible with every gem version with that name.
    :param gem_version_specifier_list: a list of gem names and (optional)version specifiers
    :param all_gems_json_data: json data of all gems to use for compatibility checks
    """
    if not gem_version_specifier_list:
        return set()

    if not all_gems_json_data:
        # no gems are available
        return set(gem_version_specifier_list)

    incompatible_gem_version_specifiers = set()
    for gem_version_specifier in gem_version_specifier_list:
        gem_name, version_specifier = utils.get_object_name_and_optional_version_specifier(gem_version_specifier)
        if not gem_name in all_gems_json_data:
            incompatible_gem_version_specifiers.add(f"{gem_version_specifier} (missing dependency)")
            continue

        if not version_specifier:
            # when no version specifier is provided we assume compatibility with any version
            continue
        
        gem_version = all_gems_json_data[gem_name].get('version')
        if gem_version and not has_compatible_version([gem_version_specifier], gem_name, gem_version):
            incompatible_gem_version_specifiers.add(f"{gem_version_specifier} (different version found ${gem_version})")
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
        logger.warning(f'Failed to parse version specifier {object_version}, please verify it is PEP 440 compatible.')
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
