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


TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
    "project_version" : "0.0.0.0",
    "compatible_engines" : [
        "o3de-sdk==2205.01"
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
    "external_subdirectories": [
        "D:/TestGem"
    ]
}
'''


@pytest.fixture(scope='class')
def init_project_json_data(request):
    class ProjectJsonData:
        def __init__(self):
            self.data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
    request.cls.project_json = ProjectJsonData()

@pytest.mark.usefixtures('init_project_json_data')
class TestEditProjectProperties:
    @pytest.mark.parametrize("project_path, project_name, project_new_name, project_id, project_origin,\
                              project_display, project_summary, project_icon, project_version, \
                              add_tags, delete_tags, replace_tags, expected_tags, \
                              add_gem_names, delete_gem_names, replace_gem_names, expected_gem_names, \
                              engine_name, \
                              add_compatible_engines, delete_compatible_engines, replace_compatible_engines, \
                              expected_compatible_engines, expected_result",  [
        pytest.param(pathlib.PurePath('E:/TestProject'),
                    'ProjNameA', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB'],
                    'NewEngineName',
                    'o3de>=1.0', 'o3de-sdk==2205.01', None, ['o3de>=1.0'],
                    0),
        pytest.param(pathlib.PurePath('D:/TestProject'),
                    'ProjNameA', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB'],
                    'o3de-sdk',
                    'c==4.3.2.1', None, 'a>=0.1 b==1.0,==2.0', ['a>=0.1', 'b==1.0,==2.0'],
                    0),
        pytest.param(pathlib.PurePath('D:/TestProject'),
                    'ProjNameA', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB'],
                    'o3de-install',
                    # removal uses exact matching, it doesn't support partial matches 
                    None, 'a>=0.1 b==1.0', None, ['b==1.0,==2.0'],
                    0),
        pytest.param(pathlib.PurePath('F:/TestProject'),
                    'ProjNameA', 'ProjNameB', 'ProjID', 'Origin', 
                    'Display', 'Summary', 'Icon', '1.0.0.0', 
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    'GemA GemB GemB', ['GemA'], None, ['GemB'],
                    None,
                    None, None, 'invalid', ['b==1.0,==2.0'], # invalid version
                    1),
        pytest.param('', # invalid path
                    'ProjNameA', 'ProjNameB', 'IDB', 'OriginB', 
                    'DisplayB', 'SummaryB', 'IconB', '1.0.0.0',
                    'A B C', 'B', 'D E F', ['D','E','F'],
                    ['GemA','GemB'], None, ['GemC'], ['GemC'],
                    None,
                    None, None, 'o3de-sdk==2205.1', ['o3de-sdk==2205.1'],
                    1)
        ]
    )
    def test_edit_project_properties(self, project_path, project_name, project_new_name, project_id, project_origin,
                                     project_display, project_summary, project_icon, project_version, 
                                     add_tags, delete_tags, replace_tags, expected_tags, expected_result,
                                     add_gem_names, delete_gem_names, replace_gem_names, expected_gem_names,
                                     engine_name,
                                     add_compatible_engines, delete_compatible_engines, replace_compatible_engines,
                                     expected_compatible_engines):

        def get_project_json_data(project_name: str, project_path) -> dict:
            if not project_path:
                self.project_json.data = None
                return None
            return self.project_json.data

        def save_o3de_manifest(new_proj_data: dict, project_path) -> bool:
            self.project_json.data = new_proj_data
            return True

        with patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_project_json_data_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch:
            result = project_properties.edit_project_props(project_path, project_name, project_new_name, project_id,
                                                           project_origin, project_display, project_summary, project_icon,
                                                           add_tags, delete_tags, replace_tags,
                                                           add_gem_names, delete_gem_names, replace_gem_names,
                                                           engine_name,
                                                           add_compatible_engines, delete_compatible_engines, replace_compatible_engines,
                                                           project_version)
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

                if engine_name:
                    assert self.project_json.data.get('engine', '') == engine_name

                assert set(self.project_json.data.get('user_tags', [])) == set(expected_tags)
                assert set(self.project_json.data.get('gem_names', [])) == set(expected_gem_names)
                assert set(self.project_json.data.get('compatible_engines', [])) == set(expected_compatible_engines)
            elif not project_path:
                assert not self.project_json.data
