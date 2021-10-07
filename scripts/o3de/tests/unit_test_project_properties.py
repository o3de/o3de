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
    @pytest.mark.parametrize("project_path, project_name, project_new_name, project_origin, project_display,\
                            project_summary, project_icon, add_tags, delete_tags,\
                            replace_tags, expected_result", [
        pytest.param(pathlib.PurePath('E:/TestProject'),
        'test', 'test', 'editing by pytest', 'Unit Test', 'pyTest project', 'pytest.bmp', 'A B C',
        'B', 'D E F', 0),
        pytest.param('',
        'test', 'test', 'editing by pytest', 'Unit Test', 'pyTest project', 'pytest.bmp', 'A B C',
        'B', 'D E F', 1)
        ]
    )
    def test_edit_project_properties(self, project_path, project_name, project_new_name, project_origin, project_display,
                                    project_summary, project_icon, add_tags, delete_tags,
                                    replace_tags, expected_result):

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
            result = project_properties.edit_project_props(project_path, project_name, project_new_name, project_origin,
                                                            project_display, project_summary, project_icon,
                                                            add_tags, delete_tags, replace_tags)
            assert result == expected_result
            if project_path:
                assert self.project_json.data
                assert self.project_json.data.get('origin', '') == project_origin
                assert self.project_json.data.get('display_name', '') == project_display
                assert self.project_json.data.get('summary', '') == project_summary
                assert self.project_json.data.get('icon_path', '') == project_icon
                expected_tag_set = set(replace_tags.split())
                project_json_tag_set = set(self.project_json.data.get('user_tags', []))
                assert project_json_tag_set == expected_tag_set
            else:
                assert not self.project_json.data
