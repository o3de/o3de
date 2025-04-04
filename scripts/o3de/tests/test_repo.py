#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import copy
import json
import pytest
import pathlib
import urllib.request
from unittest.mock import patch, MagicMock, mock_open

from o3de import manifest, register, repo

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
    "projects": [],
    "external_subdirectories": [],
    "templates": [],
    "restricted": [],
    "repos": [],
    "engines": []
}
'''

TEST_O3DE_REPO_FILENAME = 'c93a8a9235af9a27e63c4034ee3a0c26a284ced84c96d1de5dff9903daecc2f6.json'
TEST_O3DE_REPO_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "origin": "",
    "gems": []
}
'''

TEST_O3DE_REPOA_FILENAME = 'ecc7945d5e982114942a8918b52cb37476c38903ff9fd0a1eb9977d3fa2f23b5.json'
TEST_O3DE_REPOA_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "origin": "",
    "gems": ["http://o3derepo.org/TestGem"],
    "projects": ["http://o3derepo.org/TestProject"],
    "templates": ["http://o3derepo.org/TestTemplate"]
}
'''

TEST_O3DE_REPOB_FILENAME = 'fe04d87c744a0f41383122f3dd279b216376f94b4105d874b7856aa42f5e4112.json'
TEST_O3DE_REPOB_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "origin": "",
    "gems": ["http://o3derepo.org/TestGem", "http://o3derepo.org/TestGem2"],
    "projects": ["http://o3derepo.org/TestProject2"],
    "templates": ["http://o3derepo.org/TestTemplate2"]
}
'''

# This holds the repo 1.0.0 schema, which houses all relevant objects in file
# A possible future field for the version data is download_prebuilt_uri, 
# which indicates where to download dedicated binaries of a version of the O3DE object. 
TEST_O3DE_REPO_JSON_VERSION_2_FILENAME = '3b14717bafd5a3bd768d3d0791a44998c3bd0fb2bfa1a7e2ee8bb1a39b04d631.json'
TEST_O3DE_REPO_JSON_VERSION_2_PAYLOAD = '''
{
    "repo_name": "testgem3",
    "origin": "Studios",
    "$schemaVersion":"1.0.0",
    "repo_uri": "https://downloads.testgem3.com/o3de-repo",
    "summary": "Studios Repository for the testgem3 Gem.",
    "additional_info": "",
    "last_updated": "2023-01-19",
    "gems":[
        "https://legacygemrepo.com"
    ],
    "gems_data": [{
        "gem_name": "testgem3",
        "display_name": "testgem3 2",
        "version": "1.0.0",
        "download_api": "HTTP",
        "license": "Community",
        "license_url": "https://www.testgem3.com/testgem3-community-license",
        "origin": "Persistant Studios - testgem3.com",
        "requirements": "Users will need to download testgem3 Editor from the <a href='https://www.testgem3.com/download/'>testgem3 Web Site</a> to edit/author effects.",
        "documentation_url": "https://www.testgem3.com/docs/testgem3-v2/plugins/o3de-gem/",
        "type": "Code",
        "summary": "The testgem3 Gem provides real-time FX solution for particle effects.",
        "canonical_tags": [
            "Gem"
        ],
        "user_tags": [
            "Particles",
            "Simulation",
            "SDK"
        ],
        "dependencies": [],
        "download_source_uri": "https://downloads.testgem3.com/o3de-repo/testgem3-2.15/O3DE_testgem3Gem_v2.0.0_Win64_Linux64_Mac64.zip",
        "versions_data": [
            {
                "display_name": "testgem3 2.0.0",
                "download_source_uri": "https://downloads.testgem3.com/o3de-repo/testgem3-2.15/O3DE_testgem3Gem_v2.15.4_Win64_Linux64_Mac64.zip",
                "version": "2.0.0",
                "last_updated": "2023-03-09"
            }
        ]
    }],
    "project":[],
    "projects_data":[{
            "project_name": "TestProject",
            "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
            "version" : "0.0.0",
            "compatible_engines" : [
                "o3de-sdk==2205.01"
            ],
            "engine_api_dependencies" : [
                "framework==1.2.3"
            ],
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
    ],
    "templates":[],
    "templates_data":[{
            "template_name": "TestTemplate",
            "license": "Apache-2.0 Or MIT",
            "origin": "Test Creator",
            "origin_uri": "http://o3derepo.org/TestTemplate/template.zip",
            "repo_uri": "http://o3derepo.org",
            "type": "Tool",
            "summary": "A test downloadable gem.",
            "canonical_tags": [
                "Template"
            ],
            "user_tags": [],
            "icon_path": "preview.png",
            "requirements": "",
            "documentation_url": "",
            "dependencies": []
        }
    ]
}
'''

