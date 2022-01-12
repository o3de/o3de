#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file validating o3de object json files
"""
import json
import pathlib
import uuid


def valid_o3de_json_dict(json_data: dict, key: str) -> bool:
    return key in json_data


def valid_o3de_repo_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            _ = json_data['repo_name']
            _ = json_data['origin']
        except (json.JSONDecodeError, KeyError):
            return False
    return True


def valid_o3de_engine_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            _ = json_data['engine_name']
        except (json.JSONDecodeError, KeyError):
            return False
    return True


def valid_o3de_project_json(file_name: str or pathlib.Path, generate_uuid: bool = True) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            _ = json_data['project_name']

            if not generate_uuid:
                _ = json_data['project_id']
            else:
                test = json_data.get('project_id', 'No ID')
                generate_new_id = test == 'No ID'

        except (json.JSONDecodeError, KeyError):
            return False

    # Generate a random uuid for the project json if it is missing instead of failing if generate_uuid is true
    if generate_uuid and generate_new_id:
        with file_name.open('w') as f:
            new_uuid = '{' + str(uuid.uuid4()) + '}'
            json_data.update({'project_id': new_uuid})
            f.write(json.dumps(json_data, indent=4) + '\n')
    return True


def valid_o3de_gem_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            _ = json_data['gem_name']
        except (json.JSONDecodeError, KeyError):
            return False
    return True


def valid_o3de_template_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            _ = json_data['template_name']
        except (json.JSONDecodeError, KeyError):
            return False
    return True


def valid_o3de_restricted_json(file_name: str or pathlib.Path) -> bool:
    file_name = pathlib.Path(file_name).resolve()
    if not file_name.is_file():
        return False

    with file_name.open('r') as f:
        try:
            json_data = json.load(f)
            _ = json_data['restricted_name']
        except (json.JSONDecodeError, KeyError):
            return False
    return True
