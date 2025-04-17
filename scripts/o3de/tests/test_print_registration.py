#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import pytest
import pathlib
from unittest.mock import patch

from o3de import print_registration


TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "MinimalProject",
    "version": "0.0.0",
    "origin": "The primary repo for MinimalProject goes here: i.e. http://www.mydomain.com",
    "license": "What license MinimalProject uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "MinimalProject",
    "summary": "A short description of MinimalProject.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [
        "MinimalProject"
    ],
    "icon_path": "preview.png",
    "engine": "o3de-install",
    "external_subdirectories": [
        "D:/TestGem"
    ]
}
'''

TEST_ENGINE_JSON_PAYLOAD = '''
{
    "engine_name": "o3de",
    "version": "0.0.0",
    "restricted_name": "o3de",
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
    ]
}
'''

TEST_GEM_JSON_PAYLOAD = '''
{
    "gem_name": "TestGem",
    "version": "0.0.0",
    "display_name": "TestGem",
    "license": "What license TestGem uses goes here: i.e. https://opensource.org/licenses/MIT",
    "origin": "The primary repo for TestGem goes here: i.e. http://www.mydomain.com",
    "type": "Code",
    "summary": "A short description of TestGem.",
    "canonical_tags": [
        "Gem"
    ],
    "user_tags": [
        "TestGem"
    ],
    "icon_path": "preview.png",
    "requirements": ""
}
'''

TEST_TEMPLATE_JSON_PAYLOAD = '''
{
    "template_name": "AssetGem",
    "origin": "The primary repo for AssetGem goes here: i.e. http://www.mydomain.com",
    "license": "What license AssetGem uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "AssetGem",
    "summary": "A short description of AssetGem template.",
    "canonical_tags": [],
    "user_tags": [
        "AssetGem"
    ],
    "icon_path": "preview.png",
    "copyFiles": [
        {
            "file": "CMakeLists.txt",
            "isTemplated": true
        },
        {
            "file": "gem.json",
            "isTemplated": true
        },
        {
            "file": "preview.png",
            "isTemplated": false
        }
    ],
    "createDirectories": [
        {
            "dir": "Assets"
        }
    ]
}
'''

TEST_RESTRICTED_JSON_PAYLOAD = '''
{
    "restricted_name": "o3de"
}
'''

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
    "projects": [
        "D:/MinimalProject"
    ],
    "external_subdirectories": [],
    "templates": [],
    "restricted": [],
    "repos": [],
    "engines": [
        "D:/o3de/o3de"
    ]
}
'''

