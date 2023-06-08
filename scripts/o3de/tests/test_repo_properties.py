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

from o3de import repo_properties

TEST_TEMPLATE_REPO_JSON = '''
{
    "repo_name": "repotest",
    "repo_uri": "https://test",
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
# class returns a dictionary of the template repo json
@pytest.fixture(scope='function')
def init_repo_json_data(request):
    class RepoJsonData:
        def __init__(self):
            self.data = json.loads(TEST_TEMPLATE_REPO_JSON)
            self.path = None
    request.cls.repo_json = RepoJsonData()

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
                              expected_result", [
            # test add gems
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         'gem1 gem3', None, None, [{'gem_name':'gem1'}, {'gem_name':'gem3'}],
                         None, None, None, None,
                         None, None, None, None,
                         0),
            # test remove a gem
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         'gem1 gem2', 'gem2', None, [{'gem_name':'gem1'}],
                         None, None, None, None,
                         None, None, None, None,
                         0),
            # test replace with multiple gems
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         'gem1 gem2', None, 'gem3 gem4', [{'gem_name':'gem3'}, {'gem_name':'gem4'}],
                         None, None, None, None,
                         None, None, None, None,
                         0),
            # test replace a gem with an empty parameter
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         'gem1 gem2', None, '', [{'gem_name':'gem1'}, {'gem_name':'gem2'}],
                         None, None, None, None,
                         None, None, None, None,
                         0),
            # test add projects
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         'project1 project2', None, None, [{'project_name':'project1'}, {'project_name':'project2'}],
                         None, None, None, None,
                         0),
            # test remove a project
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         'project1 project2 project3', 'project2', None, [{'project_name':'project1'}, {'project_name':'project3'}],
                         None, None, None, None,
                         0),
            # test replace a project
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         'project1 project2', None, 'project3', [{'project_name':'project3'}],
                         None, None, None, None,
                         0),
            # test remove project with a empty parameter
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         'project1 project2', '', None, [{'project_name':'project1'}, {'project_name':'project2'}],
                         None, None, None, None,
                         0),
            # test add template
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         None, None, None, None,
                         'template1 template2 template3', None, None, [{'template_name':'template1'}, {'template_name':'template2'}, 
                                                                       {'template_name':'template3'}],
                         0),
            # test remove multiple templates
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         None, None, None, None,
                         'template1 template2 template3', 'template1 template2', None, [{'template_name':'template3'}],
                         0),
            # test replace a temlpate
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         None, None, None, None,
                         None, None, None, None,
                         'template1 template2 template3', None, "template48", [{'template_name':'template48'}],
                         0),
            # test add all objects
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         'gem1', None, None, [{'gem_name':'gem1'}],
                         'project1', None, None, [{'project_name':'project1'}],
                         'template1 template2', None, None, [{'template_name':'template1'}, {'template_name':'template2'}],
                         0),
            # test remove all object types
            pytest.param(pathlib.Path('F:/repotest'),'Repotest1', 
                         'gem1 gem2', 'gem1', None, [{'gem_name':'gem2'}],
                         'project1', 'project1', None, [],
                         'template1 template2 template3', 'template1 template3', None, [{'template_name':'template2'}],
                         0)
            ]
    ) 
    # Where actual test starts
    def test_edit_repo_properties(self, repo_path, repo_name, add_gems, delete_gems, replace_gems, expected_gems,
                                     add_projects, delete_projects, replace_projects, expected_projects, 
                                     add_templates, delete_templates, replace_templates, expected_templates, 
                                     expected_result):
        
        def get_repo_props(repo_path: pathlib.Path) -> dict or None:
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
            
            if object_typename == "gem":
                return {'gem_name': object_path}
            if object_typename == "project":
                return {'project_name': object_path}
            if object_typename == "template":
                return {'template_name': object_path}

        with patch('o3de.repo_properties.get_repo_props', side_effect=get_repo_props) as get_repo_props_patch, \
                patch('o3de.manifest.save_o3de_manifest', side_effect=save_o3de_manifest) as save_o3de_manifest_patch, \
                patch('o3de.manifest.get_json_data', side_effect=get_json_data) as get_json_data_patch:
            result = repo_properties.edit_repo_props(repo_path, repo_name, add_gems, delete_gems, replace_gems,
                                                     add_projects, delete_projects, replace_projects, 
                                                     add_templates, delete_templates, replace_templates)
            assert result == expected_result
            if result == 0:
                assert self.repo_json.data.get('repo_name') == repo_name
                assert self.repo_json.data.get('repo_uri') == 'https://test'
                assert self.repo_json.data.get('$schemaVersion') == '1.0.0'
                assert self.repo_json.data.get('origin') == 'o3de'

                for gem in self.repo_json.data.get('gems_data', []):
                    assert gem in expected_gems
                for project in self.repo_json.data.get('projects_data', []):
                    assert project in expected_projects
                for template in self.repo_json.data.get('templates_data', []):
                    assert template in expected_templates
              
