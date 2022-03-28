#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
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


TEST_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
    "display_name": "TestGem",
    "license": "What license TestGem uses goes here: i.e. https://opensource.org/licenses/MIT",
    "origin": "The primary repo for TestGem goes here: i.e. http://www.mydomain.com",
    "type": "Code",
    "summary": "A short description of TestGem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "TestGem"
    ],
    "icon_path": "preview.png",
    "requirements": ""
}
'''

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "engine": "o3de",
    "external_subdirectories": []
}
'''

TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "external_subdirectories": [],
    "projects": [],
    "templates": []
}
'''

TEST_O3DE_MANIFEST_JSON_PAYLOAD = '''
{
    "o3de_manifest_name": "testuser",
    "origin": "C:/Users/testuser/.o3de",
    "default_engines_folder": "C:/Users/testuser/.o3de/Engines",
    "default_projects_folder": "C:/Users/testuser/.o3de/Projects",
    "default_gems_folder": "C:/Users/testuser/.o3de/Gems",
    "default_templates_folder": "C:/Users/testuser/.o3de/Templates",
    "default_restricted_folder": "C:/Users/testuser/.o3de/Restricted",
    "default_third_party_folder": "C:/Users/testuser/.o3de/3rdParty",
    "projects": [],
    "external_subdirectories": [],
    "templates": [],
    "restricted": [],
    "repos": [],
    "engines": [],
    "engines_path": {}
}
'''
@pytest.fixture(scope='class')
def init_register_gem_data(request):
    request.cls.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
    request.cls.project_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
    request.cls.engine_data = json.loads(TEST_ENGINE_JSON_PAYLOAD)


@pytest.mark.usefixtures('init_register_gem_data')
class TestRegisterGem:
    engine_path = pathlib.PurePath('o3de')
    project_path = pathlib.PurePath('TestProject')

    @staticmethod
    def get_gem_json_data(gem_path: pathlib.Path = None):
        return json.loads(TEST_GEM_JSON_PAYLOAD)

    @pytest.mark.parametrize("gem_path, expected_manifest_file, expected_result", [
                                 pytest.param(pathlib.PurePath('TestGem'), pathlib.PurePath('o3de_manifest.json'), 0),
                                 pytest.param(project_path / 'TestGem', pathlib.PurePath('project.json'), 0),
                                 pytest.param(engine_path / 'TestGem', pathlib.PurePath('engine.json'), 0),
                             ])
    def test_register_gem_auto_detects_manifest_update(self, gem_path, expected_manifest_file,expected_result):

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == pathlib.Path(TestRegisterGem.project_path).resolve() / 'project.json':
                self.project_data = manifest_data
            elif manifest_path == pathlib.Path(TestRegisterGem.engine_path).resolve() / 'engine.json':
                self.engine_data = manifest_data
            else:
                self.o3de_manifest_data = manifest_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if manifest_path == TestRegisterGem.project_path:
                return self.project_data
            elif manifest_path == TestRegisterGem.engine_path:
                return self.engine_data
            return self.o3de_manifest_data

        def get_engine_json_data(engine_path: pathlib.Path = None):
            return json.loads(TEST_ENGINE_JSON_PAYLOAD)

        def get_project_json_data(project_path: pathlib.Path = None):
            return json.loads(TEST_PROJECT_JSON_PAYLOAD)

        def find_ancestor_dir(target_file_name: pathlib.PurePath, start_path: pathlib.Path):
            try:
                if target_file_name == pathlib.PurePath('project.json')\
                        and start_path.relative_to(TestRegisterGem.project_path):
                    return TestRegisterGem.project_path
            except ValueError:
                pass
            try:
                if target_file_name == pathlib.PurePath('engine.json')\
                        and start_path.relative_to(TestRegisterGem.engine_path):
                    return TestRegisterGem.engine_path
            except ValueError:
                pass
            return None

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2,\
                patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as _3,\
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as _4,\
                patch('o3de.manifest.get_gem_json_data', side_effect=TestRegisterGem.get_gem_json_data) as _5,\
                patch('o3de.utils.find_ancestor_dir_containing_file', side_effect=find_ancestor_dir) as _6,\
                patch('pathlib.Path.is_dir', return_value=True) as _7,\
                patch('o3de.validation.valid_o3de_gem_json', return_value=True) as _8:
            result = register.register(gem_path=gem_path)
            assert result == expected_result

            if expected_manifest_file == pathlib.PurePath('o3de_manifest.json'):
                assert pathlib.Path(gem_path).resolve() in map(lambda subdir: pathlib.PurePath(subdir),
                                       self.o3de_manifest_data.get('external_subdirectories', []))
            elif expected_manifest_file == pathlib.PurePath('project.json'):
                assert gem_path in map(lambda subdir: TestRegisterGem.project_path / subdir,
                                       self.project_data.get('external_subdirectories', []))
            elif expected_manifest_file == pathlib.PurePath('engine.json'):
                assert gem_path in map(lambda subdir: TestRegisterGem.engine_path / subdir,
                                       self.engine_data.get('external_subdirectories', []))
