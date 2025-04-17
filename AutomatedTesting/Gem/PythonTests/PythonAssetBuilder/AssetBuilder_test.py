"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
#
# This launches the AssetProcessor and Editor then attempts to find the expected
# assets created by a Python Asset Builder and the output of a scene pipeline script
#
import sys
import os
import pytest
import logging
import re
import sqlite3
pytest.importorskip('ly_test_tools')

import ly_test_tools.environment.file_system as file_system
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.waiter as waiter

from ly_test_tools.o3de.editor_test_utils import compile_test_case_name_from_request

def detect_product(sql_connection, platform, target):
    cur = sql_connection.cursor()
    product_target = f'{platform}/{target}'
    print(f'Detecting {product_target} in assetdb.sqlite')
    hits = 0
    for row in cur.execute(f'select ProductID from Products where ProductName is "{product_target}"'):
        hits = hits + 1
    assert hits == 1

def compile_test_case_name(request):
    """
    Compile a test case name for consumption by the TIAF python coverage listener gem.
    @param request: The fixture request.
    """
    try:
        test_case_prefix = "::".join(str.split(request.node.nodeid, "::")[:2])
        test_case_name = "::".join([test_case_prefix, request.node.originalname])
        callspec = request.node.callspec.id
        compiled_test_case_name = f"{test_case_name}[{callspec}]"
    except Exception as e:
        logging.warning(f"Error reading test case name for TIAF. {e}")
        compiled_test_case_name = "ERROR"
    return compiled_test_case_name


def find_products(cache_folder, platform):
    con = sqlite3.connect(os.path.join(cache_folder, 'assetdb.sqlite'))
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/test_asset.mock_asset')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_positive.fbx.azmodel')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_negative.fbx.azmodel')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_positive.fbx.azmodel')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_negative.fbx.azmodel')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_positive.fbx.azmodel')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_negative.fbx.azmodel')
    detect_product(con, platform, 'gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_center.fbx.azmodel')
    con.close()


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['auto_test'])
class TestPythonAssetProcessing(object):
    def test_DetectPythonCreatedAsset(self, request, editor, level, launcher_platform):
        unexpected_lines = []
        expected_lines = []
        timeout = 180
        halt_on_unexpected = False
        test_directory = os.path.join(os.path.dirname(__file__))
        testFile = os.path.join(test_directory, 'AssetBuilder_test_case.py')
        compiled_test_case_name = compile_test_case_name_from_request(request)
        editor.args.extend(['-NullRenderer', '-rhi=Null', "--skipWelcomeScreenDialog", "--autotest_mode", "--runpythontest", testFile, f"-pythontestcase={compiled_test_case_name}"])

        with editor.start():
            editorlog_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')
            log_monitor = ly_test_tools.log.log_monitor.LogMonitor(editor, editorlog_file)
            waiter.wait_for(
                lambda: editor.is_alive(),
                timeout,
                exc=("Log file '{}' was never opened by another process.".format(editorlog_file)),
                interval=1)
            log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected, timeout)

        cache_folder = editor.workspace.paths.cache()
        platform = editor.workspace.asset_processor_platform
        if platform == 'windows':
            platform = 'pc'
        find_products(cache_folder, platform)
