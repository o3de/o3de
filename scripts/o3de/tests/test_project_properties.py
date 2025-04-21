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

from o3de import project_properties


TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "version": "0.0.0",
    "external_subdirectories": [
    ],
    "projects": [],
    "templates": []
}
'''

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
    "version" : "0.0.0",
    "compatible_engines" : [
        "o3de-sdk==2205.01"
    ],
    "engine_api_dependencies" : [
        "framework==1.2.3"
    ],
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
    "gem_names": [
        "ExistingGemA",
        {
            "name":"ExistingGemB",
            "optional": true
        }
    ],
    "external_subdirectories": [
        "D:/TestGem"
    ]
}
'''


@pytest.fixture(scope='function')
def init_project_json_data(request):
    class ProjectJsonData:
        def __init__(self):
            self.data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            self.path = None
    request.cls.project_json = ProjectJsonData()

@pytest.mark.usefixtures('init_project_json_data')
class TestEditProjectProperties:

    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize("project_path, project_name, project_new_name, project_id, project_origin,\
                              project_display, project_summary, project_icon, project_version, \
                              add_tags, delete_tags, replace_tags, expected_tags, \
                              add_gem_names, delete_gem_names, replace_gem_names, expected_gem_names, \
                              engine_name, \
                              add_compatible_engines, delete_compatible_engines, replace_compatible_engines, \
                              expected_compatible_engines, \
                              add_engine_api_dependencies, remove_engine_api_dependencies, replace_engine_api_dependencies,\
                              expected_engine_api_dependencies,\
                              is_optional_gem, user, engine_path, engine_finder_cmake_path,\
                              expected_result",  [
        pytest.param(pathlib.Path('E:/TestProject'),
                    'ProjNameA1', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['ExistingGemA',{'name':'ExistingGemB','optional':True},'GemB'],
                    'NewEngineName',
                    'o3de>=1.0', 'o3de-sdk==2205.01', None, ['o3de>=1.0'],
                    ['editor==2.3.4'], None, None, ['framework==1.2.3','editor==2.3.4'],
                    False, True, pathlib.Path('D:/TestEngine'), pathlib.Path('cmake/CustomEngineFinder.cmake'),
                    0),
        # when adding the same gem with a different version, expect only the final gem version remains
        pytest.param(pathlib.Path('E:/TestProject'),
                    'ProjNameA1', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'ExistingGemA==1.0.0 GemB', None, None, ['ExistingGemA==1.0.0',{'name':'ExistingGemB','optional':True},'GemB'],
                    'NewEngineName',
                    'o3de>=1.0', 'o3de-sdk==2205.01', None, ['o3de>=1.0'],
                    ['editor==2.3.4'], None, None, ['framework==1.2.3','editor==2.3.4'],
                    False, True, pathlib.Path('D:/TestEngine'), pathlib.Path('cmake/CustomEngineFinder.cmake'),
                    0),
        pytest.param(pathlib.Path('D:/TestProject'),
                    'ProjNameA2', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB','ExistingGemA',{'name':'ExistingGemB','optional':True}],
                    'o3de-sdk',
                    'c==4.3.2.1', None, 'a>=0.1 b==1.0,==2.0', ['a>=0.1', 'b==1.0,==2.0'],
                    ['launcher==3.4.5'], ['framework==1.2.3'], None, ['launcher==3.4.5'],
                    False, False, None, None,
                    0),
        pytest.param(pathlib.Path('D:/TestProject'),
                    'ProjNameA3', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB','ExistingGemA',{'name':'ExistingGemB','optional':True}],
                    'o3de-install',
                    None, 'o3de-sdk==2205.01', None, [],
                    None, None, ['framework==9.8.7'], ['framework==9.8.7'],
                    False, False, None, None,
                    0),
        pytest.param(pathlib.Path('D:/TestProject'),
                    'ProjNameA4', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    None, None, '', ['ExistingGemA',{'name':'ExistingGemB','optional':True}],
                    'o3de-custom==1.0.0',
                    None, None, [], [],
                    None, None, [], [],
                    False, False, None, None,
                    0),
        pytest.param(pathlib.Path('F:/TestProject'),
                    'ProjNameA5', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB','ExistingGemA',{'name':'ExistingGemB','optional':True}],
                    None,
                    None, None, 'invalid', ['b==1.0,==2.0'], # invalid version
                    None, None, None, ['framework==1.2.3'],
                    False, False, None, None,
                    1),
        pytest.param('', # invalid path
                    'ProjNameA6', 'ProjNameB', 'IDB', 'OriginB', 
                    'DisplayB', 'SummaryB', 'IconB', '1.0.0.0',
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    ['GemA','GemB'], None, ['GemC'], ['GemC','ExistingGemA',{'name':'ExistingGemB','optional':True}],
                    None,
                    None, None, 'o3de-sdk==2205.1', ['o3de-sdk==2205.1'],
                    None, None, None, ['framework==1.2.3'],
                    False, False, None, None,
                    1),
        # test with an optional gem
        pytest.param(pathlib.Path('D:/TestProject'),
                    'ProjNameA4', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A', None, None, ['A', 'TestProject'],
                    ['GemA'], None, '', ['ExistingGemA',{'name':'ExistingGemB','optional':True},{'name':'GemA','optional':True}],
                    'o3de~=1.2',
                    None, None, [], [],
                    None, None, [], [],
                    True, True, pathlib.Path(''), None,
                    0),
        # fails when trying to set engine_path in shared project.json
        pytest.param(pathlib.Path('D:/TestProject'),
                    'ProjNameA4', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A', None, None, ['A', 'TestProject'],
                    ['GemA'], None, '', [{'name':'GemA','optional':True}],
                    'o3de-install',
                    None, None, [], [],
                    None, None, [], [],
                    False, False, pathlib.Path('D:/TestEngine'), None,
                    1),
        ]
    )
    def test_edit_project_properties(self, project_path, project_name, project_new_name, project_id, project_origin,
                                     project_display, project_summary, project_icon, project_version, 
                                     add_tags, delete_tags, replace_tags, expected_tags,
                                     add_gem_names, delete_gem_names, replace_gem_names, expected_gem_names,
                                     engine_name,
                                     add_compatible_engines, delete_compatible_engines, replace_compatible_engines,
                                     expected_compatible_engines,
                                     add_engine_api_dependencies, remove_engine_api_dependencies, 
                                     replace_engine_api_dependencies, expected_engine_api_dependencies,
                                     is_optional_gem, user, engine_path, engine_finder_cmake_path,
                                     expected_result):

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            if not project_path:
                self.project_json.data = None
                return None
            return self.project_json.data

        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None) -> dict or None:
            return TEST_ENGINE_JSON_PAYLOAD

        def save_o3de_manifest(new_proj_data: dict, project_path) -> bool:
            self.project_json.data = new_proj_data
            self.project_json.path = project_path
            return True

        with patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_project_json_data_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch, \
                patch('pathlib.Path.resolve', new=self.resolve) as pathlib_is_resolve_mock,\
                patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch:
            result = project_properties.edit_project_props(project_path, project_name, project_new_name, project_id,
                                                           project_origin, project_display, project_summary, project_icon,
                                                           add_tags, delete_tags, replace_tags,
                                                           add_gem_names, delete_gem_names, replace_gem_names,
                                                           engine_name,
                                                           add_compatible_engines, delete_compatible_engines, replace_compatible_engines,
                                                           project_version, is_optional_gem,
                                                           add_engine_api_dependencies, remove_engine_api_dependencies, replace_engine_api_dependencies,
                                                           user, engine_path, engine_finder_cmake_path)
            assert result == expected_result
            if result == 0:
                assert self.project_json.data
                assert self.project_json.data.get('project_name', '') == project_new_name
                assert self.project_json.data.get('project_id', '') == project_id
                assert self.project_json.data.get('origin', '') == project_origin
                assert self.project_json.data.get('display_name', '') == project_display
                assert self.project_json.data.get('summary', '') == project_summary
                assert self.project_json.data.get('icon_path', '') == project_icon
                assert self.project_json.data.get('version', '') == project_version

                expected_project_json_path = project_path if not user else project_path / 'user'
                expected_project_json_path /= 'project.json'
                assert self.project_json.path.as_posix() == expected_project_json_path.as_posix()

                if engine_path:
                    engine_path = engine_path.as_posix() if engine_path.name else None
                assert self.project_json.data.get('engine_path') == (engine_path if user else None)
                assert self.project_json.data.get('engine_finder_cmake_path') == (engine_finder_cmake_path.as_posix() if engine_finder_cmake_path else None)
                
                if engine_name:
                    assert self.project_json.data.get('engine', '') == engine_name

                assert set(self.project_json.data.get('user_tags', [])) == set(expected_tags)

                for gem in self.project_json.data.get('gem_names', []):
                    assert gem in expected_gem_names

                assert set(self.project_json.data.get('compatible_engines', [])) == set(expected_compatible_engines)
                assert set(self.project_json.data.get('engine_api_dependencies', [])) == set(expected_engine_api_dependencies)
            elif not project_path:
                assert not self.project_json.data
