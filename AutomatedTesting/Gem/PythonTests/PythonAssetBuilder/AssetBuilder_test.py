"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
#
# This launches the AssetProcessor and Editor then attempts to find the expected
# assets created by a Python Asset Builder and the output of a scene pipeline script
#
import sys
import os
import pytest
import logging
pytest.importorskip('ly_test_tools')

import ly_test_tools.environment.file_system as file_system
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.waiter as waiter

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['auto_test'])
class TestPythonAssetProcessing(object):
    def test_DetectPythonCreatedAsset(self, request, editor, level, launcher_platform):
        unexpected_lines = []
        expected_lines = [
            'Mock asset exists',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_positive_1.azmodel) found',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_negative_1.azmodel) found',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_positive_1.azmodel) found',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_negative_1.azmodel) found',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_positive_1.azmodel) found',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_negative_1.azmodel) found',
            'Expected subId for asset (gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_center_1.azmodel) found'
        ]
        timeout = 180
        halt_on_unexpected = False
        test_directory = os.path.join(os.path.dirname(__file__))
        testFile = os.path.join(test_directory, 'AssetBuilder_test_case.py')
        editor.args.extend(['-NullRenderer', "--skipWelcomeScreenDialog", "--autotest_mode", "--runpythontest", testFile])

        with editor.start():
            editorlog_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')
            log_monitor = ly_test_tools.log.log_monitor.LogMonitor(editor, editorlog_file)
            waiter.wait_for(
                lambda: editor.is_alive(),
                timeout,
                exc=("Log file '{}' was never opened by another process.".format(editorlog_file)),
                interval=1)
            log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected, timeout)
