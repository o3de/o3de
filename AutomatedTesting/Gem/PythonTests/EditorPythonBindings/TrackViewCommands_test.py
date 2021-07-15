"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API from VertexMode.h
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from .hydra_utils import launch_test_case


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestTrackViewAutomation(object):

    def test_Supported_TrackView(self, request, editor, level, launcher_platform):
        unexpected_lines = []
        expected_lines = [
            '[PASS] GetNumSequences returned the expected number of sequences [2].',
            '[PASS] GetSequenceName returned the expected name [Test Sequence 01]',
            '[PASS] SetSequenceTimeRange modified time range successfully.',
            '[PASS] DeleteSequence ran with no error thrown.',
            '[PASS] SetCurrentSequence ran with no error thrown.',
            '[PASS] PlaySequence ran with no error thrown.',
            '[PASS] AddNode ran with no error thrown.',
            '[PASS] Found the expected number of nodes [2].',
            '[PASS] GetNodeName returned the expected name [Test Node 01].',
            '[PASS] DeleteNode ran with no error thrown.',
            '[PASS] Found the expected number of nodes [1].',
            '[PASS] DeleteSequence ran with no error thrown.'
            ]

        test_case_file = os.path.join(os.path.dirname(__file__), 'TrackViewCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
