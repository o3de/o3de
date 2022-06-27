#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import io
import json
import logging

import pytest
import pathlib
from unittest.mock import patch

from o3de import enable_gem


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
    ]
}
'''

TEST_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
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
    ],
    "engines_path": {
        "o3de": "D:/o3de/o3de"
    }
}
'''

@pytest.fixture(scope='class')
def init_enable_gem_data(request):
    class EnableGemData:
        def __init__(self):
            self.project_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            self.gem_data = json.loads(TEST_GEM_JSON_PAYLOAD)
    request.cls.enable_gem = EnableGemData()


@pytest.mark.usefixtures('init_enable_gem_data')
class TestEnableGemCommand:
    @pytest.mark.parametrize("gem_path, project_path, gem_registered_with_project, gem_registered_with_engine,"
                             "expected_result", [
        pytest.param(pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, True, 0),
        pytest.param(pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, False, 0),
        pytest.param(pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), True, False, 0),
        pytest.param(pathlib.PurePath('TestGem'), pathlib.PurePath('TestProject'), False, False, 0),
        ]
    )
    def test_enable_gem_registers_gem_name_with_project_json(self, gem_path, project_path, gem_registered_with_project,
                                                             gem_registered_with_engine, expected_result):

        def get_registered_path(project_name: str = None, gem_name: str = None) -> pathlib.Path:
            if project_name:
                return project_path
            elif gem_name:
                return gem_path
            return None

        def save_o3de_manifest(new_project_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == project_path / 'project.json':
                self.enable_gem.project_data = new_project_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            if not manifest_path:
                return json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            return None

        def get_project_json_data(project_name: str = None, project_path: pathlib.Path = None):
            return self.enable_gem.project_data

        def get_gem_json_data(gem_path: pathlib.Path, project_path: pathlib.Path):
            return self.enable_gem.gem_data

        def get_project_gems(project_path: pathlib.Path):
            return [pathlib.Path(gem_path).resolve()] if gem_registered_with_project else []

        def get_engine_gems():
            return [pathlib.Path(gem_path).resolve()] if gem_registered_with_engine else []

        def add_gem_dependency(enable_gem_cmake_file: pathlib.Path, gem_name: str):
            return 0

        with patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_patch,\
                patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_patch, \
                patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as load_o3de_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch,\
                patch('o3de.manifest.get_registered', side_effect=get_registered_path) as get_registered_patch,\
                patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_gems', side_effect=get_project_gems) as get_project_gems_patch,\
                patch('o3de.manifest.get_engine_gems', side_effect=get_engine_gems) as get_engine_gems_patch,\
                patch('o3de.cmake.add_gem_dependency', side_effect=add_gem_dependency) as add_gem_dependency_patch,\
                patch('o3de.validation.valid_o3de_gem_json', return_value=True) as valid_gem_json_patch:

            self.enable_gem.project_data.pop('gem_names', None)
            result = enable_gem.enable_gem_in_project(gem_path=gem_path, project_path=project_path)
            assert result == expected_result

            gem_json = get_gem_json_data(gem_path, project_path)
            project_json = get_project_json_data(project_path=project_path)
            if not gem_registered_with_engine and not gem_registered_with_project:
                assert gem_json.get('gem_name', '') in project_json.get('gem_names', [])
            else:
                assert gem_json.get('gem_name', '') not in project_json.get('gem_names', [])