TEST_O3DE_REPO_GEM3_JSON_VERSION_1_PAYLOAD = '''
{
    "gem_name": "testgem3",
    "display_name": "testgem3 2",
    "download_api": "HTTP",
    "license": "Community",
    "license_url": "https://www.testgem3.com/testgem3-community-license",
    "origin": "Persistant Studios - testgem3.com",
    "requirements": "Users will need to download testgem3 Editor from the <a href='https://www.testgem3.com/download/'>testgem3 Web Site</a> to edit/author effects.",
    "documentation_url": "https://www.testgem3.com/docs/testgem3-v2/plugins/o3de-gem/",
    "type": "Code",
    "summary": "The testgem3 Gem provides real-time FX solution for particle effects.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "Particles",
        "Simulation",
        "SDK"
    ],
    "dependencies": [],
    "download_source_uri": "https://downloads.testgem3.com/o3de-repo/testgem3-2.15/O3DE_testgem3Gem_v2.0.0_Win64_Linux64_Mac64.zip",
    "version": "1.0.0"
}
'''

TEST_O3DE_REPO_GEM3_JSON_VERSION_2_PAYLOAD = '''
{
    "gem_name": "testgem3",
    "download_api": "HTTP",
    "license": "Community",
    "license_url": "https://www.testgem3.com/testgem3-community-license",
    "origin": "Persistant Studios - testgem3.com",
    "requirements": "Users will need to download testgem3 Editor from the <a href='https://www.testgem3.com/download/'>testgem3 Web Site</a> to edit/author effects.",
    "documentation_url": "https://www.testgem3.com/docs/testgem3-v2/plugins/o3de-gem/",
    "type": "Code",
    "summary": "The testgem3 Gem provides real-time FX solution for particle effects.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "Particles",
        "Simulation",
        "SDK"
    ],
    "dependencies": [],
    "display_name": "testgem3 2.0.0",
    "download_source_uri": "https://downloads.testgem3.com/o3de-repo/testgem3-2.15/O3DE_testgem3Gem_v2.15.4_Win64_Linux64_Mac64.zip",
    "version": "2.0.0",
    "last_updated": "2023-03-09"
}
'''

TEST_O3DE_REPO_BROKEN_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "gems": []
}
'''

TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "origin": "",
    "gems": ["http://o3derepo.org/TestGem"],
    "projects": ["http://o3derepo.org/TestProject"],
    "templates": ["http://o3derepo.org/TestTemplate"]
}
'''

