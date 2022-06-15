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


def has_compatible_version(name_and_version_specifier_list:list, object_name:str, object_version:str) -> str or None:
    try:
        version = Version(object_version)
    except InvalidVersion as e:
        return False

    for name_and_version_specifier in name_and_version_specifier_list:
        try:
            name, version_specifier = get_object_name_and_version_specifier(name_and_version_specifier)
            if name == object_name and version in SpecifierSet(version_specifier):
                return True
        except Exception as e:
            # skip invalid specifiers
            pass

    return False 

def get_object_name_and_version_specifier(input:str) -> tuple[str, str] or None:
    # accepts input in the form <name><version specifier(s)>, for example:
    # o3de>=1.0.0.0
    # o3de-sdk==2205.01,~=2201.10

    regex_str = r"(?P<object_name>(.*?))(?P<version_specifier>((~=|==|!=|<=|>=|<|>|===)(\s*\S+)+))"
    regex = re.compile(r"^\s*" + regex_str + r"\s*$", re.VERBOSE | re.IGNORECASE)
    match = regex.search(input)

    if not match:
        raise Exception(f"Invalid name and/or version specifier {input}, expected <name><version specifiers> e.g. o3de==1.0.0.0")

    if not match.group("object_name"):
        raise Exception(f"Invalid or missing name {input}, expected <name><version specifiers> e.g. o3de==1.0.0.0")

    # SpecifierSet with raise an exception if invalid
    if not SpecifierSet(match.group("version_specifier")):
        return None
    
    return match.group("object_name"), match.group("version_specifier")