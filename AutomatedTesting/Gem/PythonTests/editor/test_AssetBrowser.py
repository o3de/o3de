"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13660195: Asset Browser - File Tree Navigation
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 180


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAssetBrowser(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C13660195")
    @pytest.mark.SUITE_periodic
    def test_AssetBrowser_TreeNavigation(self, request, editor, level, launcher_platform):
        expected_lines = [
            "Collapse/Expand tests: True",
            "Asset visibility test: True",
            "Scrollbar visibility test: True",
            "AssetBrowser_TreeNavigation:  result=SUCCESS"

        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AssetBrowser_TreeNavigation.py",
            expected_lines,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout
        )

    @pytest.mark.test_case_id("C13660194")
    @pytest.mark.SUITE_periodic
    def test_AssetBrowser_SearchFiltering(self, request, editor, level, launcher_platform):
        expected_lines = [
            "cedar.fbx asset is filtered in Asset Browser",
            "Animation file type(s) is present in the file tree: True",
            "FileTag file type(s) and Animation file type(s) is present in the file tree: True",
            "FileTag file type(s) is present in the file tree after removing Animation filter: True",
        ]

        unexpected_lines = [
            "Asset Browser opened: False",
            "Animation file type(s) is present in the file tree: False",
            "FileTag file type(s) and Animation file type(s) is present in the file tree: False",
            "FileTag file type(s) is present in the file tree after removing Animation filter: False",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AssetBrowser_SearchFiltering.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
