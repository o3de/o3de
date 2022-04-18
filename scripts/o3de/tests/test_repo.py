#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import copy
import json
import logging
import pytest
import pathlib
import urllib.request
from unittest.mock import patch, MagicMock, mock_open

from o3de import manifest, register, repo

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
    "projects": [],
    "external_subdirectories": [],
    "templates": [],
    "restricted": [],
    "repos": ["http://o3de.org", "http://removablerepo.com"],
    "engines": [],
    "engines_path": {}
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

@pytest.fixture(scope='class')
def init_register_repo_data(request):
    request.cls.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)

@pytest.mark.usefixtures('init_register_repo_data')
class TestRepos:
    created_files = []
    valid_urls = [
        'http://o3derepo.org/repo.json',
        'http://o3derepo.org/TestGem/gem.json',
        'http://o3derepo.org/TestProject/project.json',
        'http://o3derepo.org/TestTemplate/template.json',
        'http://o3derecursiverepo.org/repo.json'
    ]

    @pytest.mark.parametrize("repo_path, expected_manifest_file, expected_result", [
                                 pytest.param('http://o3de.org', pathlib.PurePath('o3de_manifest.json'), True),
                                 pytest.param('http://o3de.org/incorrect', pathlib.PurePath('o3de_manifest.json'), False),
                             ])
    def test_get_repository_list(self, repo_path, expected_manifest_file,expected_result):
        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)

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
        self.created_files.clear()

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return copy.deepcopy(self.o3de_manifest_data)

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            self.o3de_manifest_data = manifest_data
            return True

        def mocked_requests_get(url):
            if url in self.valid_urls:
                cm = MagicMock()
                cm.getcode.return_value = 200
                cm.read.return_value = 0
                cm.__enter__.return_value = cm
            else:
                raise urllib.error.HTTPError(url, 404, "Not found", {}, 0)

            return cm

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
