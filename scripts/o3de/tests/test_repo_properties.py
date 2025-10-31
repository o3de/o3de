#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import os
import shutil
import pytest
import pathlib
from unittest.mock import patch

from o3de import download, repo_properties, utils

TEST_TEMPLATE_REPO_JSON = '''{
    "repo_name": "repotest",
    "repo_uri": "https://test.com",
    "$schemaVersion": "1.0.0",
    "origin": "o3de",
    "origin_url": "${OriginURL}",
    "summary": "${Summary}",
    "additional_info": "${AdditionalInfo}",
    "last_updated": "",
    "gems_data": [],
    "projects_data": [],
    "templates_data": []
}
'''

TEST_GEM_JSON_STR = '''{
    "gem_name": "TestGem",
    "version": "0.0.0"
}
'''

TEST_PROJECT_JSON_STR = '''{
    "project_name": "ProjectSample",
    "version": "1.0.0"
}
'''

TEST_TEMPLATE_JSON_STR = '''{
    "template_name": "MinimalProject"
}
'''

DRY_RUN_EXPECTED ="""Dry Run:
Gems Added: 1
  1. TestGem
Gems Modified: 0
Projects Added: 1
  1. ProjectSample
Projects Modified: 0
Templates Added: 1
  1. MinimalProject
Templates Modified: 0"""

TEST_GEM_JSON_PAYLOAD = {
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

TEST_PROJECT_JSON_PAYLOAD = {
    "project_name": "ProjectSample",
    "version": "1.0.0",
    "project_id": "{0A341AFF-E129-482E-B6E3-48467F18CA31}",
    "origin": "The primary repo for ProjectSample goes here: i.e. http://www.mydomain.com",
    "license": "What license ProjectSample uses goes here: i.e. https://opensource.org/licenses/Apache-2.0 Or https://opensource.org/licenses/MIT etc.",
    "display_name": "ProjectSample",
    "summary": "A short description of ProjectSample.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [
        "ProjectSample"
    ],
    "icon_path": "preview.png",
    "engine": "o3de",
    "external_subdirectories": [
        "Gem"
    ],
    "gem_names": [
        "Kaylin",
        "Gene"
    ],
    "restricted": "ProjectSample",
    "engine_version": "2.1.0",
    "repo_uri": "https://github.com/ready-player1/o3deRemoteRepo.git",
    "download_source_uri": "None/projectsample-1.0.0-project.zip"
}

TEST_TEMPLATE_JSON_PAYLOAD = {
    "template_name": "MinimalProject"
}

GEM_VERSION_ONE_JSON={
    "gem_name": "TestGem",
    "version": "1.0.0"
}

GEM_VERSION_TWO_JSON= {
    "gem_name": "TestGem",
    "version": "2.0.0"
}

GEM_VERSION_THREE_JSON= {
    "gem_name": "TestGem",
    "version": "3.0.0"
}

GEM_VERSION_ONE_TOBE_UPDATED= {
    "gem_name": "TestGemUpdate",
    "version": "1.0.0",
    "display_name": "testGem"
}

GEM_VERSION_ONE_WITH_UPDATED_FIELD= {
    "gem_name": "TestGemUpdate",
    "version": "1.0.0",
    "display_name": "hiWorld"
}

GEM_VERSION_TWO_TOBE_UPDATED= {
    "gem_name": "TestGem",
    "version": "2.0.0",
    "display_name": "wastwo"
}

GEM_VERSION_TWO_WITH_UPDATED_FIELD= {
    "gem_name": "TestGem",
    "version": "2.0.0",
    "display_name": "hiWorld"
}

GEM_WITH_NO_VERSION= {
    "gem_name": "TestGem"
}

