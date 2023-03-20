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
    "version": "0.0.0",
    "compatible_engines" : [
        "o3de-sdk==1.2.3"
    ],
    "engine_api_dependencies" : [
        "framework==1.2.3"
    ],
    "display_name": "TestGem",
    "license": "Apache-2.0 or MIT",
    "license_url": "https://github.com/o3de/o3de/blob/development/LICENSE.txt",
    "origin": "The primary repo for TestGem goes here: i.e. http://www.mydomain.com",
    "summary": "A short description of TestGem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "TestGem",
        "OtherTag"
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


@pytest.fixture(scope='function')
def init_gem_json_data(request):
    class GemJsonData:
        def __init__(self):
            self.data = json.loads(TEST_GEM_JSON_PAYLOAD)
    request.cls.gem_json = GemJsonData()

@pytest.mark.usefixtures('init_gem_json_data')
class TestEditGemProperties:
    @pytest.mark.parametrize("gem_path, gem_name, gem_new_name, gem_display, gem_origin,\
                            gem_type, gem_summary, gem_icon, gem_requirements, gem_documentation_url,\
                            gem_license, gem_license_url, gem_repo_uri, gem_version, add_compatible_engines, remove_compatible_engines, \
                            replace_compatible_engines, expected_compatible_engines, add_tags, remove_tags, replace_tags,\
                            expected_tags, add_platforms, remove_platforms, replace_platforms, expected_platforms,\
                            add_engine_api_dependencies, remove_engine_api_dependencies, replace_engine_api_dependencies,\
                            expected_engine_api_dependencies,\
                            expected_result", [
        pytest.param(pathlib.PurePath('D:/TestProject'),
                     None, 'TestGem2', 'New Gem Name', 'O3DE', 'Code', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/',
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', "https://github.com/o3de/o3de.git", '1.2.3', 
                     ['o3de>=1.0',"o3de-sdk~=1.0"], None, None, ['o3de-sdk==1.2.3','o3de>=1.0',"o3de-sdk~=1.0"],
                     ['Physics', 'Rendering', 'Scripting'], None, None, ['TestGem', 'OtherTag', 'Physics', 'Rendering', 'Scripting'],
                     ['Windows', 'MacOS', 'Linux'], None, None, ['Windows', 'MacOS', 'Linux'],
                     ['editor==2.3.4'], None, None, ['framework==1.2.3','editor==2.3.4'],
                     0),
        pytest.param(None,
                     'TestGem1', None, 'New Gem Name', 'O3DE', 'Asset', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/', 
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', None, '1.2.3', 
                     None, ['o3de-sdk==1.2.3'], None, [],
                     None, ['TestGem'], None, ['OtherTag'], 
                     ['Windows', 'MacOS'], ['Linux'], None, ['Windows', 'MacOS'],
                     ['launcher==3.4.5'], ['framework==1.2.3'], None, ['launcher==3.4.5'],
                     0),
        pytest.param(None,
                     'TestGem2', None, 'New Gem Name', 'O3DE', 'Tool', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/',
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', "https://github.com/o3de/o3de.git", '1.2.3', 
                     None, None, ['o3de>=1.2.3'], ['o3de>=1.2.3'],
                     None, None, ['Animation', 'TestGem'], ['Animation', 'TestGem'],
                     ['Windows'], None, ['MacOS', 'Linux'], ['MacOS', 'Linux'],
                     None, None, ['framework==9.8.7'], ['framework==9.8.7'],
                     0),
        pytest.param(None,
                     'TestGem3', None, 'New Gem Name', 'O3DE', 'Tool', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/',
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', "https://github.com/o3de/o3de.git",'1.2.3', 
                     None, None, ['INVALID'], [], # invalid version specifier
                     None, None, ['Animation', 'TestGem'], ['Animation', 'TestGem'],
                     ['Windows'], None, ['MacOS', 'Linux'], ['MacOS', 'Linux'],
                     None, None, None, ['framework==1.2.3'],
                     1),
        # can replace all existing tags with empty list
        pytest.param(None,
                     'TestGem4', None, 'New Gem Name', 'O3DE', 'Tool', 'Gem that exercises Default Gem Template',
                     'new_preview.png', 'Do this extra thing', 'https://o3de.org/docs/user-guide/gems/',
                     'Apache 2.0', 'https://www.apache.org/licenses/LICENSE-2.0', "https://github.com/o3de/o3de.git", '1.2.3', 
                     None, None, ['o3de>=1.2.3'], ['o3de>=1.2.3'],
                     None, None, [], [],
                     ['Windows'], None, ['MacOS', 'Linux'], ['MacOS', 'Linux'],
                     None, None, ['framework==9.8.7'], ['framework==9.8.7'],
                     0),
        ]
    )
    def test_edit_gem_properties(self, gem_path, gem_name, gem_new_name, gem_display, gem_origin,
                                    gem_type, gem_summary, gem_icon, gem_requirements, 
                                    gem_documentation_url, gem_license, gem_license_url, gem_repo_uri, 
                                    gem_version, add_compatible_engines, remove_compatible_engines,
                                    replace_compatible_engines, expected_compatible_engines,
                                    add_tags, remove_tags, replace_tags, expected_tags, 
                                    add_platforms, remove_platforms, replace_platforms, expected_platforms,
                                    add_engine_api_dependencies, remove_engine_api_dependencies, 
                                    replace_engine_api_dependencies, expected_engine_api_dependencies,
                                    expected_result):

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
                                                   gem_documentation_url, gem_license, gem_license_url, 
                                                   gem_version, add_compatible_engines, remove_compatible_engines,
                                                   replace_compatible_engines, gem_repo_uri,
                                                   add_tags, remove_tags, replace_tags,
                                                   add_platforms, remove_platforms, replace_platforms,
                                                   add_engine_api_dependencies, remove_engine_api_dependencies, replace_engine_api_dependencies)
            assert result == expected_result
            if expected_result == 0:
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
                if gem_version:
                    assert self.gem_json.data.get('version', '') == gem_version

                assert set(self.gem_json.data.get('compatible_engines', [])) == set(expected_compatible_engines)
                assert set(self.gem_json.data.get('user_tags', [])) == set(expected_tags)
                assert set(self.gem_json.data.get('platforms', [])) == set(expected_platforms)
                assert set(self.gem_json.data.get('engine_api_dependencies', [])) == set(expected_engine_api_dependencies)
