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

from o3de import manifest


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
    ],
    "external_subdirectories": [
    ]
}
'''

TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "external_subdirectories": [
        "GemInEngine"
    ],
    "projects": [],
    "templates": []
}
'''

TEST_O3DE_MANIFEST_JSON_PAYLOAD = '''
{
    "o3de_manifest_name": "testuser",
    "origin": "C:/Users/testuser/.o3de",
    "default_engines_folder": "C:/Users/testuser/.o3de/Engines",
    "default_projects_folder": "C:/Users/testuser/.o3de/Projects",
    "default_gems_folder": "C:/Users/testuser/.o3de/Gems",
    "default_templates_folder": "C:/Users/testuser/.o3de/Templates",
    "default_restricted_folder": "C:/Users/testuser/.o3de/Restricted",
    "default_third_party_folder": "C:/Users/testuser/.o3de/3rdParty",
    "projects": [
        "D:/MinimalProject"
    ],
    "external_subdirectories": [
        "D:/GemOutsideEngine"
    ],
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

@pytest.mark.parametrize("valid_project_json_paths, valid_gem_json_paths", [
    pytest.param([pathlib.Path('D:/o3de/Templates/DefaultProject/Template/project.json')],
                 [pathlib.Path('D:/o3de/Templates/DefaultGem/Template/gem.json')])
])
class TestGetTemplatesForCreation:
    @staticmethod
    def get_manifest_templates() -> list:
        return []

    @staticmethod
    def get_project_templates() -> list:
        return []

    @staticmethod
    def get_gem_templates() -> list:
        return []

    @staticmethod
    def get_engine_templates() -> list:
        return [pathlib.Path('D:/o3de/Templates/DefaultProject'), pathlib.Path('D:/o3de/Templates/DefaultGem')]

    @staticmethod
    def get_all_gems(project_path: pathlib.Path = None) -> list:
        return []

    @pytest.mark.parametrize("expected_template_paths", [
            pytest.param([])
        ]
    )
    def test_get_templates_for_generic_creation(self, valid_project_json_paths, valid_gem_json_paths,
                                                expected_template_paths):
        def validate_project_json(project_json_path) -> bool:
            return pathlib.Path(project_json_path) in valid_project_json_paths

        def validate_gem_json(gem_json_path) -> bool:
            return pathlib.Path(gem_json_path) in valid_gem_json_paths

        with patch('o3de.manifest.get_manifest_templates', side_effect=self.get_manifest_templates)\
                        as get_manifest_templates_patch, \
                patch('o3de.manifest.get_project_templates', side_effect=self.get_project_templates)\
                        as get_project_templates_patch, \
                patch('o3de.manifest.get_gem_templates', side_effect=self.get_gem_templates) \
                        as get_gem_templates_patch, \
                patch('o3de.manifest.get_engine_templates', side_effect=self.get_engine_templates)\
                        as get_engine_templates_patch, \
                patch('o3de.manifest.get_all_gems', side_effect=self.get_all_gems) \
                        as get_all_gems_patch, \
                patch('o3de.validation.valid_o3de_template_json', return_value=True) \
                        as validate_template_json,\
                patch('o3de.validation.valid_o3de_project_json', side_effect=validate_project_json) \
                        as validate_project_json,\
                patch('o3de.validation.valid_o3de_gem_json', side_effect=validate_gem_json) \
                        as validate_gem_json,\
                patch('o3de.manifest.load_o3de_manifest') as load_o3de_manifest_patch:
            templates = manifest.get_templates_for_generic_creation()
            assert templates == expected_template_paths

            # make sure the o3de manifest isn't attempted to be loaded
            load_o3de_manifest_patch.assert_not_called()


    @pytest.mark.parametrize("expected_template_paths", [
            pytest.param([pathlib.Path('D:/o3de/Templates/DefaultProject')])
        ]
    )
    def test_get_templates_for_gem_creation(self, valid_project_json_paths, valid_gem_json_paths,
                                                expected_template_paths):
        def validate_project_json(project_json_path) -> bool:
            return pathlib.Path(project_json_path) in valid_project_json_paths

        def validate_gem_json(gem_json_path) -> bool:
            return pathlib.Path(gem_json_path) in valid_gem_json_paths

        with patch('o3de.manifest.get_manifest_templates', side_effect=self.get_manifest_templates)\
                        as get_manifest_templates_patch, \
                patch('o3de.manifest.get_project_templates', side_effect=self.get_project_templates) \
                        as get_project_templates_patch, \
                patch('o3de.manifest.get_gem_templates', side_effect=self.get_gem_templates) \
                        as get_gem_templates_patch, \
                patch('o3de.manifest.get_engine_templates', side_effect=self.get_engine_templates) \
                        as get_engine_templates_patch, \
                patch('o3de.manifest.get_all_gems', side_effect=self.get_all_gems) \
                        as get_all_gems_patch, \
                patch('o3de.validation.valid_o3de_template_json', return_value=True) \
                        as validate_template_json, \
                patch('o3de.validation.valid_o3de_project_json', side_effect=validate_project_json) \
                        as validate_project_json, \
                patch('o3de.validation.valid_o3de_gem_json', side_effect=validate_gem_json) \
                        as validate_gem_json,\
                patch('o3de.manifest.load_o3de_manifest') as load_o3de_manifest_patch:
            templates = manifest.get_templates_for_project_creation()
            assert templates == expected_template_paths

            # make sure the o3de manifest isn't attempted to be loaded
            load_o3de_manifest_patch.assert_not_called()


    @pytest.mark.parametrize("expected_template_paths", [
            pytest.param([pathlib.Path('D:/o3de/Templates/DefaultGem')])
        ]
    )
    def test_get_templates_for_project_creation(self, valid_project_json_paths, valid_gem_json_paths,
                                                expected_template_paths):
        def validate_project_json(project_json_path) -> bool:
            return pathlib.Path(project_json_path) in valid_project_json_paths

        def validate_gem_json(gem_json_path) -> bool:
            return pathlib.Path(gem_json_path) in valid_gem_json_paths

        with patch('o3de.manifest.get_manifest_templates', side_effect=self.get_manifest_templates) \
                        as get_manifest_templates_patch, \
                patch('o3de.manifest.get_project_templates', side_effect=self.get_project_templates) \
                        as get_project_templates_patch, \
                patch('o3de.manifest.get_gem_templates', side_effect=self.get_gem_templates) \
                        as get_gem_templates_patch, \
                patch('o3de.manifest.get_engine_templates', side_effect=self.get_engine_templates) \
                        as get_engine_templates_patch, \
                patch('o3de.manifest.get_all_gems', side_effect=self.get_all_gems) \
                        as get_all_gems_patch, \
                patch('o3de.validation.valid_o3de_template_json', return_value=True) \
                        as validate_template_json, \
                patch('o3de.validation.valid_o3de_project_json', side_effect=validate_project_json) \
                        as validate_project_json, \
                patch('o3de.validation.valid_o3de_gem_json', side_effect=validate_gem_json) \
                        as validate_gem_json, \
                patch('o3de.manifest.load_o3de_manifest') as load_o3de_manifest_patch:
            templates = manifest.get_templates_for_gem_creation()
            assert templates == expected_template_paths

            # make sure the o3de manifest isn't attempted to be loaded
            load_o3de_manifest_patch.assert_not_called()


class TestGetAllGems:
    @staticmethod
    def get_this_engine_path() -> pathlib.Path:
        return pathlib.Path('D:/o3de/o3de')

    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize("""manifest_external_subdirectories, 
                                engine_external_subdirectories, 
                                gem_external_subdirectories,
                                expected_gem_paths""", [
            pytest.param(['D:/GemOutsideEngine'], 
                ['GemInEngine'], 
                ['GemInGem'],
                ['D:/GemOutsideEngine', 'D:/o3de/o3de/GemInEngine',
                'D:/GemOutsideEngine/GemInGem', 'D:/o3de/o3de/GemInEngine/GemInGem']),
            pytest.param(['D:/GemOutsideEngine','D:/GemOutsideEngine','D:/NonGem'],
                ['GemInEngine','GemInEngine'],
                ['GemInGem','GemInGem'],
                ['D:/GemOutsideEngine', 'D:/o3de/o3de/GemInEngine',
                'D:/GemOutsideEngine/GemInGem', 'D:/o3de/o3de/GemInEngine/GemInGem']),
        ]
    )
    def test_get_all_gems_returns_unique_paths(self, manifest_external_subdirectories, 
        engine_external_subdirectories, gem_external_subdirectories,
        expected_gem_paths ):

        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None) -> dict or None:
            engine_payload = json.loads(TEST_ENGINE_JSON_PAYLOAD)
            engine_payload['external_subdirectores'] = engine_external_subdirectories
            return engine_payload

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            manifest_payload = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            manifest_payload['external_subdirectores'] = manifest_external_subdirectories
            return manifest_payload

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
            project_path: pathlib.Path = None) -> dict or None:

            if 'NonGem' in gem_path:
                return None

            gem_payload = json.loads(TEST_GEM_JSON_PAYLOAD)

            if not 'GemInGem' in gem_path:
                gem_payload["external_subdirectories"] = gem_external_subdirectories

            return gem_payload

        with patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) \
                as get_gem_json_data_patch,\
            patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) \
                as get_engine_json_data_patch,\
            patch('o3de.manifest.get_this_engine_path', side_effect=self.get_this_engine_path) \
                as get_this_engine_path_patch,\
            patch('pathlib.Path.is_file', return_value=True) \
                as pathlib_is_file_mock,\
            patch('pathlib.Path.resolve', self.resolve) \
                as pathlib_is_resolve_mock,\
            patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) \
                as load_o3de_manifest_patch:

            assert manifest.get_all_gems() == expected_gem_paths
