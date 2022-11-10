#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import io
import json
import pytest
import pathlib
import logging
from unittest.mock import patch

from o3de import manifest, utils


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

TEST_PROJECT_TEMPLATE_JSON_PAYLOAD = '''
{
    "template_name": "DefaultProject"
}
'''

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
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
    "engine": "o3de",
    "restricted_name": "projects",
    "external_subdirectories": [
        "D:/TestGem"
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
    def test_get_templates_for_project_creation(self, valid_project_json_paths, valid_gem_json_paths,
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
    def test_get_templates_for_gem_creation(self, valid_project_json_paths, valid_gem_json_paths,
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
            engine_payload['external_subdirectories'] = engine_external_subdirectories
            return engine_payload

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            manifest_payload = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            manifest_payload['external_subdirectories'] = manifest_external_subdirectories
            return manifest_payload

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
            project_path: pathlib.Path = None) -> dict or None:

            gem_payload = json.loads(TEST_GEM_JSON_PAYLOAD)

            if 'GemInGem' != gem_path.name:
                gem_payload["external_subdirectories"] = gem_external_subdirectories

            return gem_payload

        def is_file(path : pathlib.Path) -> bool:

            if path.match("*/NonGem/gem.json"):
                return False

            return True

        with patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) \
                as get_gem_json_data_patch,\
            patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) \
                as get_engine_json_data_patch,\
            patch('o3de.manifest.get_this_engine_path', side_effect=self.get_this_engine_path) \
                as get_this_engine_path_patch,\
            patch('pathlib.Path.is_file', new=is_file) \
                as pathlib_is_file_mock,\
            patch('pathlib.Path.resolve', new=self.resolve) \
                as pathlib_is_resolve_mock,\
            patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) \
                as load_o3de_manifest_patch:

            assert manifest.get_all_gems() == expected_gem_paths


    @pytest.mark.parametrize("""gem_external_subdirectories,
                                expected_cycle_detected""", [
            pytest.param(
                {
                    'Gems/Gem1':['../Gem2'],
                    'Gems/Gem2':['../Gem1']
                },
                True),
            pytest.param(
                {
                    'Gems/Gem1':['Gem2'],
                    'Gems/Gem1/Gem2':['Gem3','Gem4'],
                    'Gems/Gem1/Gem2/Gem3':[],
                    'Gems/Gem1/Gem2/Gem4':[]
                },
                False)
        ]
    )
    def test_get_gem_external_subdirectories_detects_cycles(self,
        gem_external_subdirectories,
        expected_cycle_detected ):

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
            project_path: pathlib.Path = None) -> dict or None:

            # Try to make path relative to the current working directory
            try:
                gem_rel_path = pathlib.Path(gem_path).relative_to(pathlib.Path.cwd())
            except ValueError:
                gem_rel_path = gem_path

            gem_payload = json.loads(TEST_GEM_JSON_PAYLOAD)
            gem_payload["external_subdirectories"] = gem_external_subdirectories[gem_rel_path.as_posix()]

            return gem_payload

        def cycle_detected(msg, *args, **kwargs):
            if 'cycle' in msg:
                self.cycle_detected = True

        with patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as _1, \
            patch('logging.Logger.warning', side_effect=cycle_detected) as _2, \
            patch('pathlib.Path.is_file', return_value=True) as _3:

            self.cycle_detected = False

            # start with the first path in the dictionary
            gem_path = pathlib.Path(list(gem_external_subdirectories.keys())[0])
            manifest.get_gem_external_subdirectories(gem_path, list())

            assert self.cycle_detected == expected_cycle_detected

