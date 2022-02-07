"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import unittest.mock as mock

from pytest_mock import MockFixture

import ly_test_tools._internal.log.py_logging_util as py_logging_util

pytestmark = pytest.mark.SUITE_smoke


class TestTerminateLogging(object):

    def test_TerminateLogging_HandlersUninitialized_NoHandlersRemoved(self, mocker):
        # type: (MockFixture) -> None
        mock_getLogger = mocker.patch("logging.getLogger")  # type: MagicMock
        mock_removeHandler = mock_getLogger.return_value.removeHandler  # type: MagicMock

        py_logging_util.terminate_logging()

        mock_removeHandler.assert_not_called()

    def test_TerminateLogging_HandlersInitialized_HandlersRemoved(self, mocker):
        # type: (MockFixture) -> None
        mock_getLogger = mocker.patch("logging.getLogger")  # type: MagicMock
        mock_removeHandler = mock_getLogger.return_value.removeHandler  # type: MagicMock
        mock_stream_handler = "Mock Stream Handler"
        mock_info_file_handler = "Mock Info File Handler"
        mock_debug_file_handler = "Mock Debug File Handler"

        py_logging_util._stream_handler = mock_stream_handler
        py_logging_util._info_file_handler = mock_info_file_handler
        py_logging_util._debug_file_handler = mock_debug_file_handler

        py_logging_util.terminate_logging()

        calls = [
            mock.call(mock_stream_handler),
            mock.call(mock_info_file_handler),
            mock.call(mock_debug_file_handler),
        ]
        mock_removeHandler.assert_has_calls(calls)


class TestInitializeLogging(object):

    @mock.patch("logging.getLogger", scope='module')
    def test_InitializeLogging_AddHandlerCalled_CalledThrice(self, mock_get_logger):
        dummy_log_path = "dummy_log_path"
        dummy_info_path = "dummy_info_path"
        py_logging_util._stream_handler = None
        py_logging_util._info_file_handler = None
        py_logging_util._debug_file_handler = None
        mock_add_handler = mock_get_logger.return_value.addHandler
        py_logging_util.initialize_logging(dummy_info_path, dummy_log_path)
        assert mock_add_handler.call_count == 3
        py_logging_util.terminate_logging()


    @mock.patch("logging.getLogger", scope='module')
    def test_InitializeLogging_CheckLoggerCalled_LoggerCalledOnce(self, mock_get_logger):
        dummy_log_path = "dummy_path"
        dummy_info_path = "dummy_path"
        py_logging_util.initialize_logging(dummy_info_path, dummy_log_path)
        mock_get_logger.assert_called_once()

    @mock.patch("logging.getLogger", scope='module')
    def test_InitializeLogging_SetLogLevelValidArgs_ValidArgsPassed(self, mock_get_logger):
        dummy_log_path = "dummy_path"
        dummy_info_path = "dummy_path"
        mock_setLevel = mock_get_logger.return_value.setLevel
        py_logging_util.initialize_logging(dummy_info_path, dummy_log_path)
        mock_setLevel.assert_called_with(10)  # logging.DEBUG = 10

    def test_InitializeLogging_CheckHandlerInitialized_HandlerNotNone(self):
        dummy_log_path = "dummy_path"
        dummy_info_path = "dummy_path"
        py_logging_util.initialize_logging(dummy_info_path,dummy_log_path)
        assert py_logging_util._debug_file_handler is not None
        assert py_logging_util._info_file_handler is not None
        assert py_logging_util._stream_handler is not None

    @mock.patch("logging.StreamHandler.setFormatter", scope='module')
    def test_InitializeLogging_CheckFormatting_HandlerFormattingIsCorrect(self,mock_stream_handler_formatter):
        dummy_log_path = "dummy_path"
        dummy_info_path = "dummy_path"
        py_logging_util._stream_handler = None
        py_logging_util._info_file_handler = None
        py_logging_util._debug_file_handler = None
        py_logging_util.initialize_logging(dummy_info_path,dummy_log_path)
        #example of formatted string : 7024.00016785 - DEBUG - [MainThread] - ly_test_tools.launchers.platforms.win.launcher - Initialized Windows Launcher
        format_string = "%(relativeCreated)s - %(levelname)s - [%(threadName)s] - %(name)s - %(message)s"
        assert mock_stream_handler_formatter.call_args[0][0]._fmt == format_string





