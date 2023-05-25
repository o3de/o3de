"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import shutil
import zipfile

import pytest
from ly_test_tools.environment import file_system
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest


def cleanup_test_files(workspace, test_file_names: list[str]):
    for test_file_name in test_file_names:
        file_system.delete(test_file_name, True, True)


def to_meta_file(file: str):
    return file + ".meta"


def to_material_file(file: str):
    return file + ".material"


def original_file_set(folder: str, file: str):
    files = [os.path.join(folder, to_material_file(file)), os.path.join(folder, to_meta_file(to_material_file(file)))]
    return files


def renamed_file_set(folder: str, file: str):
    file = file + "_renamed"
    files = original_file_set(folder, file)
    return files


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])  # This test works on Windows editor
@pytest.mark.parametrize("project", ["AutomatedTesting"])  # Use AutomatedTesting project
class TestAutomation(EditorTestSuite):
    class Check_MetadataRename_test(EditorSingleTest):
        test_files: list[str] = None

        @classmethod
        def setup(self, instance, request, workspace):
            test_file_name = "bunny_material"
            original_files = original_file_set(os.path.join(os.path.dirname(os.path.realpath(__file__)), "assets"), test_file_name)
            renamed_files = renamed_file_set(os.path.dirname(os.path.realpath(__file__)), test_file_name)
            self.test_files = renamed_files

            cleanup_test_files(workspace, self.test_files)

            destination_folder = os.path.join(workspace.paths.engine_root(), "AutomatedTesting")

            # Move/rename both files.  The point of the test is to verify references are still valid after this rename
            for i, val in enumerate(original_files):
                shutil.copyfile(val, renamed_files[i])

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_files)

        from .tests import MetadataRelocation_ReferenceValidAfterRename as test_module