JSON_DICT={'gem_version1_json_key': GEM_VERSION_ONE_JSON,
           'gem_version2_json_key': GEM_VERSION_TWO_JSON,
           'gem_version3_json_key': GEM_VERSION_THREE_JSON,
           'gem_version1_tobe_updated_key': GEM_VERSION_ONE_TOBE_UPDATED,
           'gem_version1_with_updated_field_key': GEM_VERSION_ONE_WITH_UPDATED_FIELD,
           'gem_version2_tobe_updated_key': GEM_VERSION_TWO_TOBE_UPDATED,
           'gem_version2_updated_key': GEM_VERSION_TWO_WITH_UPDATED_FIELD,
           'gem_with_no_version_key': GEM_WITH_NO_VERSION,
           'gem_archive_json_key': TEST_GEM_JSON_PAYLOAD,
           'project_json_key': TEST_PROJECT_JSON_PAYLOAD,
           'template_json_key': TEST_TEMPLATE_JSON_PAYLOAD}

# class returns a dictionary of the template repo json
@pytest.fixture(scope='function')
def init_repo_json_data(request):
    class RepoJsonData:
        def __init__(self):
            self.data = json.loads(TEST_TEMPLATE_REPO_JSON)
            self.path = None
            self.backup_file_name = None
    request.cls.repo_json = RepoJsonData()

@pytest.fixture(scope="session")
def temp_folder(tmpdir_factory):
    temp_path = pathlib.Path(tmpdir_factory.mktemp('temp'))
    gem_path = temp_path / 'gem'
    os.makedirs(gem_path)
    gem_json_path = gem_path / 'gem.json'
    with gem_json_path.open('w') as s:
        s.write(json.dumps(TEST_GEM_JSON_PAYLOAD, indent=4) + '\n')

    os.makedirs(temp_path / 'release')

    # Yield the temporary path to the test and remove
    yield temp_path
    shutil.rmtree(temp_path)

# use the TEST_TEMPLATE_REPO_JSON created above as the repo json template
@pytest.mark.usefixtures('init_repo_json_data')

