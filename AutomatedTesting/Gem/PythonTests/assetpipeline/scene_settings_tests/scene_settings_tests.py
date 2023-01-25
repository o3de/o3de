"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import shutil
import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest
import tempfile


def cleanup_test_files(workspace, test_file_names):
    for test_file_name in test_file_names:
        file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", test_file_name)],
                            True, True)
        file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", test_file_name +".assetinfo")],
                            True, True)


def setup_test_files(workspace, test_file_names):
    cleanup_test_files(workspace, test_file_names)
    for test_file_name in test_file_names:
        test_file_source = os.path.join(os.path.dirname(os.path.abspath(__file__)), test_file_name)
        test_file_destination = os.path.join(workspace.paths.engine_root(), "AutomatedTesting", test_file_name)
        shutil.copyfile(test_file_source, test_file_destination)


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    global_extra_cmdline_args = []

    class scene_settings_tests_in_editor(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_names = ["auto_test_fbx.fbx"]
            setup_test_files(workspace, self.test_file_names)

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_file_names)

        from .tests import scene_settings_tests_in_editor as test_module
        
    class scene_settings_clear_unsaved_changes(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_names = ["auto_test_fbx.fbx"]
            setup_test_files(workspace, self.test_file_names)
        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_file_names)

        from .tests import scene_settings_clear_unsaved_changes as test_module

    class scene_settings_max_prefab_groups_is_one(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_names = ["auto_test_fbx.fbx"]
            setup_test_files(workspace, self.test_file_names)
        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_file_names)

        from .tests import scene_settings_max_prefab_groups_is_one as test_module


    class scene_settings_tests_readonly_rule(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_names = ["auto_test_fbx.fbx"]
            setup_test_files(workspace, self.test_file_names)

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_file_names)

        from .tests import scene_settings_readonly_rule_test as test_module
        

    class scene_settings_procedural_mesh_groups_test(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_names = ["auto_test_fbx.fbx"]
            setup_test_files(workspace, self.test_file_names)

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_file_names)

        from .tests import scene_settings_procedural_mesh_groups_test as test_module


    class scene_settings_manifest_vector_widget_tests_in_editor(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_names = ["Jack_Death_Fall_Back_ZUp.fbx", "Jack_Death_Fall_Back_ZUp.fbx.assetinfo"]
            setup_test_files(workspace, self.test_file_names)

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_file_names)

        from .tests import scene_settings_manifest_vector_widget_tests_in_editor as test_module
