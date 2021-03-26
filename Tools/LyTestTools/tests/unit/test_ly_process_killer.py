"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit tests for ly_test_tools._internal.managers.ly_process_killer
"""

import unittest.mock as mock

import pytest

import ly_test_tools._internal.managers.ly_process_killer

pytestmark = pytest.mark.SUITE_smoke


class TestProcessKiller(object):

    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    def test_DetectLumberyardProcesses_ValidProcessesList_ReturnsDetectedProcessesList(self, mock_process_exists):
        mock_process_exists.side_effect = [True, False]
        mock_process_list = ['foo', 'bar']

        under_test = ly_test_tools._internal.managers.ly_process_killer.detect_lumberyard_processes(
            processes_list=mock_process_list)

        assert under_test == ['foo']

    def test_KillProcesses_ProcessesListIsNotList_RaisesLyProcessKillerException(self):

        with pytest.raises(ly_test_tools._internal.managers.ly_process_killer.LyProcessKillerException):
            ly_test_tools._internal.managers.ly_process_killer.kill_processes(processes_list={})
