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

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    def cleanup_test_file(self, workspace, test_file_name):
        file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", test_file_name)],
                           True, True)
        file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", test_file_name +".assetinfo")],
                           True, True)

    class scene_settings_tests_in_editor(EditorSingleTest):
        from .tests import scene_settings_tests_in_editor as test_module
        

        @classmethod
        def setup(self, instance, request, workspace):
            self.test_file_name = "auto_test_fbx.fbx"
            TestAutomation.cleanup_test_file(self, workspace, self.test_file_name)

            test_file_source = os.path.join(os.path.dirname(os.path.abspath(__file__)), self.test_file_name)
            test_file_destination = os.path.join(workspace.paths.engine_root(), "AutomatedTesting", self.test_file_name)
            shutil.copyfile(test_file_source, test_file_destination)

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            TestAutomation.cleanup_test_file(self, workspace, self.test_file_name)
