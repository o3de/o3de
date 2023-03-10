#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import pytest
import pathlib
from unittest.mock import patch

from o3de import enable_gem


TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "version": "0.0.0",
    "restricted_name": "o3de",
    "file_version": 1,
    "O3DEVersion": "0.0.0",
    "copyright_year": 2021,
    "build": 0,
    "external_subdirectories": [
        "Gems/TestGem2"
    ],
    "projects": [
    ],
    "templates": [
        "Templates/MinimalProject"
    ]
}
'''

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "version": "0.0.0",
    "origin": "The primary repo for TestProject goes here: i.e. http://www.mydomain.com",
    "license": "What license TestProject uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "TestProject",
    "summary": "A short description of TestProject.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [
        "TestProject"
    ],
    "icon_path": "preview.png",
    "engine": "o3de-install",
    "restricted_name": "projects",
    "external_subdirectories": [
    ]
}
'''

TEST_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
    "version": "0.0.0",
    "display_name": "TestGem",
    "license": "Apache-2.0 Or MIT",
    "license_url": "https://github.com/o3de/o3de/blob/development/LICENSE.txt",
    "origin": "Open 3D Engine - o3de.org",
    "origin_url": "https://github.com/o3de/o3de",
    "type": "Code",
    "summary": "A short description of TestGem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "TestGem"
    ],
    "icon_path": "preview.png",
    "requirements": "Any requirement goes here.",
    "documentation_url": "The link to the documentation goes here.",
    "dependencies": [
    ]
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
    "projects": [
        "D:/MinimalProject"
    ],
    "external_subdirectories": [],
    "templates": [],
    "restricted": [],
    "repos": [],
    "engines": [
        "D:/o3de/o3de"
    ]
}
'''

@pytest.fixture(scope='class')
def init_enable_gem_data(request):
    class EnableGemData:
        def __init__(self):
            self.engine_data = json.loads(TEST_ENGINE_JSON_PAYLOAD)
            self.project_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            self.gem_data = json.loads(TEST_GEM_JSON_PAYLOAD)
    request.cls.enable_gem = EnableGemData()


@pytest.mark.usefixtures('init_enable_gem_data')
class TestEnableGemCommand:
    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize("gem_path, project_path, gem_registered_with_project, gem_registered_with_engine,"
                             "optional, expected_result", [
        pytest.param(pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, True, False, 0),
        pytest.param(pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, False, False, 0),
        pytest.param(pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), True, False, False, 0),
        pytest.param(pathlib.PurePath('TestGem'), pathlib.PurePath('TestProject'), False, False, False, 0),
        pytest.param(pathlib.PurePath('TestGem'), pathlib.PurePath('TestProject'), False, False, True, 0),
        ]
    )
    def test_enable_gem_registers_gem_name_with_project_json(self, gem_path, project_path, gem_registered_with_project,
                                                             gem_registered_with_engine, optional, expected_result):

        def get_registered_path(project_name: str = None, gem_name: str = None, engine_name: str = None) -> pathlib.Path:
            if project_name:
                return project_path
            elif gem_name:
                return gem_path
            elif engine_name:
                return pathlib.PurePath('o3de')
            return None

        def save_o3de_manifest(new_project_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == project_path / 'project.json':
                self.enable_gem.project_data = new_project_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if not manifest_path:
                return json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            return None

        def get_gems_json_data_by_name(engine_path:pathlib.Path = None, 
                                       project_path: pathlib.Path = None, 
                                       include_manifest_gems: bool = False,
                                       include_engine_gems: bool = False,
                                       external_subdirectories: list = None
                                       ) -> dict:
            return {}

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            return self.enable_gem.project_data

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                            project_path: pathlib.Path = None) -> dict or None:
            return self.enable_gem.gem_data

        def get_engine_json_data(engine_name:str = None, engine_path:pathlib.Path = None):
            return self.enable_gem.engine_data

        def get_project_gems(project_path: pathlib.Path):
            return [gem_path] if gem_registered_with_project else []

        def get_enabled_gems(cmake_file: pathlib.Path) -> set:
            return set() 

        def get_engine_gems():
            return [gem_path] if gem_registered_with_engine else []

        def is_file(path : pathlib.Path) -> bool:
            if path.match("*/user/project.json"):
                return False
            return True

        with patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_patch,\
                patch('pathlib.Path.is_file', new=is_file) as pathlib_is_file_patch, \
                patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as load_o3de_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch,\
                patch('o3de.manifest.get_gems_json_data_by_name', side_effect=get_gems_json_data_by_name) as get_gems_json_data_by_name_patch,\
                patch('o3de.manifest.get_registered', side_effect=get_registered_path) as get_registered_patch,\
                patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_gems', side_effect=get_project_gems) as get_project_gems_patch,\
                patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch,\
                patch('o3de.manifest.get_engine_gems', side_effect=get_engine_gems) as get_engine_gems_patch,\
                patch('o3de.manifest.get_enabled_gems', side_effect=get_enabled_gems) as get_enabled_gems_patch,\
                patch('pathlib.Path.resolve', new=self.resolve) as pathlib_is_resolve_mock,\
                patch('o3de.validation.valid_o3de_gem_json', return_value=True) as valid_gem_json_patch:

            self.enable_gem.project_data.pop('gem_names', None)
            result = enable_gem.enable_gem_in_project(gem_path=gem_path, project_path=project_path, optional=optional)
            assert result == expected_result

            gem_json = get_gem_json_data(gem_path=gem_path, project_path=project_path)
            project_json = get_project_json_data(project_path=project_path)
            gem_name = gem_json.get('gem_name', '')
            gem = gem_name if not optional else {'name':gem_name, 'optional':optional}
            assert gem in project_json.get('gem_names', [])

    @pytest.mark.parametrize("gem_registered_with_project, gem_registered_with_engine, "
                             "gem_version, gem_dependencies, gem_json_data_by_name, dry_run, force, "
                             "compatible_engines, engine_api_dependencies, test_engine_name, test_engine_version, "
                             "test_engine_api_versions, is_optional_gem, expected_result", [
        # passes when no version information is provided
        pytest.param(False, False, None, [], {}, False, False, [], None, 'o3de-test', '', None, False, 0),
        pytest.param(False, True, '', [], {}, False, False, [], None, 'o3de-test', '', None, False, 0),
        # passes when a compatible engine version is provided
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test>=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test>=1.0.0, <2.3.4'], [], 'o3de-test', '2.0.1', {}, False, 0),
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test'], [], 'o3de-test', '', {}, False, 0),
        pytest.param(True, False, '1.2.3', [], {}, False, False, ['o3de-test'], [], 'o3de-test', '1.2.3', {}, False, 0),
        # fails when no compatible engine version is found
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test==1.2.3'], [], 'o3de-test', None, {}, False, 1),
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test==1.2.3'], [], 'o3de-test', '', {}, False, 1),
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test<1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 1),
        # passes when forced even if no compatible engine version is found
        pytest.param(False, False, '1.2.3', [], {}, False, True, ['o3de-test<1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        # fails when no compatible engine is found
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test<1.0.0'], [], 'other-engine', '0.0.0', {}, False, 1),
        # passes when dependent gems and engines are found
        pytest.param(False, False, '1.2.3', ['testgem1==1.2.3','testgem2>1.0.0'], { 'testgem1':{'version':'1.2.3'}, 'testgem2':{'version':'2.0.0'}}, 
                    False, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        # passes when dependent gems with implicit dependencies and engines are found
        pytest.param(False, False, '1.2.3', ['testgem1==1.2.3'], { 'testgem1':{'version':'1.2.3','dependencies':['testgem2>1.0.0']}, 'testgem2':{'version':'2.0.0'}}, 
                    False, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        # passes when dependent gems without version specifiers are used
        pytest.param(False, False, '1.2.3', ['testgem1','testgem2'], { 'testgem1':{'version':'1.2.3'}, 'testgem2':{'version':'2.0.0'}},
                    False, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        # passes when dependent gems with and without version specifiers are used
        pytest.param(False, False, '1.2.3', ['testgem1','testgem2~=2.0.0'], { 'testgem1':{'version':'1.2.3'}, 'testgem2':{'version':'2.0.1'}},
                    False, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        # fails when dependent gem is not found
        pytest.param(False, False, '1.2.3', ['testgem1==1.2.3','testgem2>1.0.0'], { 'testgem1':{'version':'1.2.3'}},
                    False, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '0.0.0', {}, False, 1),
        # fails when implicit dependent gem is not found
        pytest.param(False, False, '1.2.3', ['testgem1==1.2.3'], { 'testgem1':{'version':'1.2.3','dependencies':['testgem2>1.0.0']}},
                    False, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '0.0.0', {}, False, 1),
        # fails when dependent gem with wrong version found
        pytest.param(False, False, '1.2.3', ['testgem1==1.2.3','testgem2>1.0.0'], { 'testgem1':{'version':'1.0.0'}, 'testgem2':{'version':'1.0.0'}},
                    False, False, ['o3de-test~=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 1),
        # does not modify project when check only
        pytest.param(False, False, '1.2.3', ['testgem1==1.2.3','testgem2>1.0.0'], { 'testgem1':{'version':'1.2.3'}, 'testgem2':{'version':'2.0.1'}},
                    True, False, ['o3de-test<=1.0.0'], [], 'o3de-test', '1.0.0', {}, False, 0),
        # passes when a engine api versions found
        pytest.param(False, False, '1.2.3', [], {}, False, False, [], ['api==1.2.3'], 'o3de-test', '1.0.0', {'api':'1.2.3'}, False, 0),
        # passes when a engine api without a version specifier is used and api exists 
        pytest.param(False, False, '1.2.3', [], {}, False, False, [], ['api'], 'o3de-test', '1.0.0', {'api':'1.2.3'}, False, 0),
        # fails when engine api not found 
        pytest.param(False, False, '1.2.3', [], {}, False, False, [], ['api==2.3.4'], 'o3de-test', '1.0.0', {'otherapi':'1.2.3'}, False, 1),
        # fails when engine api version incorrect
        pytest.param(False, False, '1.2.3', [], {}, False, False, [], ['api==2.3.4'], 'o3de-test', '1.0.0', {'api':'1.2.3'}, False, 1),
        # passes when engine matches compatible_engines but engine api version doesn't match 
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test>=1.0.0'], ['api==1.2.3'], 'o3de-test', '2.0.0', {'api':'2.3.4'}, False, 0),
        # passes when engine version doesn't match but api version matches
        pytest.param(False, False, '1.2.3', [], {}, False, False, ['o3de-test==1.0.0'], ['api~=1.0'], 'o3de-test', '2.0.0', {'api':'1.9.2'}, False, 0),
        # passes when a engine api versions found and optional
        pytest.param(False, False, '1.2.3', [], {}, False, False, [], ['api==1.2.3'], 'o3de-test', '1.0.0', {'api':'1.2.3'}, True, 0),
        ],
    )
    def test_enable_gem_checks_engine_compatibility(self, gem_registered_with_project, gem_registered_with_engine, 
                                                    gem_version, gem_dependencies, 
                                                    gem_json_data_by_name, dry_run, force, compatible_engines, 
                                                    engine_api_dependencies, test_engine_name, test_engine_version, 
                                                    test_engine_api_versions, is_optional_gem, expected_result):
        project_path = pathlib.PurePath('TestProject')
        gem_path = pathlib.PurePath('TestProject/TestGem')
        engine_path = pathlib.PurePath('o3de')

        def get_registered_path(engine_name: str = None, project_name: str = None, gem_name: str = None) -> pathlib.Path:
            if project_name:
                return project_path
            elif gem_name:
                return gem_path
            elif engine_name:
                return engine_path
            return None

        def save_o3de_manifest(new_project_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == project_path / 'project.json':
                self.enable_gem.project_data = new_project_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if not manifest_path:
                return json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            return None

        def get_gems_json_data_by_name( engine_path:pathlib.Path = None, 
                                        project_path: pathlib.Path = None, 
                                        include_manifest_gems: bool = False,
                                        include_engine_gems: bool = False,
                                        external_subdirectories: list = None
                                        ) -> dict:
            all_gems_json_data = {}
            # include the gem we're enabling
            gem_json_data = get_gem_json_data()
            all_gems_json_data[gem_json_data['gem_name']] = [gem_json_data]

            for gem_name in gem_json_data_by_name.keys():
                all_gems_json_data[gem_name] = [get_gem_json_data(gem_name=gem_name)]
            return all_gems_json_data

        def get_enabled_gem_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                platform: str = 'Common'):
            return pathlib.Path() 

        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None) -> dict or None:
            engine_data = self.enable_gem.engine_data.copy()
            engine_data['engine_name'] = test_engine_name
            if test_engine_version != None:
                engine_data['version'] = test_engine_version
            if test_engine_api_versions:
                engine_data['api_versions'] = test_engine_api_versions
            return engine_data

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            project_data = self.enable_gem.project_data
            project_data['engine'] = test_engine_name
            if test_engine_version != None:
                project_data['engine_version'] = test_engine_version
            return project_data

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                            project_path: pathlib.Path = None) -> dict or None:
            gem_data = self.enable_gem.gem_data.copy()
            if gem_name in gem_json_data_by_name:
                # gem dependencies data
                gem_data.update(gem_json_data_by_name[gem_name])
                gem_data['gem_name'] = gem_name 
            else:
                # test gem data
                gem_data['version'] = gem_version
                gem_data['dependencies'] = gem_dependencies
                if compatible_engines:
                    gem_data['compatible_engines'] = compatible_engines
                if engine_api_dependencies:
                    gem_data['engine_api_dependencies'] = engine_api_dependencies
            return gem_data

        def get_project_gems(project_path: pathlib.Path):
            return [gem_path] if gem_registered_with_project else []

        def get_enabled_gems(cmake_file: pathlib.Path) -> set:
            return set() 

        def get_engine_gems():
            gem_paths = list(map(lambda path:path, gem_json_data_by_name.keys()))
            if gem_registered_with_engine:
                gem_paths.append(gem_path)
            return gem_paths


        def is_file(path : pathlib.Path) -> bool:
            if path.match("*/user/project.json"):
                return False
            return True

        with patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_patch,\
                patch('pathlib.Path.is_file', new=is_file) as pathlib_is_file_patch, \
                patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as load_o3de_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch,\
                patch('o3de.manifest.get_registered', side_effect=get_registered_path) as get_registered_patch,\
                patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch,\
                patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_gems', side_effect=get_project_gems) as get_project_gems_patch,\
                patch('o3de.manifest.get_engine_gems', side_effect=get_engine_gems) as get_engine_gems_patch,\
                patch('o3de.manifest.get_gems_json_data_by_name', side_effect=get_gems_json_data_by_name) as get_gems_json_data_by_name_patch,\
                patch('o3de.manifest.get_enabled_gems', side_effect=get_enabled_gems) as get_enabled_gems_patch,\
                patch('o3de.manifest.get_enabled_gem_cmake_file', side_effect=get_enabled_gem_cmake_file) as get_enabled_gem_cmake_patch, \
                patch('pathlib.Path.resolve', new=self.resolve) as pathlib_is_resolve_mock,\
                patch('o3de.validation.valid_o3de_gem_json', return_value=True) as valid_gem_json_patch:

            self.enable_gem.project_data.pop('gem_names', None)
            result = enable_gem.enable_gem_in_project(gem_path=gem_path, project_path=project_path, force=force, dry_run=dry_run, optional=is_optional_gem)
            assert result == expected_result

            if not result:
                gem_json = get_gem_json_data(gem_path=gem_path, project_path=project_path)
                project_json = get_project_json_data(project_path=project_path)
                gem_name = gem_json.get('gem_name', '')
                gem = gem_name if not is_optional_gem else {'name':gem_name, 'optional':is_optional_gem}
                if not dry_run:
                    assert gem in project_json.get('gem_names', [])
                else:
                    assert gem not in project_json.get('gem_names', [])

    @pytest.mark.parametrize("gem_name, gem_json_data_by_path, force,"
                             "expected_result", [
        # when no version specifier used and gem exists expect it is found
        pytest.param("GemA", {pathlib.PurePath('GemA'):{"gem_name":"GemA"}}, False, 0),
        # when version specifier used and gem exists with version expect it is found
        pytest.param("GemA==1.0", {pathlib.PurePath('GemA'):{"gem_name":"GemA","version":"1.0.0"}}, False, 0),
        # when version specifier used and gem version doesn't exists expect it is not found
        pytest.param("GemA==1.0", {pathlib.PurePath('GemA'):{"gem_name":"GemA","version":"2.0.0"}}, False, 1),
        # when version specifier used and multiple gem versions exist, expect gem activated 
        pytest.param("GemA>1.0", {
            pathlib.PurePath('GemA1'):{"gem_name":"GemA","version":"2.0.0"},
            pathlib.PurePath('GemA2'):{"gem_name":"GemA","version":"3.0.0"},
            }, False, 0),
        # when dependencies cannot be satisfied expect fails, the following requires gemC 1.0.0 and 2.0.0
        pytest.param("GemA", {
            pathlib.PurePath('GemA'):{"gem_name":"GemA","version":"1.0.0","dependencies":['GemB==3.0.0','GemC==2.0.0']},
            pathlib.PurePath('GemB'):{"gem_name":"GemB","version":"3.0.0","dependencies":['GemC==1.0.0']},
            pathlib.PurePath('GemC1'):{"gem_name":"GemC","version":"1.0.0"},
            pathlib.PurePath('GemC2'):{"gem_name":"GemC","version":"2.0.0"},
            }, False, 1),
        ],
    )
    def test_enable_gem_with_version_specifier_checks_compatibility(self, gem_name, gem_json_data_by_path, 
                                                    force, expected_result):
        project_path = pathlib.PurePath('TestProject')
        engine_path = pathlib.PurePath('o3de')

        def get_manifest_engines() -> list:
            return [engine_path]

        def get_project_engine_path(project_path:str or pathlib.Path) -> pathlib.Path or None:
            return engine_path

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if not manifest_path:
                return json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            return None

        def save_o3de_manifest(new_project_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == project_path / 'project.json':
                self.enable_gem.project_data = new_project_data
            return True

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                            project_path: pathlib.Path = None) -> dict or None:
            gem_data = self.enable_gem.gem_data.copy()
            if gem_path in gem_json_data_by_path:
                gem_data.update(gem_json_data_by_path[gem_path])
            return gem_data

        def get_gems_json_data_by_name( engine_path:pathlib.Path = None, 
                                        project_path: pathlib.Path = None, 
                                        include_manifest_gems: bool = False,
                                        include_engine_gems: bool = False,
                                        external_subdirectories: list = None
                                        ) -> dict:
            all_gems_json_data = {}
            for gem_path in gem_json_data_by_path.keys():
                gem_json_data = get_gem_json_data(gem_path=gem_path)
                gem_name = gem_json_data.get('gem_name','')

                entries = all_gems_json_data.get(gem_name, [])
                entries.append(gem_json_data)
                all_gems_json_data[gem_name] = entries

            return all_gems_json_data
        def is_file(path : pathlib.Path) -> bool:
            if path.match("*/user/project.json"):
                return False
            return True

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            return self.enable_gem.project_data

        def get_engine_json_data(engine_name:str = None, engine_path:pathlib.Path = None):
            return self.enable_gem.engine_data

        def get_project_gems(project_path: pathlib.Path):
            return []

        def get_enabled_gems(cmake_file: pathlib.Path) -> set:
            return set() 

        def get_engine_gems():
            return []
        
        def get_all_gems(project_path: pathlib.Path = None) -> list:
            return list(gem_json_data_by_path.keys())

        def get_json_data(object_typename: str,
                        object_path: str or pathlib.Path,
                        object_validator: callable) -> dict or None:
            return gem_json_data_by_path.get(object_path)


        with patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_patch,\
            patch('pathlib.Path.is_file', new=is_file) as pathlib_is_file_patch, \
            patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as load_o3de_manifest_patch, \
            patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch,\
            patch('o3de.manifest.get_manifest_engines', side_effect=get_manifest_engines) as get_manifest_engines_patch,\
            patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch,\
            patch('o3de.manifest.get_json_data', side_effect=get_json_data) as get_json_data_patch,\
            patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch,\
            patch('o3de.manifest.get_gems_json_data_by_name', side_effect=get_gems_json_data_by_name) as get_gems_json_data_by_name_patch,\
            patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_gem_json_data_patch,\
            patch('o3de.manifest.get_project_gems', side_effect=get_project_gems) as get_project_gems_patch,\
            patch('o3de.manifest.get_project_engine_path', side_effect=get_project_engine_path) as get_project_engine_path_patch,\
            patch('o3de.manifest.get_engine_gems', side_effect=get_engine_gems) as get_engine_gems_patch,\
            patch('o3de.manifest.get_all_gems', side_effect=get_all_gems) as get_all_gems_patch,\
            patch('o3de.manifest.get_enabled_gems', side_effect=get_enabled_gems) as get_enabled_gems_patch,\
            patch('pathlib.Path.resolve', new=self.resolve) as pathlib_is_resolve_mock,\
            patch('o3de.validation.valid_o3de_gem_json', return_value=True) as valid_gem_json_patch:

            # remove any existing 'gem_names'
            self.enable_gem.project_data.pop('gem_names', None)
            result = enable_gem.enable_gem_in_project(gem_name=gem_name, project_path=project_path, force=force)
            assert result == expected_result
            if expected_result == 0:
                project_json = get_project_json_data(project_path=project_path)
                assert gem_name in project_json.get('gem_names', [])
