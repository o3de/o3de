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

from o3de import manifest, disable_gem, enable_gem


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
def init_disable_gem_data(request):
    class DisableGemData:
        def __init__(self):
            self.project_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            self.gem_data = json.loads(TEST_GEM_JSON_PAYLOAD)
            self.engine_data = json.loads(TEST_ENGINE_JSON_PAYLOAD)
    request.cls.disable_gem = DisableGemData()


@pytest.mark.usefixtures('init_disable_gem_data')
class TestDisableGemCommand:
    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize("enable_gem_name, enable_gem_version, disable_gem_name, test_gem_path, "
                             "project_path, unregister_gem, gem_registered_with_project, "
                             "gem_registered_with_engine, enabled_in_cmake, expected_result", [
        pytest.param(None, None, None, pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, False, True, True, 0),
        pytest.param(None, None, None, pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, False, False, False, 0),
        pytest.param(None, None, None, pathlib.PurePath('TestProject/TestGem'), pathlib.PurePath('TestProject'), False, True, False, False, 0),
        pytest.param(None, None, None, pathlib.PurePath('TestGem'), pathlib.PurePath('TestProject'), False, False, False, False, 0),
        # when requested to remove by name with no version expect success
        pytest.param('TestGem', None, 'TestGem', None, pathlib.PurePath('TestProject'), False, False, False, True, 0),
        # when requested to remove a gem with matching version expect success
        pytest.param('TestGem==1.0.0', None, 'TestGem==1.0.0', None, pathlib.PurePath('TestProject'), False, False, False, False, 0),
        # when a gem specifier is included but the gem doesn't match, expect failure
        pytest.param('TestGem', 'None', 'TestGem==1.0.0', None, pathlib.PurePath('TestProject'), False, False, False, False, 2),
        # when a gem name with no specifier is provided, expect any gem with that name is removed 
        pytest.param('TestGem==1.2.3', '1.2.3', 'TestGem', None, pathlib.PurePath('TestProject'), False, False, False, False, 0),
        # a gem can be removed even if it isn't registered
        pytest.param('TestGem', None, 'TestGem', None, pathlib.PurePath('TestProject'), True, False, False, False, 0),
        ]
    )
    def test_disable_gem_registers_gem_name_with_project_json(self, enable_gem_name, enable_gem_version, disable_gem_name, 
                                                              test_gem_path, project_path, unregister_gem, gem_registered_with_project,
                                                              gem_registered_with_engine, enabled_in_cmake, expected_result):

        project_gem_dependencies = []
        default_gem_path = pathlib.PurePath('TestGem')
        self.gem_registered = True

        def get_registered(engine_name: str = None,
                        project_name: str = None,
                        gem_name: str = None,
                        template_name: str = None,
                        default_folder: str = None,
                        repo_name: str = None,
                        restricted_name: str = None,
                        project_path: pathlib.Path = None) -> pathlib.Path or None:
            if project_name:
                return project_path
            elif gem_name and self.gem_registered:
                return test_gem_path if test_gem_path else default_gem_path
            elif engine_name:
                return pathlib.PurePath('o3de')
            return None

        def save_o3de_manifest(new_project_data: dict, manifest_path: pathlib.Path = None) -> bool:
            if manifest_path == project_path / 'project.json':
                self.disable_gem.project_data = new_project_data
            return True

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict or None:
            if not manifest_path:
                return json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            return None

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            return self.disable_gem.project_data

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                            project_path: pathlib.Path = None) -> dict or None:
            gem_json_data = self.disable_gem.gem_data
            if enable_gem_version:
                gem_json_data['version'] = enable_gem_version
            return gem_json_data

        def get_engine_json_data(engine_name:str = None, engine_path:pathlib.Path = None):
            return self.disable_gem.engine_data

        def get_project_gems(project_path: pathlib.Path):
            gem_path = test_gem_path if test_gem_path else default_gem_path
            return [gem_path] if gem_registered_with_project else []

        def get_engine_gems():
            gem_path = test_gem_path if test_gem_path else default_gem_path
            return [gem_path] if gem_registered_with_engine else []

        def remove_gem_dependency(enable_gem_cmake_file: pathlib.Path, gem_name: str):
            if gem_name in project_gem_dependencies:
                project_gem_dependencies.remove(gem_name)
                return 0
            # If the gem was enabled in enabled_gems.cmake return an 
            # error because it wasn't found in the list of dependencies
            return 1 if enabled_in_cmake else 0

        def get_enabled_gems(enable_gem_cmake_file: pathlib.Path) -> list:
            return project_gem_dependencies

        def is_file(path : pathlib.Path) -> bool:
            if path.match("*/user/project.json"):
                return False
            return True

        with patch('pathlib.Path.is_dir', return_value=True) as pathlib_is_dir_patch,\
                patch('pathlib.Path.is_file', new=is_file) as pathlib_is_file_patch, \
                patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as load_o3de_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch,\
                patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch,\
                patch('o3de.manifest.get_registered', side_effect=get_registered) as get_registered_patch,\
                patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_gem_json_data_patch,\
                patch('o3de.manifest.get_project_gems', side_effect=get_project_gems) as get_project_gems_patch,\
                patch('o3de.manifest.get_engine_gems', side_effect=get_engine_gems) as get_engine_gems_patch,\
                patch('pathlib.Path.resolve', new=self.resolve) as pathlib_is_resolve_mock,\
                patch('o3de.cmake.remove_gem_dependency',
                      side_effect=remove_gem_dependency) as remove_gem_dependency_patch, \
                patch('o3de.manifest.get_enabled_gems',
                      side_effect=get_enabled_gems) as get_enabled_gems, \
                patch('o3de.validation.valid_o3de_gem_json', return_value=True) as valid_gem_json_patch:

            # Clear out any "gem_names" from the previous iterations
            self.disable_gem.project_data.pop('gem_names', None)

            # Enable the gem
            assert enable_gem.enable_gem_in_project(gem_name=enable_gem_name, gem_path=test_gem_path, project_path=project_path) == 0

            gem_json = get_gem_json_data(gem_name=enable_gem_name, gem_path=test_gem_path, project_path=project_path)
            expected_gem_name = enable_gem_name or gem_json['gem_name']

            if enabled_in_cmake:
                # Simulate the gem existing in the deprecated `enabled_gems.cmake` file
                project_gem_dependencies.append(expected_gem_name)

            # Check that the gem is enabled in project.json
            project_json = get_project_json_data(project_path=project_path)
            assert expected_gem_name in project_json.get('gem_names', [])

            # if unregister_gem is set, then set the gem_registered flag to false
            # to simulate that the gem is now unregistered so we can test disabling a gem
            # that is not registered 
            self.gem_registered = not unregister_gem

            # Disable the gem
            result = disable_gem.disable_gem_in_project(gem_name=disable_gem_name, gem_path=test_gem_path, project_path=project_path)
            assert result == expected_result

            expected_gem_name = disable_gem_name or expected_gem_name

            if enabled_in_cmake:
                # The gem name should no longer exist in enabled_gems.cmake 
                assert expected_gem_name not in manifest.get_enabled_gems(project_path / "Gem/enabled_gems.cmake")

            # The gem name should no longer appear in the "gem_names" field
            project_json = get_project_json_data(project_path=project_path)
            assert expected_gem_name not in project_json.get('gem_names', [])