class TestEditRepoProperties:

    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize("repo_path, repo_name, \
                              add_gems, delete_gems, replace_gems, expected_gems, \
                              add_projects, delete_projects, replace_projects, expected_projects, \
                              add_templates, delete_templates, replace_templates, expected_templates, \
                              auto_update, dry_run, release_archive_path, force, download_prefix, \
                              expected_result", [
            # test add gems
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1',
                         'gem1 gem3', None, None, [{'gem_name':'gem1'}, {'gem_name':'gem3'}],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test remove a gem
            pytest.param(pathlib.Path('D:/repotest'),'Repotest2',
                         'gem1 gem2', 'gem2', None, [{'gem_name':'gem1'}],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test replace with multiple gems
            pytest.param(pathlib.Path('F:/repotest/AFH'),'Repotest3',
                         'gem1 gem2', None, 'gem3 gem4', [{'gem_name':'gem3'}, {'gem_name':'gem4'}],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test replace a gem with an empty parameter
            pytest.param(pathlib.Path('F:/repotest'),'Repo',
                         'gem1 gem2', None, '', [{'gem_name':'gem1'}, {'gem_name':'gem2'}],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test add projects
            pytest.param(pathlib.Path('F:/repotest'),'test',
                         None, None, None, None,
                         'project1 project2', None, None, [{'project_name':'project1'}, {'project_name':'project2'}],
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test remove a project
            pytest.param(pathlib.Path('F:/repotest'),'random',
                         None, None, None, None,
                         'project1 project2 project3', 'project2', None, [{'project_name':'project1'}, {'project_name':'project3'}],
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test replace a project
            pytest.param(pathlib.Path('F:/repotest'),'RepoReplace',
                         None, None, None, None,
                         'project1 project2', None, 'project3', [{'project_name':'project3'}],
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test remove project with a empty parameter
            pytest.param(pathlib.Path('F:/repotest'),'RepoRemove', 
                         None, None, None, None,
                         'project1 project2', '', None, [{'project_name':'project1'}, {'project_name':'project2'}],
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # test add template
            pytest.param(pathlib.Path('F:/repotest'),'RepoAdd',
                         None, None, None, None,
                         None, None, None, None,
                         'template1 template2 template3', None, None, [{'template_name':'template1'}, {'template_name':'template2'}, 
                                                                       {'template_name':'template3'}],
                         None, None, None, None, None,
                         0),
            # test remove multiple templates
            pytest.param(pathlib.Path('F:/repotest'),'removeTemplate',
                         None, None, None, None,
                         None, None, None, None,
                         'template1 template2 template3', 'template1 template2', None, [{'template_name':'template3'}],
                         None, None, None, None, None,
                         0),
            # test replace a temlpate
            pytest.param(pathlib.Path('F:/repotest'),'replaceTemp',
                         None, None, None, None,
                         None, None, None, None,
                         'template1 template2 template3', None, "template48", [{'template_name':'template48'}],
                         None, None, None, None, None,
                         0),
            # test add all objects
            pytest.param(pathlib.Path('F:/repotest'),'addAll',
                         'gem1', None, None, [{'gem_name':'gem1'}],
                         'project1', None, None, [{'project_name':'project1'}],
                         'template1 template2', None, None, [{'template_name':'template1'}, {'template_name':'template2'}],
                         None, None, None, None, None,
                         0),
            # test remove all object types
            pytest.param(pathlib.Path('F:/repotest'),'removeAll',
                         'gem1 gem2', 'gem1', None, [{'gem_name':'gem2'}],
                         'project1', 'project1', None, [],
                         'template1 template2 template3', 'template1 template3', None, [{'template_name':'template2'}],
                         None, None, None, None, None,
                         0),
            # Adding a gem with version 1.0.0 and then the same gem with version 2.0.0
            pytest.param(pathlib.Path('F:/repotest'),'GemAddVersion',
                         'gem_version1_json_key gem_version2_json_key', None, None, [{"gem_name": "TestGem",
                                                                                      "version": "1.0.0",
                                                                                      "versions_data": [{"version": "2.0.0"}]
                                                                                    }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem with version 1.0.0 and then the same gem with version 2.0.0 and then removing version 2.0.0             
            pytest.param(pathlib.Path('F:/repotest'),'GemAddAndRemoveVersion',
                         'gem_version1_json_key gem_version2_json_key', 'TestGem==2.0.0', None, [{"gem_name": "TestGem",
                                                                                                  "version": "1.0.0",
                                                                                                  'versions_data': []
                                                                                                }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem with version 1.0.0 and then the same gem with version 2.0.0 and then removing version 1.0.0
            pytest.param(pathlib.Path('F:/repotest'),'GemAddAndRemoveBaseVersion',
                         'gem_version1_json_key gem_version2_json_key', 'TestGem==1.0.0', None, [{"gem_name": "TestGem",
                                                                                                  "version": "2.0.0",
                                                                                                  'versions_data': []
                                                                                                }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem with version 1.0.0 and then the same gem with version 2.0.0 and then removing version 1.0.0 and 2.0.0
            pytest.param(pathlib.Path('F:/repotest'),'GemAddAndRemoveAllVersion',
                         'gem_version1_json_key gem_version2_json_key', 'TestGem==1.0.0 TestGem==2.0.0', None, [],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem with version 1.0.0 and then the same gem with the same version but other updated gem.json fields
            pytest.param(pathlib.Path('F:/repotest'),'GemAddSameVersionWithUpdateField',
                         'gem_version1_tobe_updated_key gem_version1_with_updated_field_key', None, None, [{'gem_name': 'TestGemUpdate',
                                                                                                            'version': '1.0.0',
                                                                                                            'display_name': 'hiWorld'
                                                                                                            }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem with version 1.0.0 and then the same gem with version 2.0.0 and then adding version 2.0.0 with updated gem.json fields
            pytest.param(pathlib.Path('F:/repotest'),'GemAddSameVersionWithUpdateField',
                         'gem_version1_json_key gem_version2_tobe_updated_key gem_version2_updated_key', None, None, [{'gem_name': 'TestGem',
                                                                                                                       'version': '1.0.0',
                                                                                                                       'versions_data': [{
                                                                                                                           'version': '2.0.0',
                                                                                                                           'display_name': 'hiWorld'
                                                                                                                           }]
                                                                                                                        }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem with multiple versions and then replacing all gems with another set of gems             
            pytest.param(pathlib.Path('F:/repotest'),'ReplaceGem',
                         'gem_version1_json_key gem_version2_json_key', None, 'gem_version3_json_key', [{'gem_name': 'TestGem',
                                                                                                         'version': '3.0.0'}],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Deleting a gem with version>2.0.0
            pytest.param(pathlib.Path('F:/repotest'),'DeletingGemVersion',
                         'gem_version1_json_key gem_version2_json_key gem_version3_json_key', 'TestGem>2.0.0', None, [{'gem_name': 'TestGem',
                                                                                                                       'version': '1.0.0',
                                                                                                                       'versions_data': [{
                                                                                                                           'version': '2.0.0'
                                                                                                                           }]
                                                                                                                        }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Adding a gem without a version, then adding a gem with the same name but with a version
            pytest.param(pathlib.Path('F:/repotest'),'GemAddSameVersionWithUpdateField',
                         'gem_with_no_version_key gem_version1_json_key', None, None, [{'gem_name': 'TestGem',
                                                                                        'versions_data': [{
                                                                                            'version': '1.0.0'}]
                                                                                        }],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None, None,
                         0),
            # Add a gem and create an archive
            pytest.param(pathlib.Path('F:/repotest'),'testArchive',
                         'na', None, None, [JSON_DICT['gem_archive_json_key']],
                         None, None, None, None,
                         None, None, None, None,
                         None, None, 'na', None, 'http://testGemPath', 
                         0),
            # Test Auto update a gem 
            pytest.param(pathlib.Path('F:/repotest'),'testAutoUpdateGem',
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None,
                         'gem', None, None, None, None,
                         0),
            # Test Auto update a project 
            pytest.param(pathlib.Path('F:/repotest'),'testAutoUpdateProject',
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None,
                         'project', None, None, None, None,
                         0),
            # Test Auto update a template 
            pytest.param(pathlib.Path('F:/repotest'),'testAutoUpdateTemplate',
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None,
                         'template', None, None, None, None,
                         0),
            # Test dry run for auto update gems 
            pytest.param(pathlib.Path('F:/repotest'),'testAutoUpdateAll',
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None,
                         'gem project template', None, None, None, None,
                         0),
            # Test dry-run on auto update all objects
            pytest.param(pathlib.Path('F:/repotest'),'repotest',
                         None, None, None, None,
                         None, None, None, None,
                         None, None, None, None,
                         'dry-run', True, None, None, None,
                         0),
            ]
    ) 
    # Where actual test starts
    def test_edit_repo_obj_version(self, temp_folder, repo_path, repo_name, add_gems, delete_gems, replace_gems, expected_gems,
                                     add_projects, delete_projects, replace_projects, expected_projects, 
                                     add_templates, delete_templates, replace_templates, expected_templates,
                                     auto_update, dry_run, release_archive_path, force, download_prefix,  
                                     expected_result):
        def backup_file(file_name: str or pathlib.Path) -> None:
            self.backup_file_name = file_name

        def get_repo_props(repo_path: str or pathlib.Path = None) -> dict or None:
            if not repo_path:
                self.repo_json.data = None
                return None
            return self.repo_json.data

        def save_o3de_manifest(json_data: dict, manifest_path: pathlib.Path = None) -> bool:
            self.repo_json.data = json_data
            return True

        def get_json_data(object_typename: str,
                    object_path: str or pathlib.Path,
                    object_validator: callable) -> dict or None:
            if object_path in JSON_DICT:
                return JSON_DICT[object_path].copy()
            if isinstance(object_path, pathlib.Path):
                #add_gem json data
                return JSON_DICT['gem_archive_json_key']
            # if object json template doesn't exist 
            if object_typename == "gem":
                return {'gem_name': object_path}
            if object_typename == "project":
                return {'project_name': object_path}
            if object_typename == "template":
                return {'template_name': object_path}
            
        def _auto_update_json(object_type: str or list = None,
                      repo_path: pathlib.Path = None,
                      repo_json: dict = None):
            objects = object_type.split() if isinstance(object_type, str) else object_type
            for object_type in objects:
                if object_type == 'gem':
                    self.repo_json.data.get('gems_data').append(str(JSON_DICT['gem_archive_json_key']))
                if object_type == 'project':
                    self.repo_json.data.get('projects_data').append(str(JSON_DICT['project_json_key']))
                if object_type == 'template':
                    self.repo_json.data.get('templates_data').append(str(JSON_DICT['template_json_key']))
                if object_type == 'dry-run':
                    self.repo_json.data.get('gems_data').append(json.loads(TEST_GEM_JSON_STR))
                    self.repo_json.data.get('projects_data').append(json.loads(TEST_PROJECT_JSON_STR))
                    self.repo_json.data.get('templates_data').append(json.loads(TEST_TEMPLATE_JSON_STR))
            return 0
        
        def mock_dryrun_print(msg, *args, **kwargs):
            if dry_run:
                assert DRY_RUN_EXPECTED == msg
        
        with patch('o3de.repo_properties.get_repo_props', side_effect=get_repo_props) as get_repo_props_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch, \
                patch('o3de.utils.backup_file', side_effect=backup_file) as backup_file_patch, \
                patch('o3de.repo_properties._auto_update_json', side_effect=_auto_update_json) as _auto_update_json_patch, \
                patch('o3de.manifest.get_json_data', side_effect=get_json_data) as get_json_data_patch, \
                patch('logging.Logger.warning', side_effect=mock_dryrun_print) as logger_warning_patch:
            if release_archive_path:
                add_gems = [temp_folder / 'gem']
                release_archive_path = temp_folder / 'release'
            result = repo_properties.edit_repo_props(repo_path, repo_name, add_gems, delete_gems, replace_gems,
                                                     add_projects, delete_projects, replace_projects, 
                                                     add_templates, delete_templates, replace_templates, 
                                                     auto_update, dry_run, release_archive_path, force, download_prefix)
            assert result == expected_result
            if result == 0:
                assert self.repo_json.data.get('repo_name') == repo_name
                assert self.repo_json.data.get('repo_uri') == 'https://test.com'
                assert self.repo_json.data.get('$schemaVersion') == '1.0.0'
                assert self.repo_json.data.get('origin') == 'o3de'
                if dry_run:
                    # assert true here because actual assert will be inside mock_dryrun_print
                    assert True
                else:
                    assert self.backup_file_name == repo_path / 'repo.json'
                    for gem in self.repo_json.data.get('gems_data', []):
                        if release_archive_path:
                            zip_path = release_archive_path / pathlib.Path(gem['download_source_uri']).name
                            download.validate_downloaded_zip_sha256(gem, zip_path, "gem.json")
                        elif auto_update:
                            assert gem in str(JSON_DICT.get('gem_archive_json_key'))
                        else:
                            assert gem in expected_gems
                    for project in self.repo_json.data.get('projects_data', []):
                        if auto_update:
                            assert project in str(JSON_DICT.get('project_json_key'))
                        else:
                            assert project in expected_projects
                    for template in self.repo_json.data.get('templates_data', []):
                        if auto_update:
                            assert template in str(JSON_DICT.get('template_json_key'))
                        else:
                            assert template in expected_templates
