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
This file validating o3de object json files
"""
import json
import pathlib

def valid_o3de_json_dict(json_data: dict, key: str) -> bool:
    return key in json_data


def valid_o3de_repo_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            test = json_data['repo_name']
            test = json_data['origin']
        except (json.JSONDecodeError, KeyError) as e:
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
        except (json.JSONDecodeError, KeyError) as e:
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
        except (json.JSONDecodeError, KeyError) as e:
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
        except (json.JSONDecodeError, KeyError) as e:
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
        except (json.JSONDecodeError, KeyError) as e:
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
        except (json.JSONDecodeError, KeyError) as e:
            return False
    return True
