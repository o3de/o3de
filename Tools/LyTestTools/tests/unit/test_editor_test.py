"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
import unittest.mock as mock
import unittest

import ly_test_tools.o3de.editor_test as editor_test

pytestmark = pytest.mark.SUITE_smoke


class TestEditorTest(unittest.TestCase):

    def test_SplitBatchedEditorLogFile_FoundLine_SavesTwoLogs(self):
        mock_workspace = mock.MagicMock()
        starting_path = "C:/Git/o3de/AutomatedTesting/user/log_test_1/(1)editor_test.log"
        destination_file = "C:/Git/o3de/build/windows/Testing/LyTestTools/arbitrary/(1)editor_test.log"
        log_file_to_split = "(1)editor_test.log"
        mock_data = 'plus some text here \n and more dummy (python) - Running automated test: ' \
                    'C:\\Git\\o3de\\AutomatedTesting\\Gem\\PythonTests\\largeworlds\\dyn_veg\\EditorScripts' \
                    '\\SurfaceDataRefreshes_RemainsStable.py (testcase ) \n plus some other text \n'
        with mock.patch('builtins.open', mock.mock_open(read_data=mock_data)) as mock_file:
            editor_test._split_batched_editor_log_file(mock_workspace, starting_path, destination_file, log_file_to_split)

        assert mock_workspace.artifact_manager.save_artifact.call_count == 2

    def test_SplitBatchedEditorLogFile_FoundLine_SavesOneLog(self):
        mock_workspace = mock.MagicMock()
        starting_path = "C:/Git/o3de/AutomatedTesting/user/log_test_1/(1)editor_test.log"
        destination_file = "C:/Git/o3de/build/windows/Testing/LyTestTools/arbitrary/(1)editor_test.log"
        log_file_to_split = "(1)editor_test.log"
        mock_data = 'plus some text here \n and more dummy (python) - Running automated test: ' \
                    'C:\\Git\\o3de\\AutomatedTesting\\Gem\\PythonTests\\largeworlds\\dyn_veg\\EditorScripts' \
                    '\\SurfaceDataRefreshes_RemainsStable.\n plus some other text \n'
        with mock.patch('builtins.open', mock.mock_open(read_data=mock_data)) as mock_file:
            editor_test._split_batched_editor_log_file(mock_workspace, starting_path, destination_file, log_file_to_split)

        assert mock_workspace.artifact_manager.save_artifact.call_count == 1
