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
import re
import pathlib
from o3de import manifest, utils
from enum import IntFlag

# compatibility ranges from NotCompatible to the most compatible option EngineAPIVersions
class Compatiblity(IntFlag):
    NONE    = 0
    NAME    = 1 >> 0
    VERSION = 1 >> 1
    API     = 1 >> 2
    ALL = NAME | VERSION | API

    def __bool__(self) -> bool:
        return self.value != Compatiblity.NONE


def engine_is_compatible(engine_name:str, engine_version:str, compatible_engines:list, engine_api_version_specifiers:list) -> bool:
    # early out if there are no restrictions
    if not compatible_engines and not engine_api_version_specifiers:
        return True

    if engine_name:
        engine_json_data = manifest.get_engine_json_data(engine_name=engine_name)
    else:
        engine_json_data = manifest.get_engine_json_data(engine_path=manifest.get_this_engine_path())

    if not engine_json_data:
        return False

    if not engine_name:
        # fallback to use the information from the current engine
        engine_name = engine_json_data.get('engine_name','')
        engine_version = engine_json_data.get('engine_version','')

    if compatible_engines:
        if engine_version and has_compatible_version(compatible_engines, engine_name, engine_version):
            return True
        elif not engine_version and has_compatible_name(compatible_engines, engine_name):
            # assume an engine with no version is compatible
            return True

    engine_api_versions = engine_json_data.get('engine_api_versions','')
    if engine_api_version_specifiers and not engine_api_versions:
        # assume not compatible if no engine api version information is available
        return False

    for api_version_specifier in engine_api_version_specifiers:
        api_name, unused_version_specifiers = get_object_name_and_optional_version_specifier(api_version_specifier)
        if has_compatible_version([api_version_specifier], api_name, engine_api_versions.get(api_name,'')):
            return True

    return False

def project_engine_is_compatible(project_path:pathlib.Path, compatible_engines:list, engine_api_versions:list) -> bool:
    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        return False
    
    engine_name = project_json_data.get('engine','')
    engine_version = project_json_data.get('engine_version','')
    return engine_is_compatible(engine_name, engine_version, compatible_engines, engine_api_versions)

def get_incompatible_gem_version_specifiers(project_path:pathlib.Path, gem_version_specifier_list:list, gem_paths:list) -> bool:
    if not gem_version_specifier_list:
        return []

    if not gem_paths:
        # no gems are available
        return gem_version_specifier_list

    # create a cache of all gem versions so we aren't opening files multiple times
    project_gem_versions = {} 
    for gem_path in gem_paths:
        json_data = manifest.get_gem_json_data(gem_path=gem_path, project_path=project_path)
        project_gem_versions[json_data['gem_name']] = json_data.get('gem_version','')

    incompatible_gem_version_specifiers = []
    for gem_version_specifier in gem_version_specifier_list:
        gem_name, version_specifier = get_object_name_and_optional_version_specifier(gem_version_specifier)
        if not gem_name in project_gem_versions:
            incompatible_gem_version_specifiers.append(gem_version_specifier)
            continue

        if not version_specifier:
            # when no version specifier is provided we assume compatibility with any version
            continue
        
        gem_version = project_gem_versions[gem_name]
        if gem_version and not has_compatible_version([gem_version_specifier], gem_name, gem_version):
            incompatible_gem_version_specifiers.append(gem_version_specifier)
            continue

    return incompatible_gem_version_specifiers

def has_compatible_name(name_and_version_specifier_list:list, object_name:str) -> bool:
    for name_and_version_specifier in name_and_version_specifier_list:
        # only accept a name without a version specifier
        if object_name == name_and_version_specifier:
            return True

    return False 

def has_compatible_version(name_and_version_specifier_list:list, object_name:str, object_version:str) -> bool:
    try:
        version = Version(object_version)
    except InvalidVersion as e:
        return False

    for name_and_version_specifier in name_and_version_specifier_list:
        # it's possible we received a name without a version specifier
        if object_name == name_and_version_specifier:
            return True

        try:
            name, version_specifier = get_object_name_and_version_specifier(name_and_version_specifier)
            if name == object_name and version in SpecifierSet(version_specifier):
                return True
        except Exception as e:
            # skip invalid specifiers
            pass

    return False 

def get_object_name_and_optional_version_specifier(input:str):
    try:
        return get_object_name_and_version_specifier(input)
    except Exception as e:
        return input, None

def get_object_name_and_version_specifier(input:str):
    # accepts input in the form <name><version specifier(s)>, for example:
    # o3de>=1.0.0
    # o3de-sdk==1.2.3,~=2.3.4

    regex_str = r"(?P<object_name>(.*?))(?P<version_specifier>((~=|==|!=|<=|>=|<|>|===)(\s*\S+)+))"
    regex = re.compile(r"^\s*" + regex_str + r"\s*$", re.VERBOSE | re.IGNORECASE)
    match = regex.search(input)

    if not match:
        raise Exception(f"Invalid name and/or version specifier {input}, expected <name><version specifiers> e.g. o3de==1.2.3")

    if not match.group("object_name"):
        raise Exception(f"Invalid or missing name {input}, expected <name><version specifiers> e.g. o3de==1.2.3")

    # SpecifierSet with raise an exception if invalid
    if not SpecifierSet(match.group("version_specifier")):
        return None
    
    return match.group("object_name"), match.group("version_specifier")
