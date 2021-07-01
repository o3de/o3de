"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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