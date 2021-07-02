#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import logging
import pytest
import pathlib
from unittest.mock import patch

from o3de import register

string_manifest_data = '{}'

@pytest.mark.parametrize(
    "engine_path, engine_name, force, expected_result", [
        pytest.param(pathlib.PurePath('D:/o3de/o3de'), "o3de", False, 0),
        # Same engine_name and path should result in valid registration
        pytest.param(pathlib.PurePath('D:/o3de/o3de'), "o3de", False, 0),
        # Same engine_name and but different path should fail
        pytest.param(pathlib.PurePath('D:/o3de/engine-path'), "o3de", False, 1),
        # New engine_name should result in valid registration
        pytest.param(pathlib.PurePath('D:/o3de/engine-path'), "o3de-other", False, 0),
        # Same engine_name and but different path with --force should result in valid registration
        pytest.param(pathlib.PurePath('F:/Open3DEngine'), "o3de", True, 0),
    ]
)
def test_register_engine_path(engine_path, engine_name, force, expected_result):
    parser = argparse.ArgumentParser()

    # Register the registration script subparsers with the current argument parser
    register.add_parser_args(parser)
    arg_list = ['--engine-path', str(engine_path)]
    if force:
        arg_list += ['--force']
    args = parser.parse_args(arg_list)

    def load_manifest_from_string() -> dict:
        try:
            manifest_json = json.loads(string_manifest_data)
        except json.JSONDecodeError as err:
            logging.error("Error decoding Json from Manifest file")
        else:
            return manifest_json
    def save_manifest_to_string(manifest_json: dict) -> None:
        global string_manifest_data
        string_manifest_data = json.dumps(manifest_json)

    engine_json_data = {'engine_name': engine_name}
    with patch('o3de.manifest.load_o3de_manifest', side_effect=load_manifest_from_string) as load_manifest_mock, \
         patch('o3de.manifest.save_o3de_manifest', side_effect=save_manifest_to_string) as save_manifest_mock, \
         patch('o3de.manifest.get_engine_json_data', return_value=engine_json_data) as engine_paths_mock, \
         patch('o3de.validation.valid_o3de_engine_json', return_value=True) as valid_engine_mock, \
         patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_mock:
        result = register._run_register(args)
        assert result == expected_result


@pytest.fixture(scope='class')
def init_manifest_data(request):
    class ManifestData:
        def __init__(self):
            self.json_string = json.dumps({'default_engines_folder': '',
                'default_projects_folder': '', 'default_gems_folder': '',
                'default_templates_folder': '', 'default_restricted_folder': ''})

    request.cls.manifest_data = ManifestData()


@pytest.mark.usefixtures('init_manifest_data')
class TestRegisterThisEngine:
    @pytest.mark.parametrize(
        "engine_path, engine_name, force, expected_result", [
            pytest.param(pathlib.PurePath('D:/o3de/o3de'), "o3de", False, 0),
            pytest.param(pathlib.PurePath('F:/Open3DEngine'), "o3de", False, 1),
            pytest.param(pathlib.PurePath('F:/Open3DEngine'), "o3de", True, 0)
        ]
    )
    def test_register_this_engine(self, engine_path, engine_name, force, expected_result):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        register.add_parser_args(parser)
        arg_list = ['--this-engine']
        if force:
            arg_list += ['--force']
        args = parser.parse_args(arg_list)

        def load_manifest_from_string() -> dict:
            try:
                manifest_json = json.loads(self.manifest_data.json_string)
            except json.JSONDecodeError as err:
                logging.error("Error decoding Json from Manifest file")
            else:
                return manifest_json
        def save_manifest_to_string(manifest_json: dict) -> None:
            self.manifest_data.json_string = json.dumps(manifest_json)

        engine_json_data = {'engine_name': engine_name}

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_manifest_from_string) as load_manifest_mock, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_manifest_to_string) as save_manifest_mock, \
                patch('o3de.manifest.get_engine_json_data', return_value=engine_json_data) as engine_paths_mock, \
                patch('o3de.manifest.get_this_engine_path', return_value=engine_path) as engine_paths_mock, \
                patch('o3de.validation.valid_o3de_engine_json', return_value=True) as valid_engine_mock, \
                patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_mock:
            result = register._run_register(args)
            assert result == expected_result

