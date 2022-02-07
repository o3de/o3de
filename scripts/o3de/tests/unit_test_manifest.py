#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import logging
import pytest
import pathlib
from unittest.mock import patch

from o3de import manifest


@pytest.mark.parametrize("valid_project_json_paths, valid_gem_json_paths", [
    pytest.param([pathlib.Path('D:/o3de/Templates/DefaultProject/Template/project.json')],
                 [pathlib.Path('D:/o3de/Templates/DefaultGem/Template/gem.json')])
])
class TestGetTemplatesForCreation:
    @staticmethod
    def get_templates() -> list:
        return []

    @staticmethod
    def get_project_templates() -> list:
        return []

    @staticmethod
    def get_engine_templates() -> list:
        return [pathlib.Path('D:/o3de/Templates/DefaultProject'), pathlib.Path('D:/o3de/Templates/DefaultGem')]


    @pytest.mark.parametrize("expected_template_paths", [
            pytest.param([])
        ]
    )
    def test_get_templates_for_generic_creation(self, valid_project_json_paths, valid_gem_json_paths,
                                                expected_template_paths):
        def validate_project_json(template_path) -> bool:
            return pathlib.Path(template_path) in valid_project_json_paths

        def validate_gem_json(template_path) -> bool:
            return pathlib.Path(template_path) in valid_gem_json_paths

        with patch('o3de.manifest.get_templates', side_effect=self.get_templates) as get_templates_patch, \
                patch('o3de.manifest.get_project_templates', side_effect=self.get_project_templates)\
                        as get_project_templates_patch, \
                patch('o3de.manifest.get_engine_templates', side_effect=self.get_engine_templates)\
                        as get_engine_templates_patch, \
            patch('o3de.validation.valid_o3de_template_json', return_value=True) as validate_template_json,\
            patch('o3de.validation.valid_o3de_project_json', side_effect=validate_project_json) as validate_project_json,\
            patch('o3de.validation.valid_o3de_gem_json', side_effect=validate_gem_json) as validate_gem_json:
            templates = manifest.get_templates_for_generic_creation()
            assert templates == expected_template_paths


    @pytest.mark.parametrize("expected_template_paths", [
            pytest.param([pathlib.Path('D:/o3de/Templates/DefaultProject')])
        ]
    )
    def test_get_templates_for_gem_creation(self, valid_project_json_paths, valid_gem_json_paths,
                                                expected_template_paths):
        def validate_project_json(template_path) -> bool:
            return pathlib.Path(template_path) in valid_project_json_paths

        def validate_gem_json(template_path) -> bool:
            return pathlib.Path(template_path) in valid_gem_json_paths

        with patch('o3de.manifest.get_templates', side_effect=self.get_templates) as get_templates_patch, \
                patch('o3de.manifest.get_project_templates', side_effect=self.get_project_templates) \
                        as get_project_templates_patch, \
                patch('o3de.manifest.get_engine_templates', side_effect=self.get_engine_templates) \
                        as get_engine_templates_patch, \
                patch('o3de.validation.valid_o3de_template_json', return_value=True) as validate_template_json, \
                patch('o3de.validation.valid_o3de_project_json',
                      side_effect=validate_project_json) as validate_project_json, \
                patch('o3de.validation.valid_o3de_gem_json', side_effect=validate_gem_json) as validate_gem_json:
            templates = manifest.get_templates_for_project_creation()
            assert templates == expected_template_paths


    @pytest.mark.parametrize("expected_template_paths", [
            pytest.param([pathlib.Path('D:/o3de/Templates/DefaultGem')])
        ]
    )
    def test_get_templates_for_project_creation(self, valid_project_json_paths, valid_gem_json_paths,
                                                expected_template_paths):
        def validate_project_json(template_path) -> bool:
            return pathlib.Path(template_path) in valid_project_json_paths

        def validate_gem_json(template_path) -> bool:
            return pathlib.Path(template_path) in valid_gem_json_paths

        with patch('o3de.manifest.get_templates', side_effect=self.get_templates) as get_templates_patch, \
                patch('o3de.manifest.get_project_templates', side_effect=self.get_project_templates) \
                        as get_project_templates_patch, \
                patch('o3de.manifest.get_engine_templates', side_effect=self.get_engine_templates) \
                        as get_engine_templates_patch, \
                patch('o3de.validation.valid_o3de_template_json', return_value=True) as validate_template_json, \
                patch('o3de.validation.valid_o3de_project_json',
                      side_effect=validate_project_json) as validate_project_json, \
                patch('o3de.validation.valid_o3de_gem_json', side_effect=validate_gem_json) as validate_gem_json:
            templates = manifest.get_templates_for_gem_creation()
            assert templates == expected_template_paths