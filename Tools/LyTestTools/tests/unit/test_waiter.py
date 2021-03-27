"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import unittest.mock as mock
import unittest

import time
import pytest

import ly_test_tools.environment.waiter

pytestmark = pytest.mark.SUITE_smoke


@mock.patch('time.sleep', mock.MagicMock)
class TestWaitFor(unittest.TestCase):

    def test_WaitForFunctionCall_GivenExceptionTimeoutExceeded_RaiseException(self):
        input_func = mock.MagicMock()
        input_func.return_value = False

        with self.assertRaises(Exception):
            ly_test_tools.environment.waiter.wait_for(input_func, .001, Exception, 0)

    def test_WaitForFunctionCall_TimeoutExceeded_RaiseAssertionError(self):
        input_func = mock.MagicMock()
        input_func.return_value = False

        with self.assertRaises(Exception):
            ly_test_tools.environment.waiter.wait_for(input_func, .001, interval=0)

    def test_WaitForFunctionCall_TimeoutExceeded_EnoughTime(self):
        input_func = mock.MagicMock()
        input_func.return_value = False

        timeout_end = time.time() + 0.1
        try:
            ly_test_tools.environment.waiter.wait_for(input_func, 0.1, Exception, interval=0.01)
        except Exception:
            pass
        # It should have taken at least 1/10 second
        assert time.time() > timeout_end