class TestManifestGetRegistered:
    @staticmethod
    def get_this_engine_path() -> pathlib.Path:
        return pathlib.Path('D:/o3de/o3de')

    @staticmethod
    def is_file(self) -> bool:
        # use a simple suffix check to avoid hitting the actual file system
        return self.suffix != ''

    @staticmethod
    def resolve(self):
        return self

    @staticmethod
    def samefile(self, otherFile):
        return self.as_posix() == otherFile.as_posix()

    @pytest.mark.parametrize("template_name, relative_template_path, expected_path", [
            pytest.param('DefaultProject', pathlib.Path('Templates/DefaultProject'), pathlib.Path('D:/o3de/o3de/Templates/DefaultProject'), ),
            pytest.param('InvalidProject', pathlib.Path('Templates/DefaultProject'), None)
    ])
    def test_get_registered_template(self, template_name, relative_template_path, expected_path):
            def get_engine_json_data(engine_name: str = None,
                                    engine_path: str or pathlib.Path = None) -> dict or None:
                engine_payload = json.loads(TEST_ENGINE_JSON_PAYLOAD)
                if expected_path:
                    engine_payload['templates'] = [relative_template_path]
                return engine_payload

            def get_gem_json_data(gem_name: str = None,
                                    gem_path: str or pathlib.Path = None) -> dict or None:
                gem_payload = json.loads(TEST_GEM_JSON_PAYLOAD)
                return gem_payload

            def get_project_json_data(project_name: str = None,
                                    project_path: str or pathlib.Path = None) -> dict or None:
                project_payload = json.loads(TEST_PROJECT_JSON_PAYLOAD)
                return project_payload

            def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
                manifest_payload = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
                manifest_payload['projects'] = []
                return manifest_payload

            with patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as _1, \
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as _2, \
                patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as _3, \
                patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _4, \
                patch('pathlib.Path.resolve', self.resolve) as _5, \
                patch('pathlib.Path.samefile', self.samefile) as _6, \
                patch('pathlib.Path.open', return_value=io.StringIO(TEST_PROJECT_TEMPLATE_JSON_PAYLOAD)) as _7, \
                patch('pathlib.Path.is_file', self.is_file) as _8,\
                patch('o3de.manifest.get_this_engine_path', side_effect=self.get_this_engine_path) as _9: 

                path = manifest.get_registered(template_name=template_name)
                assert path == expected_path

class TestManifestProjects:
    @staticmethod
    def resolve(self):
        return self

    @staticmethod
    def samefile(self, otherFile ):
        return self.as_posix() == otherFile.as_posix()

    @pytest.mark.parametrize("project_path, project_engine_name, engines, expected_engine_path", [
            pytest.param(pathlib.Path('C:/project1'),
                'engine1',
                {'engine1': pathlib.Path('C:/engine1'), 'engine2': pathlib.Path('D:/engine2')},
                pathlib.Path('C:/engine1')),
            pytest.param(pathlib.Path('C:/project1'),
                'engine2',
                {'engine1': pathlib.Path('C:/engine1'), 'engine2': pathlib.Path('D:/engine2')},
                pathlib.Path('D:/engine2')),
            pytest.param(pathlib.Path('C:/engine1/project1'),
                '',
                {'engine1': pathlib.Path('C:/engine1'), 'engine2': pathlib.Path('D:/engine2')},
                pathlib.Path('C:/engine1')),
            pytest.param(pathlib.Path('C:/project1'),
                '',
                {'engine1': pathlib.Path('C:/engine1'), 'engine2': pathlib.Path('D:/engine2')},
                None),
        ]
    )
    def test_get_project_engines(self, project_path, project_engine_name,
                                    engines, expected_engine_path):
        def get_project_json_data(project_name: str = None,
            project_path: str or pathlib.Path = None) -> dict or None:
            project_json = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            project_json['engine'] = project_engine_name
            return project_json

        def get_registered(engine_name: str):
            return engines[engine_name]

        def get_manifest_engines():
            return list(engines.values())

        def find_ancestor_dir_containing_file(target_file_name: pathlib.PurePath, start_path: pathlib.Path,
                                            max_scan_up_range: int=0) -> pathlib.Path or None:
            for engine_path in engines.values():
                if engine_path in start_path.parents:
                    return engine_path
            return None

        def get_engine_projects(engine_path:pathlib.Path = None) -> list:
            return [project_path] if engine_path in project_path.parents else []

        with patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as _1, \
            patch('o3de.manifest.get_registered', side_effect=get_registered) as _2, \
            patch('o3de.manifest.get_manifest_engines', side_effect=get_manifest_engines) as _3, \
            patch('o3de.manifest.get_engine_projects', side_effect=get_engine_projects) as _4, \
            patch('o3de.utils.find_ancestor_dir_containing_file', side_effect=find_ancestor_dir_containing_file) as _5, \
            patch('pathlib.Path.resolve', self.resolve) as _6, \
            patch('pathlib.Path.samefile', self.samefile) as _7:

            engine_path = manifest.get_project_engine_path(project_path)
            assert engine_path == expected_engine_path
