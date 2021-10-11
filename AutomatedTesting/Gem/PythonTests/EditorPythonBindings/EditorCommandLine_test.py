"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API from PythonEditorFuncs
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from hydra_utils import launch_test_case, launch_test_case_with_args


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestEditorAutomation(object):

    def test_EditorNoArgs(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "editor command line works",
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'EditorCommandLine_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

    def test_EditorWithArgs(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "editor command line works",
            "editor command line arg foo",
            "editor command line arg bar",
            "editor command line arg baz",
            "editor engroot set",
            "path resolved worked"
            ]
        
        extra_args = ['foo bar baz']        
        test_case_file = os.path.join(os.path.dirname(__file__), 'EditorCommandLine_test_case.py')
        launch_test_case_with_args(editor, test_case_file, expected_lines, unexpected_lines, extra_args)
