"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from unittest import TestCase
from unittest.mock import MagicMock

from multithread.worker import FunctionWorker


class TestWorker(TestCase):
    """
    FunctionWorker unit test cases
    """
    def test_run_no_result_callbacks_get_invoked_as_expected(self) -> None:
        expected_function_holder: MagicMock = MagicMock()
        test_function_worker: FunctionWorker = FunctionWorker(expected_function_holder.no_result_callback)
        test_function_worker.signals.finished.connect(expected_function_holder.finished_callback)

        test_function_worker.run()
        expected_function_holder.no_result_callback.assert_called_once()
        expected_function_holder.finished_callback.assert_called_once()

    def test_run_have_result_callbacks_get_invoked_as_expected(self) -> None:
        expected_function_holder: MagicMock = MagicMock()
        expected_result: MagicMock = MagicMock()
        expected_function_holder.have_result_callback.return_value = expected_result
        test_function_worker: FunctionWorker = FunctionWorker(expected_function_holder.have_result_callback)
        test_function_worker.signals.result.connect(expected_function_holder.result_callback)
        test_function_worker.signals.finished.connect(expected_function_holder.finished_callback)

        test_function_worker.run()
        expected_function_holder.have_result_callback.assert_called_once()
        expected_function_holder.result_callback.assert_called_once_with(expected_result)
        expected_function_holder.finished_callback.assert_called_once()

    def test_run_exception_callbacks_get_invoked_as_expected(self) -> None:
        expected_function_holder: MagicMock = MagicMock()
        expected_function_holder.exception_callback.side_effect = Exception
        test_function_worker: FunctionWorker = FunctionWorker(expected_function_holder.exception_callback)
        test_function_worker.signals.result.connect(expected_function_holder.result_callback)
        test_function_worker.signals.error.connect(expected_function_holder.error_callback)
        test_function_worker.signals.finished.connect(expected_function_holder.finished_callback)

        test_function_worker.run()
        expected_function_holder.exception_callback.assert_called_once()
        expected_function_holder.result_callback.assert_not_called()
        expected_function_holder.error_callback.assert_called_once()
        expected_function_holder.finished_callback.assert_called_once()
