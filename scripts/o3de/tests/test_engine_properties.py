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

from o3de import engine_properties


TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "restricted_name": "o3de",
    "version": "0.0.0",
    "api_versions": {
        "api":"1.2.3"
    },
    "file_version": 1,
    "O3DEVersion": "0.0.0.0",
    "copyright_year": 2021,
    "build": 0,
    "external_subdirectories": [
        "Gems/TestGem2"
    ],
    "projects": [
    ],
    "templates": [
        "Templates/MinimalProject"
    ],
    "gem_names":[
        "TestGem"
    ]
}
'''


@pytest.fixture(scope='function')
def init_engine_json_data(request):
    class EngineJsonData:
        def __init__(self):
            self.data = json.loads(TEST_ENGINE_JSON_PAYLOAD)
    request.cls.engine_json = EngineJsonData()

@pytest.mark.usefixtures('init_engine_json_data')
class TestEditEngineProperties:
    @pytest.mark.parametrize("engine_path, engine_name, engine_new_name, engine_version, engine_display_version,"\
            "add_gem_names, delete_gem_names, replace_gem_names, expected_gem_names,"\
            "add_api_versions, delete_api_versions, replace_api_versions, expected_api_versions,"\
            "expected_result", [
        pytest.param(pathlib.PurePath('D:/o3de'), None, 'o3de-install', '1.0.0.0', '0.0.0',
            ['TestGem2'], None, None, ['TestGem','TestGem2'],
            ['api2=2.3.4'], None, None, {'api':'1.2.3','api2':'2.3.4'},
            0),
        pytest.param(None, 'o3de-install', 'o3de-sdk', '1.0.0.1', '0.0.0',
            None, ['TestGem'], None, [],
            None, ['api'], None, {},
            0),
        pytest.param(None, 'o3de-sdk', None, '2.0.0.0', '0.0.0',
            None, None, ['TestGem3','TestGem4'], ['TestGem3','TestGem4'],
            None, None, ['api3=3.4.5'], {'api3':'3.4.5'},
            0),
        # invalid apis are ignored
        pytest.param(None, 'o3de-sdk', None, '2.0.0.0', '0.0.0',
            None, None, ['TestGem3','TestGem4'], ['TestGem3','TestGem4'],
            ['api3'], None, None, {'api':'1.2.3'},
            0),
        ]
    )
    def test_edit_engine_properties(self, engine_path, engine_name, engine_new_name, 
        engine_version, engine_display_version,
        add_gem_names, delete_gem_names, replace_gem_names, expected_gem_names,
        add_api_versions, delete_api_versions, replace_api_versions, expected_api_versions,
        expected_result):

        def get_engine_json_data(engine_path: pathlib.Path) -> dict:
            return self.engine_json.data

        def get_engine_path(engine_name: str) -> pathlib.Path:
            return pathlib.Path('D:/o3de')

        def save_o3de_manifest(new_engine_data: dict, engine_path: pathlib.Path) -> bool:
            self.engine_json.data = new_engine_data
            return True

        with patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch, \
                patch('o3de.manifest.get_registered', side_effect=get_engine_path) as get_registered_patch:
            result = engine_properties.edit_engine_props(engine_path, engine_name, engine_new_name, engine_version,
                engine_display_version, add_gem_names, delete_gem_names, replace_gem_names,
                add_api_versions, delete_api_versions, replace_api_versions)
            assert result == expected_result
            if result == 0:
                if engine_new_name:
                    assert self.engine_json.data.get('engine_name', '') == engine_new_name
                if engine_version:
                    assert self.engine_json.data.get('O3DEVersion', '') == engine_version
                    assert self.engine_json.data.get('version', '') == engine_version
                if isinstance(engine_display_version, str):
                    assert self.engine_json.data.get('display_version', '') == engine_display_version

                assert set(self.engine_json.data.get('gem_names', [])) == set(expected_gem_names)
                assert self.engine_json.data.get('api_versions', {}) == expected_api_versions
