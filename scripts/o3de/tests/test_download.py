#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import copy
import json
import pytest
import pathlib
import urllib.request
from unittest.mock import patch, MagicMock, mock_open

from o3de import manifest, download, utils

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
    "repos": ["http://o3de.org"],
    "engines": [],
    "engines_path": {}
}
'''

TEST_O3DE_MANIFEST_EXISTING_GEM_JSON_PAYLOAD = '''
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
    "external_subdirectories": ["C:/Users/testuser/O3DE/Gems/TestGem"],
    "templates": [],
    "restricted": [],
    "repos": ["http://o3de.org"],
    "engines": [],
    "engines_path": {}
}
'''

TEST_O3DE_REPO_FILE_NAME = '3fb160cdfde8b32864335e71a9b7a0519591f3080d2a06e7ca10f830e0cb7a54.json'
TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD = '''
{
    "repo_name": "Test Repo",
    "origin": "",
    "gems": ["http://o3derepo.org/TestGem", "C:/localrepo/TestLocalGem"],
    "projects": ["http://o3derepo.org/TestProject"],
    "engines": ["http://o3derepo.org/TestEngine"],
    "templates": ["http://o3derepo.org/TestTemplate"]
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
    "last_updated": "2022-01-01 14:00:00",
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

TEST_O3DE_REPO_GEM_WITH_HASH_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestGem/gem.zip",
    "repo_uri": "http://o3derepo.org",
    "sha256": "cd89c508cad0e48e51806a9963d17a0f2f7196e26c79f45aa9ea3b435a2ceb6a",
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

TEST_O3DE_LOCAL_REPO_GEM_FILE_NAME = '8758b5acace49baf89ba5d36c1c214f10f8e47cd198096d1ae6b016b23b0833d.json'
TEST_O3DE_LOCAL_REPO_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestLocalGem",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "C:/localrepo/TestLocalGem/gem.zip",
    "repo_uri": "http://o3derepo.org",
    "last_updated": "Jan-2022",
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

TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestProject/project.zip",
    "repo_uri": "http://o3derepo.org",
    "last_updated": "2022-01-01 11:00:00",
    "type": "Tool",
    "summary": "A test downloadable gem.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "",
    "dependencies": []
}
'''

TEST_O3DE_REPO_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "TestEngine",
    "license": "Apache-2.0 Or MIT",
    "origin": "Test Creator",
    "origin_uri": "http://o3derepo.org/TestEngine/engine.zip",
    "repo_uri": "http://o3derepo.org",
    "last_updated": "2021-12-01",
    "type": "Tool",
    "summary": "A test downloadable gem.",
    "canonical_tags": [
        "Engine"
    ],
    "user_tags": [],
    "icon_path": "preview.png",
    "requirements": "",
    "documentation_url": "",
    "dependencies": []
}
'''

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

# This data will be hashed as if it were the zip file
TEST_O3DE_ZIP_FILE_DATA = 'O3DE'
TEST_O3DE_INCORRECT_FILE_DATA = 'O3DE '

@pytest.fixture(scope='class')
def init_register_repo_data(request):
    request.cls.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)

