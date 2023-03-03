import os
import shutil
import zipfile

import pytest
from ly_test_tools.environment import file_system
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest


def cleanup_test_files(workspace, test_file_names):
    for test_file_name in test_file_names:
        file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", test_file_name)],
                           True, True)


def to_meta_file(file):
    return file + ".meta"


def to_material_file(file):
    return file + ".material"


def original_file_set(file):
    set = [to_material_file(file), to_meta_file(to_material_file(file))]
    return set


def renamed_file_set(file):
    file = file + "_renamed"
    set = original_file_set(file)
    return set


@pytest.mark.SUITE_main  # Marks the test suite to be run as MAIN
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])  # This test works on Windows editor
@pytest.mark.parametrize("project", ["AutomatedTesting"])  # Use AutomatedTesting project
class TestAutomation(EditorTestSuite):
    class Check_MetadataRename_test(EditorSingleTest):
        @classmethod
        def setup(self, instance, request, workspace):
            test_file_name = "bunny_material"
            original_files = original_file_set(test_file_name)
            renamed_files = renamed_file_set(test_file_name)
            self.test_files = original_files + renamed_files

            cleanup_test_files(workspace, self.test_files)

            # Extract the original test files (material + its meta file)
            dst = os.path.join(workspace.paths.engine_root(), "AutomatedTesting")
            with zipfile.ZipFile("bunny_material.zip") as bundle_zip:
                bundle_zip.extractall(dst)

            # Now rename both files.  The point of the test is to verify references are still valid after this rename
            os.rename(os.path.join(dst, original_files[0]), os.path.join(dst, renamed_files[0]))
            os.rename(os.path.join(dst, original_files[1]), os.path.join(dst, renamed_files[1]))

        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            cleanup_test_files(workspace, self.test_files)

        from .tests import MetadataRelocation_ReferenceValidAfterRename as test_module
