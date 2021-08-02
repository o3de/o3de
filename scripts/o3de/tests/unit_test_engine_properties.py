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
    "FileVersion": 1,
    "O3DEVersion": "0.0.0.0",
    "O3DECopyrightYear": 2021,
    "O3DEBuildNumber": 0,
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


@pytest.fixture(scope='class')
def init_engine_json_data(request):
    class EngineJsonData:
        def __init__(self):
            self.data = json.loads(TEST_ENGINE_JSON_PAYLOAD)
    request.cls.engine_json = EngineJsonData()

@pytest.mark.usefixtures('init_engine_json_data')
class TestEditEngineProperties:
    @pytest.mark.parametrize("engine_path, engine_name, engine_new_name, engine_version, expected_result", [
        pytest.param(pathlib.PurePath('D:/o3de'), None, 'o3de-install', '1.0.0.0', 0),
        pytest.param(None, 'o3de-install', 'o3de-sdk', '1.0.0.1', 0),
        pytest.param(None, 'o3de-sdk', None, '2.0.0.0', 0),
        ]
    )
    def test_edit_engine_properties(self, engine_path, engine_name, engine_new_name, engine_version, expected_result):

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
            result = engine_properties.edit_engine_props(engine_path, engine_name, engine_new_name, engine_version)
            assert result == expected_result
            if engine_new_name:
                assert self.engine_json.data.get('engine_name', '') == engine_new_name
            if engine_version:
                assert self.engine_json.data.get('O3DEVersion', '') == engine_version
