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
    "version": "0.0.0",
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
    ]
}
'''

class TestGetEnabledGems:
    @pytest.mark.parametrize(
        "enable_gems_cmake_data, expected_set", [
            pytest.param("""
                # Comment
                set(ENABLED_GEMS foo bar baz)
            """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo
                            bar
                            baz
                        )
                    """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                    # Comment
                    set(ENABLED_GEMS
                        foo
                        bar
                        baz)
                """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            baz)
                    """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                        # Comment
                        set(RANDOM_VARIABLE TestGame, TestProject Test Engine)
                        set(ENABLED_GEMS HelloWorld IceCream
                            foo
                            baz bar
                            baz baz baz baz baz morebaz lessbaz
                        )
                        Random Text
                    """, set(['HelloWorld', 'IceCream', 'foo', 'bar', 'baz', 'morebaz', 'lessbaz'])),
        ]
    )
    def test_get_enabled_gems(self, enable_gems_cmake_data, expected_set):
        enabled_gems_set = set()
        with patch('pathlib.Path.resolve', return_value=pathlib.Path('enabled_gems.cmake')) as pathlib_is_resolve_mock,\
                patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_mock,\
                patch('pathlib.Path.open', return_value=io.StringIO(enable_gems_cmake_data)) as pathlib_open_mock:
            enabled_gems_set = manifest.get_enabled_gems(pathlib.Path('enabled_gems.cmake'))

        assert enabled_gems_set == expected_set


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
            manifest.get_gem_external_subdirectories(gem_path, list(), dict())

            assert self.cycle_detected == expected_cycle_detected

class TestGetProjectEnabledGems:

    project_path = pathlib.Path("project")

    @staticmethod
    def get_this_engine_path() -> pathlib.Path:
        return pathlib.Path('D:/o3de/o3de')

    @staticmethod
    def resolve(self):
        return self

    @staticmethod
    def as_posix(self):
        return self

    @pytest.mark.parametrize("""project_gem_names, cmake_gem_names, 
                                all_gems_json_data, include_dependencies,
                                expected_result""", [
            # When gems are provided without version specifiers expect they are found
            pytest.param(
                ['GemA'], ['GemB'],
                {
                    'GemA':[{'gem_name':'GemA','path':pathlib.Path('c:/GemA')}],
                    'GemB':[{'gem_name':'GemB','path':pathlib.Path('c:/GemB')}]
                },
                True,
                {'GemA':'c:/GemA', 'GemB':'c:/GemB'}
            ),
            # When dependencies exist they are not included if include_dependencies is False 
            pytest.param(
                ['GemA'], ['GemB'],
                {
                    'GemA':[{'gem_name':'GemA','path':pathlib.Path('c:/GemA'), 'dependencies':['GemC']}],
                    'GemB':[{'gem_name':'GemB','path':pathlib.Path('c:/GemB')}],
                    'GemC':[{'gem_name':'GemC','path':pathlib.Path('c:/GemC')}]
                },
                False,
                {'GemA':'c:/GemA', 'GemB':'c:/GemB'}
            ),
            # When dependencies exist they are included if include_dependencies is True 
            pytest.param(
                ['GemA'], ['GemB'],
                {
                    'GemA':[{'gem_name':'GemA','path':pathlib.Path('c:/GemA'), 'dependencies':['GemC']}],
                    'GemB':[{'gem_name':'GemB','path':pathlib.Path('c:/GemB')}],
                    'GemC':[{'gem_name':'GemC','path':pathlib.Path('c:/GemC')}]
                },
                True,
                {'GemA':'c:/GemA', 'GemB':'c:/GemB', 'GemC':'c:/GemC'}
            ),
            # When a mix of gems are provided with and without version specifiers expect they are found
            pytest.param(
                ['GemA>=1.0.0'], ['GemB'],
                {
                    'GemA':[
                        {'gem_name':'GemA','version':'1.0.0', 'path':pathlib.Path('c:/GemA1')},
                        {'gem_name':'GemA','version':'2.0.0', 'path':pathlib.Path('c:/GemA2')}
                    ],
                    'GemB':[{'gem_name':'GemB','path':pathlib.Path('c:/GemB')}]
                },
                True,
                {'GemA>=1.0.0':'c:/GemA2', 'GemB':'c:/GemB'}
            ),
            # When no gems are installed expect the names are returned without paths
            pytest.param(
                ['GemA>=1.0.0'], ['GemB==2.0.0'],
                {},
                True,
                {'GemA>=1.0.0':None, 'GemB==2.0.0':None}
            ),
            # When some gems are missing expect their paths are none 
            pytest.param(
                ['GemA<3.0.0'], ['GemB==2.0.0'],
                {
                    'GemA':[
                        {'gem_name':'GemA','version':'2.0.0','path':pathlib.Path('c:/GemA2')}, # <-- correct
                        {'gem_name':'GemA','version':'3.0.0','path':pathlib.Path('c:/GemA3')},
                    ]
                    # no GemB
                },
                True,
                {'GemA<3.0.0':'c:/GemA2', 'GemB==2.0.0':None}
            ),
        ]
    )
    def test_get_project_enabled_gems(self, project_gem_names, cmake_gem_names, 
                                      all_gems_json_data, include_dependencies, expected_result):
        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            project_json_data = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            project_json_data['gem_names'] = project_gem_names
            return project_json_data
        
        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None) -> dict or None:
            return json.loads(TEST_ENGINE_JSON_PAYLOAD)

        def get_enabled_gem_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                platform: str = 'Common'):
            return None if not cmake_gem_names else pathlib.Path('enabled_gems.cmake')

        def get_enabled_gems(cmake_file: pathlib.Path) -> set:
            return cmake_gem_names 

        def get_gems_json_data_by_name( engine_path:pathlib.Path = None, 
                                        project_path: pathlib.Path = None, 
                                        include_manifest_gems: bool = False,
                                        include_engine_gems: bool = False,
                                        external_subdirectories: list = None
                                        ) -> dict:
            return all_gems_json_data

        def get_project_engine_path(project_path: pathlib.Path) -> pathlib.Path or None:
            return None

        with patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) \
                as get_engine_json_data_patch,\
            patch('o3de.manifest.get_gems_json_data_by_name', side_effect=get_gems_json_data_by_name) \
                as get_gems_json_data_by_name_patch,\
            patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) \
                as get_project_json_data_patch, \
            patch('o3de.manifest.get_project_engine_path', side_effect=get_project_engine_path) \
                as get_project_engine_path_patch,\
            patch('o3de.manifest.get_enabled_gem_cmake_file', side_effect=get_enabled_gem_cmake_file) \
                as get_enabled_gem_cmake_file_patch, \
            patch('o3de.manifest.get_enabled_gems', side_effect=get_enabled_gems) as get_enabled_gems_patch, \
            patch('pathlib.Path.resolve', self.resolve) as resolve_patch, \
            patch('pathlib.Path.is_file', return_value=True) as is_file_patch:

            assert expected_result == manifest.get_project_enabled_gems(self.project_path, include_dependencies)

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
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
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

@pytest.mark.parametrize("test_object_typename", [
        pytest.param('engine'),
        pytest.param('project'),
        pytest.param('gem')
    ])
class TestManifestGetRegisteredVersionedObject:
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

    @pytest.mark.parametrize("object_name, json_data_by_path, expected_path", [
            # when no version information exists and name matches expect object not found
            pytest.param('object-name', {pathlib.PurePath('object1'):{"name":"object-name"}}, pathlib.PurePath('object1'), ),
            # when version information exists and version specifier provided expect correct engine found
            pytest.param('object-name==1.0.0', {
                pathlib.PurePath('object1'):{"name":"object-name","version":"1.0.0"},
                pathlib.PurePath('object2'):{"name":"object-name","version":"2.0.0"}
                }, pathlib.PurePath('object1'), ),
            # when version information exists and version specifier provided expect correct engine found
            pytest.param('object-name>=1.0.0', {
                pathlib.PurePath('object1'):{"name":"object-name","version":"1.0.0"},
                pathlib.PurePath('object2'):{"name":"object-name","version":"2.0.0"}
                }, pathlib.PurePath('object2'), ),
            # when version information exists and version specifier matches multiple engines, expect first match returned 
            pytest.param('object-name==1.0.0', {
                pathlib.PurePath('object1'):{"name":"object-name","version":"1.0.0"},
                pathlib.PurePath('object2'):{"name":"object-name","version":"1.0.0"}
                }, pathlib.PurePath('object1'), ),
            # when engine is not found expect none is returned 
            pytest.param('object-missing', {pathlib.PurePath('object1'):{"name":"object-name"}}, None ),
    ])
    def test_get_registered_object_with_version(self, test_object_typename, object_name, json_data_by_path, expected_path):
        def get_json_data(object_typename: str,
                        object_path: str or pathlib.Path,
                        object_validator: callable) -> dict or None:
            if object_typename == test_object_typename:
                if test_object_typename == 'engine':
                    payload = json.loads(TEST_ENGINE_JSON_PAYLOAD)
                elif test_object_typename == 'project':
                    payload = json.loads(TEST_PROJECT_JSON_PAYLOAD)
                elif test_object_typename == 'gem':
                    payload = json.loads(TEST_GEM_JSON_PAYLOAD)
                object_path = pathlib.PurePath(object_path)
                payload.update(json_data_by_path.get(object_path, {}))
                # change '<object>_name' field value set in 'name' 
                # e.g. 'engine_name' or 'project_name' gets value from 'name'
                if 'name' in payload:
                    payload[object_typename + '_name'] = payload['name']
                return payload

        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None) -> dict or None:
            return get_json_data('engine', engine_path, None)

        def get_gem_json_data(gem_name: str = None,
                            gem_path: str or pathlib.Path = None) -> dict or None:
            return get_json_data('gem', gem_path, None)

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            return get_json_data('project', project_path, None)

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            manifest_payload = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
            if test_object_typename == 'gem':
                manifest_payload['external_subdirectories'] = [p.as_posix() for p in json_data_by_path.keys()]
            else:
                manifest_payload[test_object_typename + 's'] = [p.as_posix() for p in json_data_by_path.keys()]
            return manifest_payload

        with patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as _1, \
            patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as _2, \
            patch('o3de.manifest.get_json_data', side_effect=get_json_data) as _2, \
            patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as _3, \
            patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _4, \
            patch('pathlib.Path.resolve', self.resolve) as _5, \
            patch('pathlib.Path.is_file', self.is_file) as _8,\
            patch('o3de.manifest.get_this_engine_path', side_effect=self.get_this_engine_path) as _9: 

            engine_name = object_name if test_object_typename == 'engine' else None
            project_name = object_name if test_object_typename == 'project' else None
            gem_name = object_name if test_object_typename == 'gem' else None

            path = manifest.get_registered(engine_name=engine_name, project_name=project_name, gem_name=gem_name)
            assert path == expected_path

class TestManifestProjects:
    @staticmethod
    def resolve(self):
        return self

    @staticmethod
    def samefile(self, otherFile ):
        return self.as_posix() == otherFile.as_posix()

    @pytest.mark.parametrize("project_path, project_engine, user_project_json, engines, engines_json_data, \
                expected_engine_path", [
            pytest.param(pathlib.Path('C:/project1'), 'engine1', None,
                [pathlib.Path('C:/engine1'),pathlib.Path('D:/engine2')],
                [{'engine_name':'engine1'},{'engine_name':'engine2'}],
                pathlib.Path('C:/engine1')),
            pytest.param(pathlib.Path('C:/project1'), 'engine2', {},
                [pathlib.Path('C:/engine1'),pathlib.Path('D:/engine2')],
                [{'engine_name':'engine1'},{'engine_name':'engine2'}],
                pathlib.Path('D:/engine2')),
            pytest.param(pathlib.Path('C:/engine1/project1'), '', {},
                [pathlib.Path('C:/engine1'),pathlib.Path('D:/engine2')],
                [{'engine_name':'engine1'},{'engine_name':'engine2'}],
                pathlib.Path('C:/engine1')),
            pytest.param(pathlib.Path('C:/project1'), '', {},
                [pathlib.Path('C:/engine1'),pathlib.Path('D:/engine2')],
                [{'engine_name':'engine1'},{'engine_name':'engine2'}],
                None),
            # When multiple engines with the same name and version exist, the first is
            # chosen
            pytest.param(pathlib.Path('C:/project1'), 'o3de', {},
                [pathlib.Path('C:/o3de1'),pathlib.Path('D:/o3de2')],
                [{'engine_name':'o3de','version':'1.0.0'},{'engine_name':'o3de','version':'1.0.0'}],
                pathlib.Path('C:/o3de1')),
            # When multiple engines with the same name and version exist, and the user
            # sets a local engine_path, the engine is chosen based on engine_path
            pytest.param(pathlib.Path('C:/project1'), 'o3de', {'engine_path':pathlib.Path('D:/o3de2')},
                [pathlib.Path('C:/o3de1'),pathlib.Path('D:/o3de2')],
                [{'engine_name':'o3de','version':'1.0.0'},{'engine_name':'o3de','version':'1.0.0'}],
                pathlib.Path('D:/o3de2')),
            # When multiple engines with the same name and different versions exist, 
            # the engine with the highest compatible version is chosen
            pytest.param(pathlib.Path('C:/project1'), 'o3de', {},
                [pathlib.Path('C:/o3de1'),pathlib.Path('D:/o3de2')],
                [{'engine_name':'o3de','version':'1.0.0'},{'engine_name':'o3de','version':'2.0.0'}],
                pathlib.Path('D:/o3de2')),
            # When no engines with the same name and compatible versions exists, 
            # no engine path is returned
            pytest.param(pathlib.Path('C:/project1'), 'o3de==1.5.0', {},
                [pathlib.Path('C:/o3de1'),pathlib.Path('D:/o3de2')],
                [{'engine_name':'o3de','version':'1.0.0'},{'engine_name':'o3de','version':'2.0.0'}],
                None),
        ]
    )
    def test_get_project_engines(self, project_path, project_engine, user_project_json,
                                    engines, engines_json_data, expected_engine_path):
        def get_engine_json_data(engine_name: str = None,
                                engine_path: str or pathlib.Path = None) -> dict or None:
            engine_payload = json.loads(TEST_ENGINE_JSON_PAYLOAD)
            for i in range(len(engines)):
                if engines[i] == engine_path:
                    engine_payload.update(engines_json_data[i])
            return engine_payload

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            project_json = json.loads(TEST_PROJECT_JSON_PAYLOAD)
            project_json['engine'] = project_engine
            return project_json

        def get_json_data_file(object_json: pathlib.Path,
                            object_typename: str,
                            object_validator: callable) -> dict or None:
            return user_project_json 

        def get_registered(engine_name: str):
            return engines[engine_name]

        def get_manifest_engines():
            return engines

        def find_ancestor_dir_containing_file(target_file_name: pathlib.PurePath, start_path: pathlib.Path,
                                            max_scan_up_range: int=0) -> pathlib.Path or None:
            for engine_path in engines:
                if engine_path in start_path.parents:
                    return engine_path
            return None

        def get_engine_projects(engine_path:pathlib.Path = None) -> list:
            return [project_path] if engine_path in project_path.parents else []

        def is_file(path : pathlib.Path) -> bool:
            return True if user_project_json else False

        with patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as _1, \
            patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch, \
            patch('o3de.manifest.get_json_data_file', side_effect=get_json_data_file) as get_json_data_file_patch, \
            patch('o3de.manifest.get_registered', side_effect=get_registered) as _2, \
            patch('o3de.manifest.get_manifest_engines', side_effect=get_manifest_engines) as _3, \
            patch('o3de.manifest.get_engine_projects', side_effect=get_engine_projects) as _4, \
            patch('o3de.utils.find_ancestor_dir_containing_file', side_effect=find_ancestor_dir_containing_file) as _5, \
            patch('pathlib.Path.is_file', new=is_file) as pathlib_is_file_mock,\
            patch('pathlib.Path.resolve', self.resolve) as _6, \
            patch('pathlib.Path.samefile', self.samefile) as _7:

            engine_path = manifest.get_project_engine_path(project_path)
            assert engine_path == expected_engine_path

class TestManifestGetGemsJsonData:

    manifest_external_path = pathlib.Path("manifest_gem1")
    engine_external_path = pathlib.Path("engine_gem1")
    project_external_path = pathlib.Path("project_gem1")
    gem_external_path = pathlib.Path("external_gem1")
    gem_version1_external_path = pathlib.Path("external_gem_v1")
    gem_version2_external_path = pathlib.Path("external_gem_v2")

    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize("engine_path, project_path, include_manifest_gems, include_engine_gems,"\
                            "external_subdirectories, expected_result", [
            # when engine_path provided, expect engine gems
            pytest.param(pathlib.Path('C:/engine1'), None, False, False, list(), 
                {'engine_gem1':[{'gem_name':'engine_gem1', 'path':pathlib.Path('engine_gem1')}]}),
            # when project_path provided, expect project gems
            pytest.param(None, pathlib.Path('C:/project1'), False, False, list(),
                {'project_gem1':[{'gem_name':'project_gem1', 'path':pathlib.Path('project_gem1')}]}),
            # when manifest gems are requested expect manifest gems 
            pytest.param(None, None, True, False, list(),
                {'manifest_gem1':[{'gem_name':'manifest_gem1', 'path':pathlib.Path('manifest_gem1')}]}),
            # when engine gems are requested expect engine gems 
            pytest.param(None, None, False, True, list(),
                {'engine_gem1':[{'gem_name':'engine_gem1', 'path':pathlib.Path('engine_gem1')}]}),
            # when project_path provided and engine gems are requested expect both 
            pytest.param(None, pathlib.Path('C:/project1'), False, True, list(),
                {'project_gem1':[{'gem_name':'project_gem1', 'path':pathlib.Path('project_gem1')}],
                 'engine_gem1':[{'gem_name':'engine_gem1', 'path':pathlib.Path('engine_gem1')}]}),
            # when external subdirectories are provided (recursive), expect they are found 
            pytest.param(None, None, False, False, ['external_gem1'],
                {'external_gem1':[{'gem_name':'external_gem1', 'external_subdirectories':['external_sub_gem2'], 'path':pathlib.Path('external_gem1')}],
                'external_sub_gem2':[{'gem_name':'external_sub_gem2', 'path':pathlib.Path('external_gem1/external_sub_gem2')}]
                }),
            # when a gem with multiple version exists, expect they are both found
            pytest.param(None, None, False, False, ['external_gem_v1','external_gem_v2'],
                {'versioned_gem':[
                    {'gem_name':'versioned_gem', 'version':'1.0.0', 'path':pathlib.Path('external_gem_v1')},
                    {'gem_name':'versioned_gem', 'version':'2.0.0', 'path':pathlib.Path('external_gem_v2')},
                    ],
                }),
        ]
    )
    def test_get_gems_json_data_by_name(self, engine_path, project_path, 
                                    include_manifest_gems, include_engine_gems, 
                                    external_subdirectories, expected_result):

        def get_manifest_external_subdirectories() -> list:
            return [self.manifest_external_path]

        def get_engine_external_subdirectories(engine_path:pathlib.Path = None) -> list:
            return [self.engine_external_path]
        
        def get_project_external_subdirectories(project_path: pathlib.Path) -> list:
            return [self.project_external_path]

        def get_project_engine_path(project_path: pathlib.Path) -> pathlib.Path or None:
            return None

        def get_gem_json_data(gem_name: str = None, gem_path: str or pathlib.Path = None,
                            project_path: pathlib.Path = None):
            if gem_path == self.manifest_external_path:
                return {'gem_name':'manifest_gem1'}
            elif gem_path == self.engine_external_path:
                return {'gem_name':'engine_gem1'}
            elif gem_path == self.project_external_path:
                return {'gem_name':'project_gem1'}
            elif gem_path == self.gem_external_path:
                return {'gem_name':'external_gem1', 'external_subdirectories':['external_sub_gem2']}
            elif gem_path == self.gem_external_path / 'external_sub_gem2':
                return {'gem_name':'external_sub_gem2'}
            elif gem_path == self.gem_version1_external_path:
                return {'gem_name':'versioned_gem','version':'1.0.0'}
            elif gem_path == self.gem_version2_external_path:
                return {'gem_name':'versioned_gem','version':'2.0.0'}
            return  {}

        with patch('o3de.manifest.get_gem_json_data', side_effect=get_gem_json_data) as get_gem_json_data_patch, \
            patch('o3de.manifest.get_manifest_external_subdirectories', side_effect=get_manifest_external_subdirectories) as get_manifest_external_subdirs_patch, \
            patch('o3de.manifest.get_engine_external_subdirectories', side_effect=get_engine_external_subdirectories) as get_engine_external_subdirs_patch, \
            patch('o3de.manifest.get_project_external_subdirectories', side_effect=get_project_external_subdirectories) as get_project_external_subdirs_patch, \
            patch('o3de.manifest.get_project_engine_path', side_effect=get_project_engine_path) as get_project_engine_path_patch,\
            patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_patch,\
            patch('pathlib.Path.resolve', new=self.resolve) as pathlib_resolve_patch:\

            all_gems_by_name = manifest.get_gems_json_data_by_name(engine_path=engine_path, 
                                            project_path=project_path,
                                            include_manifest_gems=include_manifest_gems,
                                            include_engine_gems=include_engine_gems,
                                            external_subdirectories=external_subdirectories)
            assert all_gems_by_name == expected_result

class TestManifestRemoveNonDependencyGemJsonData:
    @pytest.mark.parametrize("top_level_gem_names, gems_json_data_by_name, expected_result", [
            # when no dependencies, only top level gems returned 
            pytest.param(['gem1'], 
                {'gem1':{'dependencies':[]}, 'gem2':{}}, 
                {'gem1':{'dependencies':[]}}),
            # when dependencies exist, non-dependencies are removed 
            pytest.param(['gem1'], 
                {'gem1':{'dependencies':['gem2']}, 'gem2':{'dependencies':['gem3']}, 'gem3':{}, 'gem4':{}}, 
                {'gem1':{'dependencies':['gem2']}, 'gem2':{'dependencies':['gem3']}, 'gem3':{}}), 
        ]
    )
    def test_remove_non_dependency_gem_json_data(self, top_level_gem_names, gems_json_data_by_name, expected_result):
            manifest.remove_non_dependency_gem_json_data(gem_names=top_level_gem_names, 
                                                        gems_json_data_by_name=gems_json_data_by_name)
            assert gems_json_data_by_name == expected_result
