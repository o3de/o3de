"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# TODO: Delete bundle_mode_tests.py, rename this to bundle_mode_tests.py. This is in a "_2" file so both tests could be run while rebuilding the test.

# TODO: Clean up imports
import logging
import os
import pytest
import shutil
import subprocess
import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest
import tempfile
from ..ap_fixtures.bundler_batch_setup_fixture import bundler_batch_setup_fixture as bundler_batch_helper
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

logger = logging.getLogger(__name__)


def bundle_platform_file_name(file_name: str, platform: str) -> str:
    """Converts the standard [file_name] to a platform specific file name"""
    split = file_name.split(".", 1)
    platform_name = ASSET_PROCESSOR_PLATFORM_MAP.get(platform)
    if not platform_name:
        logger.warning(f"platform {platform} not recognized. File name could not be generated")
        return file_name
    return f'{split[0]}_{platform_name}.{split[1]}'


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class BundleModeTests(EditorTestSuite):

    global_extra_cmdline_args = []

    class bundle_mode_test(EditorSingleTest):
        extra_cmdline_args = []

        @classmethod
        def setup(self, instance, request, workspace):
            # TODO : Does this work make sense in setup, or should it be moved to before the yield in run_test?
            level_spawnable = os.path.join("levels", "TestDependenciesLevel", "TestDependenciesLevel.spawnable")
            # TODO : Can we put the bundle into a temp folder that will auto-cleanup, instead of a project folder?
            bundles_folder = os.path.join(workspace.paths.project(), "Bundles")
            bundle_request_path = os.path.join(bundles_folder, "bundle.pak")
            bundle_result_path = os.path.join(bundles_folder,
                                              bundle_platform_file_name("bundle.pak", workspace.asset_processor_platform))

            if not os.path.exists(bundles_folder):
                os.mkdir(bundles_folder)
            if os.path.exists(bundle_result_path):
                file_system.delete([bundle_result_path], True, False)

            bin_dir = workspace.paths.build_directory()
            bundler_batch = os.path.join(bin_dir, "AssetBundlerBatch")
            
            tempDir = os.path.join(tempfile.gettempdir(), "AssetBundlerTempDirectory")
            if os.path.exists(tempDir):
                file_system.delete([tempDir], True, True)
            if not os.path.exists(tempDir):
                os.mkdir(tempDir)
            asset_info_file_name = "assetFileInfo.assetlist"
            asset_info_file_request = os.path.join(tempDir, asset_info_file_name)

            # TODO: Do we actually want the default seeds in this list?
            file_list_command = [bundler_batch, "assetLists", f"--addSeed={level_spawnable}", "--addDefaultSeedListFiles", f"--assetListFile={asset_info_file_request}"]
            logger.info(f"Generating asset list using command: {file_list_command}")
            try:
                output = subprocess.check_output(file_list_command).decode()
            except subprocess.CalledProcessError as e:
                output = e.output.decode('utf-8')
                logger.error(f"AssetBundlerBatch called with args {file_list_command} returned error {e} with output {output}")
            except FileNotFoundError as e:
                logger.error(f"File Not Found - Failed to call AssetBundlerBatch with args {file_list_command} with error {e}")
                raise e

            asset_list_output_name = bundle_platform_file_name(asset_info_file_request, workspace.asset_processor_platform)

            if os.path.isfile(asset_list_output_name) == False:
                # Print out the output if there was an error, to help with debugging.
                logger.error(f"Asset list file was not generated as expected. Output from command {file_list_command} is:\n{output}")

            # TODO: Probably don't need maxsize
            bundle_command = [bundler_batch, "bundles", f"--assetListFile={asset_list_output_name}", f"--outputBundlePath={bundle_request_path}", f"--maxSize=2048"]
            logger.info(f"Generating asset bundle using command: {file_list_command}")
            try:
                output = subprocess.check_output(bundle_command).decode()
            except subprocess.CalledProcessError as e:
                output = e.output.decode('utf-8')
                logger.error(f"AssetBundlerBatch called with args {bundle_command} returned error {e} with output {output}")
            except FileNotFoundError as e:
                logger.error(f"File Not Found - Failed to call AssetBundlerBatch with args {bundle_command} with error {e}")
                raise e

            if os.path.isfile(bundle_result_path) == False:
                logger.error(f"Asset bundle was not generated as expected. Output from command {file_list_command} is:\n{output}")

            # TODO: Should this pass in the expected/unexpected assets, so the in-editor test doesn't have to figure them out?
            # It would mean one place to maintain changes, here.
            self.extra_cmdline_args = ["--runpythonargs", bundles_folder]

        # TODO: Teardown should delete temp files
        @classmethod
        def teardown(self, instance, request, workspace, editor_test_results):
            logger.info("Teardown")
            
        # TODO: Probably don't need a wrap_run
        @classmethod
        def wrap_run(cls, instance, request, workspace, editor_test_results):
            logger.info("Before")
            yield
            logger.info("After")

        from .tests import bundle_mode_tests_in_editor as test_module

            