TEST_O3DE_RECURSIVE_REPO_FILENAME = '9265e40cf042c1e70b04f3886ed361c7b305fed5e6f5cb239b22d69ccff7cf09.json'
TEST_O3DE_RECURSIVE_REPO_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "origin": "",
    "repos": ["http://o3derepo.org"]
}
'''

TEST_O3DE_REPO_GEM_FILE_NAME = 'a765db91484f0d963d4ba5c98161074df7cd87caf1340e6bc7cebdce1807c994.json'
TEST_O3DE_REPO_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestGem/gem.zip",
    "repo_uri": "http://o3derepo.org",
    "type": "Tool",
    "summary": "A test downloadable gem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "",
    "dependencies": []
}
'''
TEST_O3DE_REPO_GEM2_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem2",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestGem2/gem.zip",
    "repo_uri": "http://o3derepo.org",
    "type": "Tool",
    "summary": "A test downloadable gem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "",
    "dependencies": []
}
'''

TEST_O3DE_REPO_PROJECT_FILE_NAME = '233c6e449888b4dc1355b2bf668b91b53715888e6777a2791df0e7aec9d08989.json'
TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
    "version" : "0.0.0",
    "compatible_engines" : [
        "o3de-sdk==2205.01"
    ],
    "engine_api_dependencies" : [
        "framework==1.2.3"
    ],
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
TEST_O3DE_REPO_PROJECT2_JSON_PAYLOAD = '''
{
    "project_name": "TestProject2",
    "project_id": "{04112c69-306d-4de6-b3b4-4cb1a3eca58e}",
    "version" : "0.0.0",
    "compatible_engines" : [
        "o3de==1.2.3"
    ],
    "engine_api_dependencies" : [
        "framework==1.2.3"
    ],
    "origin": "The primary repo for TestProject2 goes here: i.e. http://www.mydomain.com",
    "license": "What license TestProject2 uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "TestProject",
    "summary": "A short description of TestProject.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [
        "TestProject2"
    ],
    "icon_path": "preview.png",
    "engine": "o3de==1.2.3",
    "restricted_name": "projects",
    "external_subdirectories": [
        "TestGem"
    ]
}
'''

TEST_O3DE_REPO_TEMPLATE_FILE_NAME = '7802eae005ca1c023e14611ed63182299bf87e760708b4dba8086a134e309f3a.json'
TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD = '''
{
    "template_name": "TestTemplate",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestTemplate/template.zip",
    "repo_uri": "http://o3derepo.org",
    "type": "Tool",
    "summary": "A test downloadable gem.",
    "canonical_tags": [
        "Template"
    ],
    "user_tags": [],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "",
    "dependencies": []
}
'''
TEST_O3DE_REPO_TEMPLATE2_JSON_PAYLOAD = '''
{
    "template_name": "TestTemplate2",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestTemplate/template2.zip",
    "repo_uri": "http://o3derepo.org",
    "type": "Tool",
    "summary": "A test downloadable gem.",
    "canonical_tags": [
        "Template"
    ],
    "user_tags": [],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "",
    "dependencies": []
}
'''

@pytest.fixture(scope='class')
def init_register_repo_data(request):
    request.cls.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)

@pytest.mark.usefixtures('init_register_repo_data')
class TestRepos:
    created_files = []
    valid_urls = [
        'http://o3derepo.org/repo.json',
        'http://o3derepo.org/TestGem/gem.json',
        'http://o3derepo.org/TestGem2/gem.json',
        'http://o3derepo.org/TestProject/project.json',
        'http://o3derepo.org/TestProject2/project.json',
        'http://o3derepo.org/TestTemplate/template.json',
        'http://o3derepo.org/TestTemplate2/template.json',
        'http://o3derecursiverepo.org/repo.json'
    ]

    @pytest.mark.parametrize("repo_path, expected_manifest_file, expected_result", [
                                 pytest.param('http://o3de.org', pathlib.PurePath('o3de_manifest.json'), True),
                                 pytest.param('http://o3de.org/incorrect', pathlib.PurePath('o3de_manifest.json'), False),
                             ])
    def test_get_repository_list(self, repo_path, expected_manifest_file,expected_result):
        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
        self.o3de_manifest_data["repos"] = ["http://o3de.org", "http://removablerepo.com"]

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return self.o3de_manifest_data

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1:
            assert (repo_path in manifest.get_manifest_repos()) == expected_result

    @pytest.mark.parametrize("repo_uri, expected_result, expected_in_repo, download_repo_data, created_file", [
                                 pytest.param('http://o3derepo.org', 0, True, TEST_O3DE_REPO_JSON_PAYLOAD, TEST_O3DE_REPO_FILENAME),
                                 pytest.param('http://o3derepo.org', 1, False, TEST_O3DE_REPO_BROKEN_JSON_PAYLOAD, ''),
                                 pytest.param('http://o3derepo.org', 0, True, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_FILE_NAME),
                                 pytest.param('http://o3de.org/incorrect', 1, False, 0, ''),
                                 pytest.param('http://o3derecursiverepo.org', 0,True, TEST_O3DE_RECURSIVE_REPO_JSON_PAYLOAD, TEST_O3DE_REPO_FILENAME),
                             ])
    def test_add_repository(self, repo_uri, expected_result, expected_in_repo, download_repo_data, created_file):

        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
        self.o3de_manifest_data["repos"] = ["http://o3de.org", "http://removablerepo.com"]
        self.created_files.clear()

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return copy.deepcopy(self.o3de_manifest_data)

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            self.o3de_manifest_data = manifest_data
            return True

        def mocked_requests_get(url):
            if isinstance(url, urllib.request.Request):
                url_str = url.get_full_url()
            else:
                url_str = url

            if url_str in self.valid_urls:
                custom_mock = MagicMock()
                custom_mock.getcode.return_value = 200
                custom_mock.read.return_value = 0
                custom_mock.__enter__.return_value = custom_mock
            else:
                raise urllib.error.HTTPError(url_str, 404, "Not found", {}, 0)

            return custom_mock

        def mocked_open(path, mode, *args, **kwargs):
            file_data = bytes(0)
            if pathlib.Path(path).name == TEST_O3DE_REPO_FILENAME:
                file_data = download_repo_data
            elif pathlib.Path(path).name == TEST_O3DE_RECURSIVE_REPO_FILENAME:
                file_data = TEST_O3DE_RECURSIVE_REPO_JSON_PAYLOAD
            mockedopen = mock_open(mock=MagicMock(), read_data=file_data)
            if 'w' in mode:
                self.created_files.append(path)
            return mockedopen(self, *args, **kwargs)

        def mocked_isfile(path):
            if path in self.created_files:
                return True
            else:
                return False

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2, \
                patch('urllib.request.urlopen', side_effect=mocked_requests_get) as _3, \
                patch('pathlib.Path.open', mocked_open) as _4, \
                patch('pathlib.Path.is_file', mocked_isfile) as _5:

            result = register.register(repo_uri=repo_uri, force=True)

            assert result == expected_result
            assert (repo_uri in manifest.get_manifest_repos()) == expected_in_repo

            #If we were expecting to create a file, check that it was created
            matches = [pathlib.Path(x).name for x in self.created_files if pathlib.Path(x).name == created_file]
            assert (len(matches) != 0) == (created_file != '') 

    @pytest.mark.parametrize("repo_path, existing_repo, expected_result", [
                                 pytest.param('http://o3de.org', True, 0),
                                 pytest.param('http://o3de.org/incorrect', False, 0),
                             ])
    def test_remove_repository(self, repo_path, existing_repo, expected_result):
        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
        self.o3de_manifest_data["repos"] = ["http://o3de.org", "http://removablerepo.com"]

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return self.o3de_manifest_data

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            self.o3de_manifest_data = manifest_data
            return True

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2:
            assert (repo_path in manifest.get_manifest_repos()) == existing_repo
            result = register.register(repo_uri=repo_path, remove=True)
            assert result == expected_result
            assert repo_path not in manifest.get_manifest_repos()

    @pytest.mark.parametrize("test_name, repo_paths, expected_gems_json_data, expected_projects_json_data, expected_templates_json_data", [
            pytest.param("repoA loads repoA objects", ['http://o3de.org/repoA'],  
                         [TEST_O3DE_REPO_GEM_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD]),
            pytest.param("repoB loads repoB objects", ['http://o3de.org/repoB'],  
                         [TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_REPO_GEM2_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_PROJECT2_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_TEMPLATE2_JSON_PAYLOAD]),
            # TestGem is included twice because it is in both repositories
            pytest.param("repoA and repoB loads all objects", ['http://o3de.org/repoA','http://o3de.org/repoB'],  
                         [TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_REPO_GEM2_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD, TEST_O3DE_REPO_PROJECT2_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD, TEST_O3DE_REPO_TEMPLATE2_JSON_PAYLOAD]),
            # RepoC contains a mix of objects with versions_data and objects without
            pytest.param("repoC loads repoC objects", ['http://o3de.org/repoC'],  
                         [TEST_O3DE_REPO_GEM3_JSON_VERSION_1_PAYLOAD, TEST_O3DE_REPO_GEM3_JSON_VERSION_2_PAYLOAD],
                         [TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD],
                         [TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD])
        ])
    def test_get_object_json_data(self, test_name, repo_paths, expected_gems_json_data, 
                                  expected_projects_json_data, expected_templates_json_data):
        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
        self.o3de_manifest_data["repos"] = repo_paths

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return copy.deepcopy(self.o3de_manifest_data)

        def mocked_open(path, mode, *args, **kwargs):
            file_data = bytes(0)
            if pathlib.Path(path).name == TEST_O3DE_REPOA_FILENAME:
                file_data = TEST_O3DE_REPOA_JSON_PAYLOAD
            elif pathlib.Path(path).name == TEST_O3DE_REPOB_FILENAME:
                file_data = TEST_O3DE_REPOB_JSON_PAYLOAD
            elif pathlib.Path(path).name == TEST_O3DE_REPO_JSON_VERSION_2_FILENAME:
                file_data = TEST_O3DE_REPO_JSON_VERSION_2_PAYLOAD

            mockedopen = mock_open(mock=MagicMock(), read_data=file_data)
            return mockedopen(self, *args, **kwargs)

        def get_json_data_file(object_json: pathlib.Path,
                            object_typename: str,
                            object_validator: callable) -> dict or None:
            if object_typename == 'gem':
                if object_json == self.test_gem_cache_filename:
                    return json.loads(TEST_O3DE_REPO_GEM_JSON_PAYLOAD)
                if object_json == self.test_gem2_cache_filename:
                    return json.loads(TEST_O3DE_REPO_GEM2_JSON_PAYLOAD)
                else:
                    return None
            elif object_typename == 'project':
                if object_json == self.test_project_cache_filename:
                    return json.loads(TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD)
                elif object_json == self.test_project2_cache_filename:
                    return json.loads(TEST_O3DE_REPO_PROJECT2_JSON_PAYLOAD)
                else:
                    return None
            elif object_typename == 'template':
                if object_json == self.test_template_cache_filename:
                    return json.loads(TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD)
                elif object_json == self.test_template2_cache_filename:
                    return json.loads(TEST_O3DE_REPO_TEMPLATE2_JSON_PAYLOAD)
                else:
                    return None
            else:
                None

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
            patch('o3de.manifest.get_o3de_cache_folder', return_value=pathlib.Path('Cache')) as _2, \
            patch('o3de.manifest.get_json_data_file', side_effect=get_json_data_file) as get_json_data_file_patch, \
            patch('pathlib.Path.open', mocked_open) as _3, \
            patch('pathlib.Path.is_file', return_value=True) as _4:

            self.test_gem_cache_filename, _ = repo.get_cache_file_uri("http://o3derepo.org/TestGem/gem.json")
            self.test_gem2_cache_filename, _ = repo.get_cache_file_uri("http://o3derepo.org/TestGem2/gem.json")
            self.test_project_cache_filename, _ = repo.get_cache_file_uri("http://o3derepo.org/TestProject/project.json")
            self.test_project2_cache_filename, _ = repo.get_cache_file_uri("http://o3derepo.org/TestProject2/project.json")
            self.test_template_cache_filename, _ = repo.get_cache_file_uri("http://o3derepo.org/TestTemplate/template.json")
            self.test_template2_cache_filename, _ = repo.get_cache_file_uri("http://o3derepo.org/TestTemplate2/template.json")

            # Gems
            gems_json_data = repo.get_gem_json_data_from_all_cached_repos()
            expected_gems_json_list = [json.loads(data) for data in expected_gems_json_data]
            assert all(json_data in expected_gems_json_list for json_data in gems_json_data) 
            # Projects
            projects_json_data = repo.get_project_json_data_from_all_cached_repos()
            expected_projects_json_list = [json.loads(data) for data in expected_projects_json_data]
            assert all(json_data in expected_projects_json_list for json_data in projects_json_data) 
            # Templates
            templates_json_data = repo.get_template_json_data_from_all_cached_repos()
            expected_templates_json_list = [json.loads(data) for data in expected_templates_json_data]
            assert all(json_data in expected_templates_json_list for json_data in templates_json_data) 


    @pytest.mark.parametrize("repo_uri, validate_objects", [
        # tests with version schema 0.0.0
        pytest.param('http://o3de.org/repoA', False),
        pytest.param('http://o3de.org/repoA', True),
        # tests with version schema 1.0.0
        pytest.param('http://o3de.org/repoC', False),
        pytest.param('http://o3de.org/repoC', True)
    ])
    def test_validation(self, repo_uri, validate_objects):
        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)
        self.o3de_manifest_data["repos"] = []
        self.created_files.clear()

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return copy.deepcopy(self.o3de_manifest_data)

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            self.o3de_manifest_data = manifest_data
            return True

        def mocked_requests_get(url):
            if isinstance(url, urllib.request.Request):
                url_str = url.get_full_url()
            else:
                url_str = url

            if url_str in ['http://o3de.org/repoA/repo.json',
                           'http://o3de.org/repoC/repo.json',
                           'http://o3derepo.org/TestProject/project.json',
                           'http://o3derepo.org/TestGem/gem.json',
                           'http://o3derepo.org/TestTemplate/template.json']:
                custom_mock = MagicMock()
                custom_mock.getcode.return_value = 200
                custom_mock.read.return_value = 0
                custom_mock.__enter__.return_value = custom_mock
            else:
                raise urllib.error.HTTPError(url_str, 404, "Not found", {}, 0)

            return custom_mock

        def mocked_open(path, mode, *args, **kwargs):
            file_data = bytes(0)
            pathname = pathlib.Path(path).name
            if pathname == TEST_O3DE_REPOA_FILENAME:
                file_data = TEST_O3DE_REPOA_JSON_PAYLOAD
            elif pathname == TEST_O3DE_REPOB_FILENAME:
                file_data = TEST_O3DE_REPOB_JSON_PAYLOAD
            elif pathname == TEST_O3DE_REPO_JSON_VERSION_2_FILENAME:
                file_data = TEST_O3DE_REPO_JSON_VERSION_2_PAYLOAD

            elif pathname == TEST_O3DE_REPO_GEM_FILE_NAME:
                file_data = TEST_O3DE_REPO_GEM_JSON_PAYLOAD
            elif pathname == TEST_O3DE_REPO_PROJECT_FILE_NAME:
                file_data = TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD
            elif pathname == TEST_O3DE_REPO_TEMPLATE_FILE_NAME:
                file_data = TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD
            
            mockedopen = mock_open(mock=MagicMock(), read_data=file_data)
            if 'w' in mode:
                self.created_files.append(path)
            return mockedopen(self, *args, **kwargs)

        def mocked_isfile(path):
            if path in self.created_files:
                return True
            else:
                return False

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2, \
                patch('pathlib.Path.open', mocked_open) as _3, \
                patch('urllib.request.urlopen', side_effect=mocked_requests_get) as _4, \
                patch('pathlib.Path.is_file', mocked_isfile) as _5:
                    valid = repo.validate_remote_repo(repo_uri, validate_objects)
                    assert valid