class TestPrintRegistration:
    @staticmethod
    def load_manifest_json():
        return json.loads(TEST_O3DE_MANIFEST_JSON_PAYLOAD)

    @staticmethod
    def get_engine_json_data(engine_path: pathlib.Path = None):
        return json.loads(TEST_ENGINE_JSON_PAYLOAD)

    @staticmethod
    def get_project_json_data(project_name: str = None,
                            project_path: str or pathlib.Path = None,
                            user: bool = False) -> dict or None:
        return json.loads(TEST_PROJECT_JSON_PAYLOAD)

    @staticmethod
    def get_gem_json_data(gem_path: pathlib.Path = None):
        return json.loads(TEST_GEM_JSON_PAYLOAD)

    @staticmethod
    def get_template_json_data(template_path: pathlib.Path = None):
        return json.loads(TEST_TEMPLATE_JSON_PAYLOAD)

    @staticmethod
    def get_restricted_json_data(restricted_path: pathlib.Path = None):
        return json.loads(TEST_RESTRICTED_JSON_PAYLOAD)

    @pytest.mark.parametrize("project_path, verbose", [
        pytest.param(None, 0),
        pytest.param(None, 1),
        pytest.param(pathlib.Path("D:/MinimalProject"), 0),
        pytest.param(pathlib.Path("D:/MinimalProject"), 1)
    ])
    def test_print_registration_no_option(self, project_path, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = []
        if project_path:
            arg_list += ['--project-path', project_path.as_posix()]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)

        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_engine_json_data',
                      side_effect=self.get_engine_json_data) as get_engine_json_data_patch, \
                patch('o3de.manifest.get_project_json_data',
                      side_effect=self.get_project_json_data) as get_project_json_patch, \
                patch('o3de.manifest.get_gem_json_data', side_effect=self.get_gem_json_data) as get_gem_json_patch, \
                patch('o3de.manifest.get_template_json_data', side_effect=self.get_template_json_data) as get_template_json_patch, \
                patch('o3de.manifest.get_restricted_json_data', side_effect=self.get_restricted_json_data) as get_json_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0


    @pytest.mark.parametrize("engine_arg_option, verbose", [
        pytest.param("--this-engine", 0),
        pytest.param("--this-engine", 1),
        pytest.param("--engines", 0),
        pytest.param("--engines", 1)
    ])
    def test_print_engine_registration(self, engine_arg_option, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [engine_arg_option]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)


        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_engine_json_data', side_effect=self.get_engine_json_data) as get_engine_json_data_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0


    @pytest.mark.parametrize("arg_option, verbose", [
        pytest.param("--projects", 0),
        pytest.param("--projects", 1),
        pytest.param("--engine-projects", 0),
        pytest.param("--engine-projects", 1),
        pytest.param("--all-projects", 0),
        pytest.param("--all-projects", 1)
    ])
    def test_print_project_registration(self, arg_option, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [arg_option]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)


        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_project_json_data', side_effect=self.get_project_json_data) as get_json_data_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0


    @pytest.mark.parametrize("arg_option, project_path, verbose", [
        pytest.param("--gems", None, 0),
        pytest.param("--gems", None, 1),
        pytest.param("--engine-gems", None, 0),
        pytest.param("--engine-gems", None, 1),
        pytest.param("--project-gems", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--project-gems", pathlib.Path("D:/MinimalProject"), 1),
        pytest.param("--all-gems", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--all-gems", None, 0),
        pytest.param("--all-gems", pathlib.Path("D:/MinimalProject"), 1),
        pytest.param("--all-gems", None, 1)
    ])
    def test_print_gem_registration(self, arg_option, project_path, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [arg_option]
        if project_path:
            arg_list += ['--project-path', project_path.as_posix()]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)

        # Patch the manifest.py function to locate gem.json files in external subdirectories
        # to just return a fake path to a single test gem
        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_gem_json_data', side_effect=self.get_gem_json_data) as get_json_patch, \
                patch('o3de.manifest.get_project_json_data', side_effect=self.get_project_json_data) as get_project_json_patch, \
                patch('o3de.print_registration.get_project_path', return_value=project_path) as get_project_path_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0


    @pytest.mark.parametrize("arg_option, project_path, verbose", [
        pytest.param("--templates", None, 0),
        pytest.param("--templates", None, 1),
        pytest.param("--engine-templates", None, 0),
        pytest.param("--engine-templates", None, 1),
        pytest.param("--project-templates", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--project-templates", pathlib.Path("D:/MinimalProject"), 1),
        pytest.param("--all-templates", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--all-templates", None, 0),
        pytest.param("--all-templates", pathlib.Path("D:/MinimalProject"), 1),
        pytest.param("--all-templates", None, 1)
    ])
    def test_print_template_registration(self, arg_option, project_path, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [arg_option]
        if project_path:
            arg_list += ['--project-path', project_path.as_posix()]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)

        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_template_json_data', side_effect=self.get_template_json_data) as get_json_patch, \
                patch('o3de.manifest.get_project_json_data', side_effect=self.get_project_json_data) as get_project_json_patch, \
                patch('o3de.print_registration.get_project_path', return_value=project_path) as get_project_path_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0


    @pytest.mark.parametrize("arg_option, project_path, verbose", [
        pytest.param("--restricted", None, 0),
        pytest.param("--restricted", None, 1),
        pytest.param("--engine-restricted", None, 0),
        pytest.param("--engine-restricted", None, 1),
        pytest.param("--project-restricted", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--project-restricted", pathlib.Path("D:/MinimalProject"), 1),
        pytest.param("--all-restricted", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--all-restricted", None, 0),
        pytest.param("--all-restricted", pathlib.Path("D:/MinimalProject"), 1),
        pytest.param("--all-restricted", None, 1)
    ])
    def test_print_restricted_registration(self, arg_option, project_path, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [arg_option]
        if project_path:
            arg_list += ['--project-path', project_path.as_posix()]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)

        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_restricted_json_data', side_effect=self.get_restricted_json_data) as get_json_patch, \
                patch('o3de.manifest.get_project_json_data', side_effect=self.get_project_json_data) as get_project_json_patch, \
                patch('o3de.print_registration.get_project_path', return_value=project_path) as get_project_path_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0


    # Setting --verbose with the --*external-subdirectories option doesn't result in any additional output
    # So it is only parameterized as 0
    @pytest.mark.parametrize("arg_option, project_path, verbose", [
        pytest.param("--external-subdirectories", None, 0),
        pytest.param("--engine-external-subdirectories", None, 0),
        pytest.param("--project-external-subdirectories", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--all-external-subdirectories", pathlib.Path("D:/MinimalProject"), 0),
        pytest.param("--all-external-subdirectories", None, 0),
    ])
    def test_print_external_subdirectories_registration(self, arg_option, project_path, verbose):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [arg_option]
        if project_path:
            arg_list += ['--project-path', project_path.as_posix()]
        if verbose:
            arg_list += ['-' + 'v' * verbose]
        test_args = parser.parse_args(arg_list)

        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_project_json_data',
                      side_effect=self.get_project_json_data) as get_project_json_patch, \
                patch('o3de.print_registration.get_project_path', return_value=project_path) as get_project_path_patch:
            result = print_registration._run_register_show(test_args)
            assert result == 0

    # No --verbose for the -project-engine-name option doesn't produce more output.
    @pytest.mark.parametrize("arg_option, project_path, expected_result", [
        pytest.param("--project-engine-name", None, 1),
        pytest.param("--project-engine-name", pathlib.Path("D:/MinimalProject"), 0),
    ])
    def test_print_external_subdirectories_registration(self, arg_option, project_path, expected_result):
        parser = argparse.ArgumentParser()

        # Register the registration script subparsers with the current argument parser
        print_registration.add_parser_args(parser)
        arg_list = [arg_option]
        if project_path:
            arg_list += ['--project-path', project_path.as_posix()]
        test_args = parser.parse_args(arg_list)

        with patch('o3de.manifest.load_o3de_manifest', side_effect=self.load_manifest_json) as load_manifest_patch, \
                patch('o3de.manifest.save_o3de_manifest', return_value=True) as save_manifest_patch, \
                patch('o3de.manifest.get_project_json_data',
                      side_effect=self.get_project_json_data) as get_project_json_patch, \
                patch('o3de.print_registration.get_project_path', return_value=project_path) as get_project_path_patch:
            result = print_registration._run_register_show(test_args)
            assert result == expected_result
