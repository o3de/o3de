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
        # Same engine_name but different path succeeds
        pytest.param(pathlib.PurePath('D:/o3de/engine-path'), "o3de", False, 0),
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
            # registering a new engine succeeds
            pytest.param(pathlib.PurePath('D:/o3de/o3de'), "o3de", False, 0),
            # registering an engine with the same name at a new location succeeds
            pytest.param(pathlib.PurePath('F:/Open3DEngine'), "o3de", False, 0),
            # re-registering an engine with the same name at the same location succeeds
            pytest.param(pathlib.PurePath('F:/Open3DEngine'), "o3de", False, 0),
            # forcing re-registering an engine with the same name at the same location succeeds
            pytest.param(pathlib.PurePath('F:/Open3DEngine'), "o3de", True, 0),
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
    "version": "0.0.0",
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

TEST_SUB_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestSubGem",
    "version": "0.0.0",
    "display_name": "TestSubGem",
    "license": "What license TestGem uses goes here: i.e. https://opensource.org/licenses/MIT",
    "origin": "The primary repo for TestSubGem goes here: i.e. http://www.mydomain.com",
    "type": "Code",
    "summary": "A short description of TestSubGem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "TestSubGem"
    ],
    "icon_path": "preview.png",
    "requirements": ""
}
'''

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "version": "0.0.0",
    "engine": "o3de",
    "external_subdirectories": []
}
'''

TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "version": "0.0.0",
    "external_subdirectories": [],
    "projects": [],
    "templates": []
}
'''

TEST_O3DE_MANIFEST_JSON_PAYLOAD = '''
{
    "o3de_manifest_name": "testuser",
    "origin": "C:/Users/testuser/.o3de",
    "default_engines_folder": "C:/Users/testuser/O3DE/Engines",
    "default_projects_folder": "C:/Users/testuser/O3DE/Projects",
    "default_gems_folder": "C:/Users/testuser/O3DE/Gems",
    "default_templates_folder": "C:/Users/testuser/O3DE/Templates",
    "default_restricted_folder": "C:/Users/testuser/O3DE/Restricted",
    "default_third_party_folder": "C:/Users/testuser/.o3de/3rdParty",
    "projects": [],
    "external_subdirectories": [],
    "templates": [],
    "restricted": [],
    "repos": [],
    "engines": []
}
'''
@pytest.fixture(scope='function')
def init_register_gem_data(request):
    request.cls.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
    request.cls.project_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
    request.cls.engine_data = json.loads(TEST_ENGINE_JSON_PAYLOAD)
    request.cls.ancestor_gem_data = json.loads(TEST_GEM_JSON_PAYLOAD)


@pytest.mark.usefixtures('init_register_gem_data')
class TestRegisterGem:
    engine_path = pathlib.PurePath('o3de')
    project_path = pathlib.PurePath('TestProject')
    ancestor_gem_path = pathlib.PurePath('TestGem')

    @staticmethod
    def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                        project_path: pathlib.Path = None) -> dict or None:
        if (gem_name and gem_name == "TestSubGem") or \
            (gem_path and gem_path.name == "TestSubGem"):
            return json.loads(TEST_SUB_GEM_JSON_PAYLOAD)
        else:
            return json.loads(TEST_GEM_JSON_PAYLOAD)

    @pytest.mark.parametrize("gem_path, expected_manifest_file, dry_run, force_o3de_manifest_register, expected_result", [
                                 pytest.param(pathlib.PurePath('TestGem'), pathlib.PurePath('o3de_manifest.json'), False, False, 0),
                                 pytest.param(project_path / 'TestGem', pathlib.PurePath('project.json'), False, False, 0),
                                 pytest.param(project_path / 'TestGem', pathlib.PurePath('o3de_manifest.json'), False, True, 0),
                                 pytest.param(engine_path / 'TestGem', pathlib.PurePath('engine.json'), False, False, 0),
                                 pytest.param(engine_path / 'TestGem', pathlib.PurePath('o3de_manifest.json'), False, True, 0),
                                 pytest.param(pathlib.PurePath('TestGem/TestSubGem') , pathlib.PurePath('gem.json'), False, False, 0),
                                 pytest.param(pathlib.PurePath('TestGem/TestSubGem') , pathlib.PurePath('o3de_manifest.json'), False, True, 0),
                                 pytest.param(pathlib.PurePath('TestGem'), pathlib.PurePath('o3de_manifest.json'), True, False, 0),
                                 pytest.param(project_path / 'TestGem', pathlib.PurePath('project.json'), True, False, 0),
                                 pytest.param(engine_path / 'TestGem', pathlib.PurePath('engine.json'), True, False, 0),
                                 pytest.param(pathlib.PurePath('TestGem/TestSubGem') , pathlib.PurePath('gem.json'), True, False, 0),
                             ])
    def test_register_gem_auto_detects_manifest_update(self, gem_path, expected_manifest_file, dry_run,
     force_o3de_manifest_register, expected_result):

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == pathlib.Path(TestRegisterGem.ancestor_gem_path).resolve() / 'gem.json':
                self.ancestor_gem_data = manifest_data
            if manifest_path == pathlib.Path(TestRegisterGem.project_path).resolve() / 'project.json':
                self.project_data = manifest_data
            elif manifest_path == pathlib.Path(TestRegisterGem.engine_path).resolve() / 'engine.json':
                self.engine_data = manifest_data
            else:
                self.o3de_manifest_data = manifest_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if manifest_path == TestRegisterGem.ancestor_gem_path:
                return self.ancestor_gem_data
            if manifest_path == TestRegisterGem.project_path:
                return self.project_data
            elif manifest_path == TestRegisterGem.engine_path:
                return self.engine_data
            return self.o3de_manifest_data

        def get_engine_json_data(engine_name:str = None, engine_path: pathlib.Path = None):
            return json.loads(TEST_ENGINE_JSON_PAYLOAD)

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            return json.loads(TEST_PROJECT_JSON_PAYLOAD)

        def find_ancestor_dir(target_file_name: pathlib.PurePath, start_path: pathlib.Path):
            try:
                if target_file_name == pathlib.PurePath('gem.json')\
                        and start_path.relative_to(TestRegisterGem.ancestor_gem_path):
                    return TestRegisterGem.ancestor_gem_path
            except ValueError:
                pass
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
            result = register.register(gem_path=gem_path, dry_run=dry_run,
                force_register_with_o3de_manifest=force_o3de_manifest_register)
            assert result == expected_result

            if expected_manifest_file == pathlib.PurePath('o3de_manifest.json'):
                external_subdirectories = map(lambda subdir: pathlib.PurePath(subdir),
                                        self.o3de_manifest_data.get('external_subdirectories', []))
            elif expected_manifest_file == pathlib.PurePath('gem.json'):
                external_subdirectories = map(lambda subdir: TestRegisterGem.ancestor_gem_path / subdir,
                                       self.ancestor_gem_data.get('external_subdirectories', []))
            elif expected_manifest_file == pathlib.PurePath('project.json'):
                external_subdirectories = map(lambda subdir: TestRegisterGem.project_path / subdir,
                                       self.project_data.get('external_subdirectories', []))
            elif expected_manifest_file == pathlib.PurePath('engine.json'):
                external_subdirectories = map(lambda subdir: TestRegisterGem.engine_path / subdir,
                                       self.engine_data.get('external_subdirectories', []))

            gem_path = pathlib.Path(gem_path).resolve() if expected_manifest_file == pathlib.PurePath('o3de_manifest.json') else gem_path
            if dry_run:
                assert gem_path not in external_subdirectories
            else:
                assert gem_path in external_subdirectories


@pytest.fixture(scope='function')
def init_register_project_data(request):
    request.cls.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
    request.cls.project_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
    request.cls.engine_data = json.loads(TEST_ENGINE_JSON_PAYLOAD)


@pytest.mark.usefixtures('init_register_project_data')
class TestRegisterProject:
    engine_path = pathlib.PurePath('o3de')
    project_path = pathlib.PurePath('TestProject')

    @pytest.mark.parametrize(
        'test_engine_name, engine_version, engine_api_versions,'
        'registered_gem_versions, project_engine_name, project_engine_version, '
        'project_gems, project_compatible_engines, project_engine_api_dependencies,'
        'gem_compatible_engines, gem_engine_api_dependencies,'
        'force, dry_run, expected_result', [
            # passes when registering without version information
            pytest.param('o3de1', None, None, { 'gem1':'' }, None, None, ['gem1'], None, None, None, None, False, False, 0),
            pytest.param('o3de2', '', None, { 'gem1':'' }, None, None, ['gem1'], None, None, None, None, False, False, 0),
            pytest.param('o3de3', '', None, { 'gem1':'' }, None, None, ['gem1'], None, None, None, None, False, True, 0),
            # fails when compatible_engines has no match
            pytest.param('o3de4', '2.3.4', None, { 'gem1':'' }, None, None, ['gem1'], ['o3de3==1.2.3'], None, None, None,  False, False, 1),
            pytest.param('o3de5', '1.2.3', None, { 'gem1':'' }, "", "", ['gem1'], ['o3de1'], None, None, None, False, False, 1),
            # passes when forced
            pytest.param('o3de6', '1.2.3', None, { 'gem1':'' }, None, None, ['gem1'], ['o3de1'], None, None, None, True, False, 0),
            # passes when compatible_engines has match
            pytest.param('o3de7', '0.0.0', None, { 'gem1':'' }, None, None, ['gem1'], ['o3de7'], None, None, None, False, False, 0),
            pytest.param('o3de8', '1.2.3', None, { 'gem1':'' }, None, None, ['gem1'], ['o3de8>=1.2.3','o3de-sdk==2.3.4'], None, None, None, False, False, 0),
            # fails when gem is used that is not known to be compatible with version 1.2.3
            pytest.param('o3de9', '1.2.3', None, { 'gem1':'' }, None, None, ['gem1'], ['o3de9'], None, ['o3de==2.3.4'], None, False, False, 1),
            # passes when gem is used that is known compatible with version 1.2.3
            pytest.param('o3de10', '1.2.3', None, { 'gem1':'' }, None, None, ['gem1'], ['o3de10'], None, ['o3de10==1.2.3'], None, False, False, 0),
            # passes when compatible engine not found but compatible api found
            pytest.param('o3de11', '1.2.3', {'api':'1.2.3'}, { 'gem1':'' }, "", "", ['gem1'], ['o3de11==2.3.4'], ['api==1.2.3'], None, None, False, False, 0),
            # passes when compatible engine found and no compatible api found
            pytest.param('o3de12', '1.2.3', {'api':'2.3.4'}, { 'gem1':'' }, "", "", ['gem1'], ['o3de12==1.2.3'], ['api==1.2.3'], None, None, False, False, 0),
            # fails when no compatible engine or api found
            pytest.param('o3de13', '1.2.3', {'api':'2.3.4'}, { 'gem1':'' }, None, None, ['gem1'], ['o3de13==3.4.5'], ['api==4.5.6'], None, None, False, False, 1),
            # passes when gem is used with compatible engine and api found
            pytest.param('o3de14', '1.2.3', {'api':'2.3.4'}, { 'gem1':'' }, None, None, ['gem1'], None, None, ['o3de14>=1.0.0'], ['api~=1.2.3'] , False, False, 0),
            # fails when gem is used with no compatible engine or api found
            pytest.param('o3de15', '1.2.3', {'api':'2.3.4'}, { 'gem1':'' }, None, None, ['gem1'], None, None, ['o3de15==2.3.4'], ['api==1.2.3'] , False, False, 1),
            # fails as usual with same test when dry-run specified
            pytest.param('o3de16', '1.2.3', {'api':'2.3.4'}, { 'gem1':'' }, None, None, ['gem1'], None, None, ['o3de16==2.3.4'], ['api==1.2.3'] , False, True, 1),
        ]
    )
    def test_register_project(self, test_engine_name, engine_version, engine_api_versions,
                                registered_gem_versions, project_engine_name, project_engine_version,
                                project_gems, project_compatible_engines, project_engine_api_dependencies,
                                gem_compatible_engines, gem_engine_api_dependencies,
                                force, dry_run, expected_result):
        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == pathlib.Path(TestRegisterProject.project_path).resolve() / 'project.json':
                self.project_data = manifest_data
            elif manifest_path == pathlib.Path(TestRegisterProject.engine_path).resolve() / 'engine.json':
                self.engine_data = manifest_data
            else:
                self.o3de_manifest_data = manifest_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if manifest_path == TestRegisterProject.project_path:
                return self.project_data
            elif manifest_path == TestRegisterProject.engine_path:
                return self.engine_data
            return self.o3de_manifest_data

        def get_gems_json_data_by_name( engine_path:pathlib.Path = None,
                                        project_path: pathlib.Path = None,
                                        include_manifest_gems: bool = False,
                                        include_engine_gems: bool = False,
                                        external_subdirectories: list = None
                                        ) -> dict:
            all_gems_json_data = {}
            for gem_name in registered_gem_versions.keys():
                all_gems_json_data[gem_name] = [get_gem_json_data(gem_name=gem_name)]
            return all_gems_json_data

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                            project_path: pathlib.Path = None):
            gem_json_data = json.loads(TEST_GEM_JSON_PAYLOAD)
            if gem_path and gem_path.name in registered_gem_versions:
                gem_name = gem_path.name
            if gem_name:
                gem_json_data['gem_name'] = gem_name
            if gem_name and gem_name in registered_gem_versions:
                gem_json_data['version'] = registered_gem_versions[gem_name]
            if gem_compatible_engines:
                gem_json_data['compatible_engines'] = gem_compatible_engines
            if gem_engine_api_dependencies:
                gem_json_data['engine_api_dependencies'] = gem_engine_api_dependencies
            return gem_json_data

        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None):
            engine_json_data = json.loads(TEST_ENGINE_JSON_PAYLOAD)
            if test_engine_name != None:
                engine_json_data['engine_name'] = test_engine_name

            # we want to allow for testing the case where these fields
            # are missing or empty
            if engine_version != None:
                engine_json_data['version'] = engine_version
            if engine_api_versions != None:
                engine_json_data['api_versions'] = engine_api_versions

            return engine_json_data

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            project_json_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)

            # we want to allow for testing the case where these fields
            # are missing or empty
            if project_engine_name != None:
                project_json_data['engine'] = project_engine_name
            if project_engine_version != None:
                project_json_data['engine_version'] = project_engine_version
            if project_gems != None:
                project_json_data['gem_names'] = project_gems
            if project_compatible_engines != None:
                project_json_data['compatible_engines'] = project_compatible_engines
            if project_engine_api_dependencies != None:
                project_json_data['engine_api_dependencies'] = project_engine_api_dependencies

            return project_json_data

        def find_ancestor_dir(target_file_name: pathlib.PurePath, start_path: pathlib.Path):
            try:
                if target_file_name == pathlib.PurePath('project.json')\
                        and start_path.relative_to(TestRegisterProject.project_path):
                    return TestRegisterProject.project_path
            except ValueError:
                pass
            try:
                if target_file_name == pathlib.PurePath('engine.json')\
                        and start_path.relative_to(TestRegisterProject.engine_path):
                    return TestRegisterProject.engine_path
            except ValueError:
                pass
            return None

        def get_enabled_gem_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                platform: str = 'Common'):
            return None

        def get_enabled_gems(cmake_file: pathlib.Path) -> set:
            return project_gems

        def get_all_gems(project_path: pathlib.Path = None) -> list:
            return project_gems

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
            patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2,\
            patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as _3,\
            patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as _4,\
            patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as _5,\
            patch('o3de.manifest.get_all_gems', side_effect=get_all_gems) as _6,\
            patch('o3de.utils.find_ancestor_dir_containing_file', side_effect=find_ancestor_dir) as _7,\
            patch('pathlib.Path.is_dir', return_value=True) as _8,\
            patch('o3de.validation.valid_o3de_project_json', return_value=True) as _9, \
            patch('o3de.utils.backup_file', return_value=True) as _10, \
            patch('o3de.manifest.get_enabled_gem_cmake_file', side_effect=get_enabled_gem_cmake_file) as _11, \
            patch('o3de.manifest.get_gems_json_data_by_name', side_effect=get_gems_json_data_by_name) as get_gems_json_data_by_name_patch,\
            patch('o3de.manifest.get_enabled_gems', side_effect=get_enabled_gems) as _12:

            result = register.register(project_path=TestRegisterProject.project_path, force=force, dry_run=dry_run)

            if dry_run:
                assert TestRegisterProject.project_path not in map(lambda subdir: pathlib.PurePath(subdir),
                                        self.o3de_manifest_data.get('external_subdirectories', []))

            assert result == expected_result
