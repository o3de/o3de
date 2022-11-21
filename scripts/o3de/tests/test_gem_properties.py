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

from o3de import gem_properties


TEST_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
    "display_name": "TestGem",
    "license": "Apache-2.0 or MIT",
    "license_url": "https://github.com/o3de/o3de/blob/development/LICENSE.txt",
    "origin": "The primary repo for TestGem goes here: i.e. http://www.mydomain.com",
    "summary": "A short description of TestGem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "TestGem"
    ],
    "platforms": [
    ],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "https://o3de.org/docs/",
    "dependencies": [
    ]
}
'''


@pytest.fixture(scope='class')
def init_gem_json_data(request):
    class GemJsonData:
        def __init__(self):
            self.data = json.loads(TEST_GEM_JSON_PAYLOAD)
    request.cls.gem_json = GemJsonData()

@pytest.mark.usefixtures('init_gem_json_data')
class TestEditGemProperties:
    @pytest.mark.parametrize("gem_path, gem_name, gem_new_name, gem_display, gem_origin,\
                            gem_type, gem_summary, gem_icon, gem_requirements, gem_documentation_url,\
                            gem_license, gem_license_url, gem_repo_uri, add_tags, remove_tags, replace_tags,\
                            expected_tags, add_platforms, remove_platforms, replace_platforms, expected_platforms,\
                            expected_result", [
        pytest.param(pathlib.PurePath('D:/TestProject'),
                     None, 'TestGem2', 'New Gem Name', 'O3DE', 'Code', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/',
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', "https://github.com/o3de/o3de.git",
                     ['Physics', 'Rendering', 'Scripting'], None, None, ['TestGem', 'Physics', 'Rendering', 'Scripting'],
                     ['Windows', 'MacOS', 'Linux'], None, None, ['Windows', 'MacOS', 'Linux'],
                     0),
        pytest.param(None,
                     'TestGem2', None, 'New Gem Name', 'O3DE', 'Asset', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/', 
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', None,
                     None, ['Physics'], None, ['TestGem', 'Rendering', 'Scripting'], 
                     ['Windows', 'MacOS'], ['Linux'], None, ['Windows', 'MacOS'],
                     0),
        pytest.param(None,
                     'TestGem2', None, 'New Gem Name', 'O3DE', 'Tool', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/',
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0',"https://github.com/o3de/o3de.git",
                     None, None, ['Animation', 'TestGem'], ['Animation', 'TestGem'],
                     ['Windows'], None, ['MacOS', 'Linux'], ['MacOS', 'Linux'],
                     0)
        ]
    )
    def test_edit_gem_properties(self, gem_path, gem_name, gem_new_name, gem_display, gem_origin,
                                    gem_type, gem_summary, gem_icon, gem_requirements, 
                                    gem_documentation_url, gem_license, gem_license_url, gem_repo_uri, add_tags, remove_tags,
                                    replace_tags, expected_tags, add_platforms, remove_platforms, replace_platforms,
                                    expected_platforms, expected_result):

        def get_gem_json_data(gem_path: pathlib.Path) -> dict:
            return self.gem_json.data

        def get_gem_path(gem_name: str) -> pathlib.Path:
            return pathlib.Path('D:/TestProject')

        def save_o3de_manifest(new_gem_data: dict, gem_path: pathlib.Path) -> bool:
            self.gem_json.data = new_gem_data
            return True

        with patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch, \
                patch('o3de.manifest.get_registered', side_effect=get_gem_path) as get_registered_patch:
            result = gem_properties.edit_gem_props(gem_path, gem_name, gem_new_name, gem_display, gem_origin,
                                                   gem_type, gem_summary, gem_icon, gem_requirements,
                                                   gem_documentation_url, gem_license, gem_license_url, gem_repo_uri,
                                                   add_tags, remove_tags, replace_tags,
                                                   add_platforms, remove_platforms, replace_platforms)
            assert result == expected_result
            if gem_new_name:
                assert self.gem_json.data.get('gem_name', '') == gem_new_name
            if gem_display:
                assert self.gem_json.data.get('display_name', '') == gem_display
            if gem_origin:
                assert self.gem_json.data.get('origin', '') == gem_origin
            if gem_type:
                assert self.gem_json.data.get('type', '') == gem_type
            if gem_summary:
                assert self.gem_json.data.get('summary', '') == gem_summary
            if gem_icon:
                assert self.gem_json.data.get('icon_path', '') == gem_icon
            if gem_requirements:
                assert self.gem_json.data.get('requirements', '') == gem_requirements
            if gem_documentation_url:
                assert self.gem_json.data.get('documentation_url', '') == gem_documentation_url
            if gem_license:
                assert self.gem_json.data.get('license', '') == gem_license
            if gem_license_url:
                assert self.gem_json.data.get('license_url', '') == gem_license_url
            if gem_repo_uri:
                assert self.gem_json.data.get('repo_uri', '') == gem_repo_uri

            assert set(self.gem_json.data.get('user_tags', [])) == set(expected_tags)

            assert set(self.gem_json.data.get('platforms', [])) == set(expected_platforms)