@pytest.mark.usefixtures('init_register_repo_data')
class TestObjectDownload:
    extracted_gem_json = ''
    extracted_gem_path = ''
    created_files = []
    valid_urls = [
        'http://o3derepo.org/repo.json',
        'http://o3derepo.org/TestGem/gem.json',
        'http://o3derepo.org/TestGem/gem.zip',
        'http://o3derepo.org/TestProject/project.json',
        'http://o3derepo.org/TestTemplate/template.json',
        'http://o3derecursiverepo.org/repo.json'
    ]

    @pytest.mark.parametrize("manifest_data, gem_name, expected_result, repo_data, gem_data, zip_data, skip_auto_register, force_overwrite, registration_expected", [
                                 # Remote and local gem tests
                                 pytest.param(TEST_O3DE_MANIFEST_JSON_PAYLOAD, 'TestGem', 0, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_ZIP_FILE_DATA, False, True, True),
                                 pytest.param(TEST_O3DE_MANIFEST_JSON_PAYLOAD, 'TestLocalGem', 0, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_LOCAL_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_ZIP_FILE_DATA, False, True, True),
                                 pytest.param(TEST_O3DE_MANIFEST_EXISTING_GEM_JSON_PAYLOAD, 'TestGem', 1, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_ZIP_FILE_DATA, False, False, False),
                                 pytest.param(TEST_O3DE_MANIFEST_JSON_PAYLOAD, 'TestGem', 0, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_ZIP_FILE_DATA, True, True, False),
                                 pytest.param(TEST_O3DE_MANIFEST_JSON_PAYLOAD, 'UnavailableGem', 1, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_JSON_PAYLOAD, TEST_O3DE_ZIP_FILE_DATA, False, True, False),
                                 # hashing tests
                                 pytest.param(TEST_O3DE_MANIFEST_JSON_PAYLOAD, 'TestGem', 0, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_WITH_HASH_JSON_PAYLOAD, TEST_O3DE_ZIP_FILE_DATA, False, True, True),
                                 pytest.param(TEST_O3DE_MANIFEST_JSON_PAYLOAD, 'TestGem', 1, TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD, TEST_O3DE_REPO_GEM_WITH_HASH_JSON_PAYLOAD, TEST_O3DE_INCORRECT_FILE_DATA, False, True, False),
                             ])
    def test_download_gem(self, manifest_data, gem_name, expected_result, repo_data, gem_data, zip_data, skip_auto_register, force_overwrite, registration_expected):
        self.o3de_manifest_data = json.loads(manifest_data)
        self.created_files.clear()
        # add pre existing files
        self.created_files.append('C:/Users/testuser/.o3de/cache/3fb160cdfde8b32864335e71a9b7a0519591f3080d2a06e7ca10f830e0cb7a54.json')
        self.created_files.append('C:/Users/testuser/.o3de/cache/a765db91484f0d963d4ba5c98161074df7cd87caf1340e6bc7cebdce1807c994.json')
        self.created_files.append('C:/Users/testuser/.o3de/cache/8758b5acace49baf89ba5d36c1c214f10f8e47cd198096d1ae6b016b23b0833d.json')
        self.created_files.append('C:/Users/testuser/.o3de/cache/Gems/TestGem/gem.zip')
        self.created_files.append('C:/localrepo/TestLocalGem/gem.zip')

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
                custom_mock.headers = {'content-length': 0}
                custom_mock.__enter__.return_value = custom_mock
            else:
                raise urllib.error.HTTPError(url_str, 404, "Not found", {}, 0)

            return custom_mock

        def mocked_extract(path=None, members=None, pwd=None):
            self.extracted_gem_path = path
            self.extracted_gem_json = gem_data
            self.created_files.append(path / 'gem.json')

        def mocked_open(path, mode = '', *args, **kwargs):
            file_data = zip_data.encode('utf-8')
            if pathlib.Path(path).name == TEST_O3DE_REPO_FILE_NAME:
                file_data = TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD
            elif pathlib.Path(path).name == TEST_O3DE_REPO_GEM_FILE_NAME or \
                pathlib.Path(path).name == TEST_O3DE_LOCAL_REPO_GEM_FILE_NAME or \
                pathlib.Path(path).name == 'gem.json':
                    file_data = gem_data
            elif path == pathlib.Path(self.extracted_gem_path) / 'gem.json':
                file_data = self.extracted_gem_json
            mockedopen = mock_open(mock=MagicMock(), read_data=file_data)
            if 'w' in mode:
                self.created_files.append(path)

            file_obj = mockedopen(self, *args, **kwargs)
            file_obj.extractall = mocked_extract
            file_obj.open = mocked_open
            return file_obj

        def mocked_isfile(path):
            matches = [pathlib.Path(x).name for x in self.created_files if pathlib.Path(x).name == pathlib.Path(path).name]
            if len(matches) != 0:
                return True
            else:
                return False

        def mocked_copy(origin, dest):
            if mocked_isfile(origin):
                self.created_files.append(dest)

        download_callback_called = False
        def download_callback(downloaded, total_size):
            download_callback_called = True

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2, \
                patch('o3de.utils.find_ancestor_dir_containing_file', return_value=None) as _3, \
                patch('urllib.request.urlopen', side_effect=mocked_requests_get) as _4, \
                patch('pathlib.Path.open', mocked_open) as _5, \
                patch('pathlib.Path.is_file', mocked_isfile) as _6, \
                patch('pathlib.Path.mkdir') as _7, \
                patch('pathlib.Path.unlink') as _8, \
                patch('zipfile.is_zipfile', return_value=True) as _9, \
                patch('zipfile.ZipFile', mocked_open) as _10, \
                patch('pathlib.Path.is_dir', return_value=True) as _11, \
                patch('shutil.copy', mocked_copy) as _12, \
                patch('os.unlink') as _13, \
                patch('os.path.getsize', return_value=64) as _14:

            result = download.download_gem(gem_name, '', skip_auto_register, force_overwrite, download_callback)
            assert result == expected_result

            # Check that the path is in external directories - register validates the gem.json so that will be tested here
            matches = [pathlib.Path(x) for x in manifest.get_manifest_external_subdirectories() if pathlib.Path(x) == self.extracted_gem_path]

            assert (len(matches) != 0) == registration_expected

    @pytest.mark.parametrize("update_function, object_name, object_data, existing_time, update_available", [
                                 # Repo gem is newer
                                 pytest.param(download.is_o3de_gem_update_available, 'TestGem', TEST_O3DE_REPO_GEM_JSON_PAYLOAD, "2022-01-01 13:00:00", True),
                                 # Repo engine is not newer
                                 pytest.param(download.is_o3de_engine_update_available, 'TestEngine', TEST_O3DE_REPO_ENGINE_JSON_PAYLOAD, "2021-12-01", False),
                                 # Repo project has a last_updated field, local does not
                                 pytest.param(download.is_o3de_project_update_available, 'TestProject', TEST_O3DE_REPO_PROJECT_JSON_PAYLOAD, "", True),
                                 # Repo template does not have a last_updated field
                                 pytest.param(download.is_o3de_template_update_available, 'TestTemplate', TEST_O3DE_REPO_TEMPLATE_JSON_PAYLOAD, "", False),
                                 # Repo object does not exist
                                 pytest.param(download.is_o3de_gem_update_available, 'NonExistingObject', "", "", False),
                                 # Incorrect repo datetime format
                                 pytest.param(download.is_o3de_gem_update_available, 'TestLocalGem', TEST_O3DE_LOCAL_REPO_GEM_JSON_PAYLOAD, "2021-12-01", False),
                             ])
    def test_check_updates(self, update_function, object_name, object_data, existing_time, update_available):
        self.o3de_manifest_data = json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)

        def load_o3de_manifest(manifest_path: pathlib.Path = None) -> dict:
            return copy.deepcopy(self.o3de_manifest_data)

        def save_o3de_manifest(manifest_data: dict, manifest_path: pathlib.Path = None) -> bool:
            self.o3de_manifest_data = manifest_data
            return True

        def mocked_open(path, mode, *args, **kwargs):
            file_data = bytes(0)
            if pathlib.Path(path).name == TEST_O3DE_REPO_FILE_NAME:
                file_data = TEST_O3DE_REPO_WITH_OBJECTS_JSON_PAYLOAD
            else:
                file_data = object_data
            mockedopen = mock_open(mock=MagicMock(), read_data=file_data)
            return mockedopen(self, *args, **kwargs)

        with patch('o3de.manifest.load_o3de_manifest', side_effect=load_o3de_manifest) as _1,\
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as _2, \
                patch('pathlib.Path.open', mocked_open) as _3, \
                patch('pathlib.Path.is_file', return_value=True) as _4:

            assert update_function(object_name, existing_time) == update_available
