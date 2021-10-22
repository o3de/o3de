"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

test_ly_remote_console contains all the unit test for remote console tool.
"""
try:  # Py2
    import mock
except ImportError:  # Py3
    import unittest.mock as mock
import pytest

import ly_remote_console.remote_console_commands as remote_console


@pytest.mark.unit
class TestScreenShot():

    @mock.patch('ly_remote_console.remote_console_commands.RemoteConsole')
    @mock.patch('ly_remote_console.remote_console_commands.capture_screenshot_command')
    def test_CaptureScreenshot_MockSendRequest_SendRequestCalled(self, mock_send_screenshot, mock_remote_console):
        remote_console.capture_screenshot_command(mock_remote_console)
        assert len(mock_send_screenshot.mock_calls) == 1


    def test_SendScreenshotCommand_MockConsole_LineReadSuccess(self):
        mock_remote_console = mock.MagicMock()
        mock_remote_console.expect_log_line.return_value = True

        try:
            remote_console.send_command_and_expect_response(mock_remote_console, 'foo_command', 'foo_line')
        except AssertionError:
            assert False

    def test_SendScreenshotCommand_MockConsole_LineReadFailure(self):
        mock_remote_console = mock.MagicMock()
        mock_remote_console.expect_log_line.return_value = False

        with pytest.raises(AssertionError):
            remote_console.send_command_and_expect_response(mock_remote_console, 'foo_command', 'foo_line')

@pytest.mark.unit
class TestRemoteConsole():

    @mock.patch('socket.socket', mock.MagicMock())
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_Start_CheckDefaultConnection_ConnectionTrue(self):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.start()
        assert rc_instance.connected

    @mock.patch('socket.socket')
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_Start_SocketConnection_SocketCalledOnce(self, mock_socket):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.start()
        mock_socket.assert_called_once()

    @mock.patch('socket.socket')
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_Stop_SocketCommand_SocketCalledForShutdownAndClose(self, mock_socket):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.stop()
        mock_socket.assert_called_once()

    @mock.patch('socket.socket', mock.MagicMock)
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_ExpectLogLine_CheckIfEventLogged_DoesNotReturnFalse(self):
        rc_instance = remote_console.RemoteConsole()
        result = rc_instance.expect_log_line("test")
        assert result

    @mock.patch('socket.socket', mock.MagicMock())
    @mock.patch('ly_remote_console.remote_console_commands.RemoteConsole._create_message')
    def test_SendCommand_CheckMessageCreated_AssertCreateMessageCalled(self, mock_create_message):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.send_command("testCommand")
        mock_create_message.assert_called_once()

    @mock.patch('socket.socket', mock.MagicMock())
    def test_Pump_CheckSendMessage_AssertSendMessageCalled(self):
        rc_instance = remote_console.RemoteConsole()
        rc_instance._send_message = mock.MagicMock()
        rc_instance._handle_message = mock.MagicMock()
        rc_instance._handle_message.side_effect = Exception()  # to force except path in pump()

        rc_instance.pump()
        rc_instance._send_message.assert_called_once()

    @mock.patch('socket.socket', mock.MagicMock())
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_CreateMessage_SimpleNoopMessage_MessageCreated(self):
        rc_instance = remote_console.RemoteConsole()
        expected = bytearray(b'0foo\x00')

        actual = rc_instance._create_message(remote_console.CONSOLE_MESSAGE_MAP['NOOP'], 'foo')

        assert expected == actual

    @mock.patch('socket.socket')
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_SendMessage_SimpleMessage_SocketCalled(self, mock_socket):
        rc_instance = remote_console.RemoteConsole()
        msg = bytearray(0)

        rc_instance._send_message(msg)

        assert mock_socket.sendall.called_once_with(msg)

    @mock.patch('socket.socket', mock.MagicMock())
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_HandleMessage_LogMessage_HandlerSet(self):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.on_display = mock.MagicMock()
        rc_instance.ready = mock.MagicMock()
        mock_evt_handler = mock.MagicMock()
        msg = b'2foo warning0'  # in python3 socket.recv returns byte array. 2 is LOGMESSAGED

        rc_instance.handlers[b'foo warning'] = mock_evt_handler
        rc_instance._handle_message(msg)

        rc_instance.on_display.assert_called_once_with(b'foo warning')
        mock_evt_handler.set.assert_called_once()
        assert 'foo warning' not in rc_instance.handlers.keys()
        rc_instance.ready.set.assert_not_called()

    @mock.patch('socket.socket', mock.MagicMock())
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_HandleMessage_AutoCompleteList_NoOp(self):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.on_display = mock.MagicMock()
        rc_instance.ready = mock.MagicMock()
        msg = b'60'  # in python3 socket.recv returns byte array. 6 is AUTOCOMPLETELIST

        rc_instance._handle_message(msg)

        rc_instance.on_display.assert_not_called()
        rc_instance.ready.set.assert_not_called()

    @mock.patch('socket.socket', mock.MagicMock())
    @mock.patch('ly_remote_console.remote_console_commands.threading', mock.MagicMock())
    def test_HandleMessage_ConnectMessage_ReadySet(self):
        rc_instance = remote_console.RemoteConsole()
        rc_instance.on_display = mock.MagicMock()
        rc_instance.ready = mock.MagicMock()
        msg = b'I0'  # in python3 socket.recv returns byte array. I is CONNECTMESSAGE

        rc_instance._handle_message(msg)

        rc_instance.on_display.assert_not_called()
        rc_instance.ready.set.assert_called_once()

