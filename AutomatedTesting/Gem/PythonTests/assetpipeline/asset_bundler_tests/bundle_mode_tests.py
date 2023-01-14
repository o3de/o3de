"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import logging
import sys
import time
pytest.importorskip('ly_test_tools')

import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.log.log_monitor

from ly_test_tools.o3de.editor_test_utils import compile_test_case_name_from_request

from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.bundler_batch_setup_fixture import bundler_batch_setup_fixture as bundler_batch_helper
from ..ap_fixtures.timeout_option_fixture import timeout_option_fixture as timeout


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['TestDependenciesLevel'])

class TestBundleMode(object):
    def test_bundle_mode_with_levels_mounts_bundles_correctly(self, request, editor, level, launcher_platform,
                                                              asset_processor, workspace, bundler_batch_helper):
        asset_processor.add_source_folder_assets("AutomatedTesting\\Levels\\TestDependenciesLevel")
        asset_processor.batch_process()
        level_pak = os.path.join("levels", level, "TestDependenciesLevel.spawnable")
        bundles_folder = os.path.join(workspace.paths.project(), "Bundles")
        bundle_request_path = os.path.join(bundles_folder, "bundle.pak")
        bundle_result_path = os.path.join(bundles_folder,
                                          bundler_batch_helper.platform_file_name(
                                              "bundle.pak", workspace.asset_processor_platform))

        # Create target 'Bundles' folder if it doesn't exist
        if not os.path.exists(bundles_folder):
            os.mkdir(bundles_folder)
        # Delete target bundle file if it already exists
        if os.path.exists(bundle_result_path):
            fs.delete([bundle_result_path], True, False)

        # Make asset list file to use in the bundle
        bundler_batch_helper.call_assetLists(
            addSeed=level_pak,
            assetListFile=bundler_batch_helper["asset_info_file_request"],
        )

        # Make bundle in <project_folder>/Bundles
        bundler_batch_helper.call_bundles(
            assetListFile=bundler_batch_helper["asset_info_file_result"],
            outputBundlePath=bundle_request_path,
            maxSize="2048",
        )

        # Ensure the bundle was created
        assert os.path.exists(bundle_result_path), f"Bundle was not created at location: {bundle_result_path}"

        # The editor flips the slash direction in some of the printouts
        bundle_result_path_editor_separator = bundle_result_path.replace('\\', '/')

        expected_lines = [
            # A beginning of test printout can help debug where failures occur, if this line is missing
            # then the Editor didn't launch, didn't run the Python test, or didn't pass in the right parameter
            f'Bundle mode test running with path {bundles_folder}',
            # These printouts happen in response to the loadbundles call, and verify this bundle is actually loaded
            f"[CONSOLE] Executing console command 'loadbundles {bundles_folder}'",
            f'(BundlingSystem) - Loading bundles from {bundles_folder} of type .pak',
            f'(Archive) - Opening archive file {bundle_result_path_editor_separator}',
        ]
        unexpected_lines = []

        timeout = 180
        halt_on_unexpected = False
        test_directory = os.path.join(os.path.dirname(__file__))
        test_file = os.path.join(test_directory, 'bundle_mode_in_editor_tests.py')
        compiled_test_case_name = compile_test_case_name_from_request(request)
        editor.args.extend(['-NullRenderer', '-rhi=Null', "--skipWelcomeScreenDialog",
                            "--autotest_mode", "--runpythontest", test_file, "--runpythonargs", bundles_folder, f"-pythontestcase={compiled_test_case_name}"])

        with editor.start(launch_ap=True):
            editor_log_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')
            log_monitor = ly_test_tools.log.log_monitor.LogMonitor(editor, editor_log_file)
            waiter.wait_for(
                lambda: editor.is_alive(),
                timeout,
                exc=("Log file '{}' was never opened by another process.".format(editor_log_file)),
                interval=1)
            log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected, timeout)

        # Delete the bundle created and used in this test
        fs.delete([bundle_result_path], True, False)
