"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

RemoteConsole: Used to interact with Lumberyard Launchers through the Remote Console to execute console commands.
"""

import logging
import socket
import threading
import time

client_message_logger = logging.getLogger('remote_console.client_message')
diagnostic_logger = logging.getLogger('remote_console.diagnostic')


# List of all console command types, some are not being used in this
BASE_MSG_TYPE = ord('0')
CONSOLE_MESSAGE_MAP = {
    'NOOP': BASE_MSG_TYPE + 0,
    'REQ': BASE_MSG_TYPE + 1,
    'LOGMESSAGE': BASE_MSG_TYPE + 2,
    'LOGWARNING': BASE_MSG_TYPE + 3,
    'LOGERROR': BASE_MSG_TYPE + 4,
    'COMMAND': BASE_MSG_TYPE + 5,
    'AUTOCOMPLETELIST': BASE_MSG_TYPE + 6,
    'AUTOCOMPLETELISTDONE': BASE_MSG_TYPE + 7,
    'GAMEPLAYEVENT': BASE_MSG_TYPE + 22,
    'CONNECTMESSAGE': BASE_MSG_TYPE + 25,
}
REVERSE_CONSOLE_MESSAGE_MAP = dict()
for key in CONSOLE_MESSAGE_MAP:
    REVERSE_CONSOLE_MESSAGE_MAP[CONSOLE_MESSAGE_MAP[key]] = key


def capture_screenshot_command(remote_console_instance):
    # type: (RemoteConsole) -> None
    """
    This is just a helper function to help capture in-game screenshot
    :param remote_console_instance: RemoteConsole instance
    :return: None
    """
    screenshot_response = 'Screenshot: '
    send_command_and_expect_response(remote_console_instance, 'r_GetScreenShot 2', screenshot_response)
    diagnostic_logger.info('Screenshot has been taken.')


def send_command_and_expect_response(remote_console_instance, command_to_run, expected_log_line, timeout=60):
    # type: (RemoteConsole, str, str, int) -> None
    """
    This is just a helper function to help send a command and validate against expected output.
    :param remote_console_instance: RemoteConsole instance
    :param command_to_run: The command that you wish to run
    :param expected_log_line:  The console log line to expect in order to set the event to true
    :param timeout: int representing the time to wait in seconds
    :return: None, but will assert against the expected_response() boolean return for validation.
    """
    event = threading.Event()
    remote_console_instance.handlers[expected_log_line.encode()] = event

    def expected_response():
        return remote_console_instance.expect_log_line(expected_log_line, timeout)

    remote_console_instance.send_command(command_to_run)

    assert expected_response(), \
        '{} command failed. Was looking for {} in the log but did not find it.'.format(
            command_to_run, expected_log_line)


def _default_on_message_received(raw: str):
    """
    This will just print the raw data from the message received.  We are striping white spaces before and after the
    message and logging it.  We are passing this function to the remote console instance as a default.  On any received
    message a user can overwrite the functionality with any function that they would like.
    :param raw: Raw string returned from the remote console
    :return: None
    """
    refined = raw.strip()
    if len(refined):
        client_message_logger.info(raw)


def _default_disconnect():
    """
    On a disconnect a user can overwrite the functionality with any function, this one will just print to the
    logger a line 'Disconnecting from the Port.'
    :return: None
    """
    diagnostic_logger.info('Disconnecting')


class RemoteConsole:
    def __init__(self, addr='127.0.0.1', port=4600, on_disconnect=_default_disconnect, on_message_received=_default_on_message_received):
        """
        Creates a port connection using port 4600 to issue console commands and poll for specific console log lines.
        :param addr: The ip address where the launcher lives that we want to connect to
        :param port: The port where the remote console will be connecting to.  This usually starts at 4600, and is
        increased by one for each additional launcher that is opened
        :param on_disconnect: User can supply their own disconnect functionality if they would like
        :param on_message_received: on_message_received function in case they want their logging info handled in a
        different way
        """
        self.handlers = {}
        self.connected = False
        self.addr = addr
        self.port = port
        self.on_disconnect = on_disconnect
        self.on_display = on_message_received
        self.pump_thread = threading.Thread(target=self.pump)
        self.stop_pump = threading.Event()
        self.ready = threading.Event()

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def start(self, timeout: int = 10, timeout_port: int = 30, retry_delay: int = 5):
        """
        Starts the socket connection to the Launcher instance.
        :param timeout: The timeout in seconds for the pump thread to get ready before raising an exception.
        :param timeout_port: The timeout in seconds before giving up connecting to a port
        :param retry_delay: The delay in seconds before a retry is attempted.
        """
        if self.connected:
            diagnostic_logger.warning('RemoteConsole is already connected.')
            return

        # Connect to the remote console via a port
        self._connect_to_port(timeout_port, retry_delay)

        # Clear the timeout. Further socket operations won't timeout.
        self.socket.settimeout(None)

        # Check if the remote console is ready
        self.pump_thread.start()
        if not self.ready.wait(timeout):
            raise Exception(f'remote_console_commands.py:start: Remote console connection never became ready. Waited for {timeout} seconds.')

        self.connected = True
        diagnostic_logger.info(f'Remote Console Started at port {self.port}')

    def _connect_to_port(self, timeout_port: int, retry_delay: int):
        """
        Helper method to connect to the RC on a given port.
        :param timeout_port: The timeout in seconds before giving up connecting to a port
        :param retry_delay: The delay in seconds before a retry is attempted.
        """
        time_waited = 0
        while True:
            if time_waited >= timeout_port:
                raise Exception(f'remote_console_commands.py:start: Remote console connection never became ready. Waited for {time_waited} seconds.')
            else:
                if self._scan_ports():
                    # connected successfully to RC on a port
                    break
                else:
                    # failed to find RC on a port, wait
                    time_waited += retry_delay
                    time.sleep(retry_delay)
                    diagnostic_logger.debug(f'remote_console_commands.py:start: Have waited for at least {time_waited} seconds.')

    def _scan_ports(self):
        # Do not wait more than 3.0 seconds per connection attempt.
        self.socket.settimeout(3.0)
        num_ports_to_scan = 8
        max_port = self.port + num_ports_to_scan
        while self.port < max_port:
            try:
                diagnostic_logger.debug(f'Trying port {self.addr}:{self.port}')
                self.socket.connect((self.addr, self.port))
                diagnostic_logger.info(f'Successfully connected to port: {self.port}')
                return True
            except Exception as e:
                diagnostic_logger.info(f'Connection exception {e}')
                self.port += 1
        if self.port >= max_port:
            from_port_to_port = f'from port {self.port - num_ports_to_scan} to port {self.port - 1}'
            diagnostic_logger.debug(f'Remote console connection never became ready after scanning {from_port_to_port}. Trying again')
        return False

    def stop(self):
        """
        Stops and closes the socket connection to the Launcher instance.
        """
        if not self.connected:
            diagnostic_logger.warning('RemoteConsole is not connected, cannot stop.')
            return

        self.stop_pump.set()
        self.socket.shutdown(socket.SHUT_WR)
        self.socket.close()
        self.pump_thread.join()
        self.connected = False

    def send_command(self, command: str):
        """
        Transforms and sends commands to the Launcher instance.
        :param command: The command to be sent to the Launcher instance
        """
        message = self._create_message(CONSOLE_MESSAGE_MAP['COMMAND'], command)
        try:
            self._send_message(message)
        except Exception:
            self.connected = False
            self.on_disconnect()

    def pump(self):
        """
        Pump function that is used by the pump_thread. Listens to receive messages from the socket
        and disconnects during an exception.
        """
        while not self.stop_pump.is_set():
            try:
                # Sending a NOOP message in order to get log lines
                self._send_message(self._create_message(CONSOLE_MESSAGE_MAP['NOOP']))
                
                self._handle_message(self.socket.recv(4096))
            except Exception as e:
                diagnostic_logger.debug(f'disconnect because of an exception {e}')
                self.connected = False
                self.on_disconnect()
                self.stop_pump.set()

    def expect_log_line(self, match_string: str, timeout: int = 30):
        """
        Looks for a log line event to expect within a time frame. Returns False is timeout is reached.
        :param match_string: The string to match that acts as a key
        :param timeout: The timeout to wait for the log line in seconds
        :return: boolean True if match_string found, False otherwise.
        """
        diagnostic_logger.info(f'Waiting for event "{match_string}" for {timeout} seconds')

        event = threading.Event()
        self.handlers[match_string.encode()] = event
        event_success = event.wait(timeout)

        diagnostic_logger.warning(f'Returning "{event_success}" for expect_log_line() - previously this returned a function object, so if you see failures now this may be why.')
        return event_success

    def _create_message(self, message_type: int, message_body: str = '') -> bytearray:
        """
        Transforms a message to be sent to the launcher. The string is converted to a bytearray and
        is prepended with the message type and appended with an ending 0.
        :param message_type: Uses CONSOLE_MESSAGE_MAP to prepend the bytearray message
        :param message_body: The message string to be converted
        """
        message = bytearray(0)

        message.append(message_type)

        message_body = message_body.encode()
        for message_body_char in message_body:
            message.append(message_body_char)

        message.append(0)
        return message

    def _send_message(self, message: bytearray):
        """
        Sends console commands through the socket connection to the launcher. The message string should
        first be transformed into a bytearray.
        :param message: The message to be sent to the Launcher instance
        """
        self.socket.sendall(message)

    def _handle_message(self, message: bytearray):
        """
        Handles the messages and and will poll for expected console messages that we are looking for and set() events to True.
        Displays the message if we determine it is a logging message.
        :param message: The message (a byte array) received to be handled in various ways
        """
        if len(message) < 1:
            diagnostic_logger.error('Received an empty message')
            return

        # message[0] is the representation of the message type inside of the message received from our launchers
        message_type = message[0]

        # ignoring the first byte and the last byte.  The first byte is the message type and the last byte is a
        # Null terminator
        message_body = message[1:-1]

        '''
        diagnostic code
        if message_type != 49 and message_type != 50:
            if message_type in REVERSE_CONSOLE_MESSAGE_MAP:
                diagnostic_logger.debug(f'handle message {message_type}/{REVERSE_CONSOLE_MESSAGE_MAP[message_type]} : {message[1:]}')
            else:
                diagnostic_logger.debug(f'handle message {message_type}/unknown : {message[1:]}')
        '''

        # display the message if it's a logging message type
        if CONSOLE_MESSAGE_MAP['LOGMESSAGE'] <= message_type <= CONSOLE_MESSAGE_MAP['LOGERROR']:
            self.on_display(message_body.decode("utf-8"))
            for key in self.handlers.keys():
                # message received, set flag handler as True for success
                if key in message_body:
                    client_message_logger.info(f'matched key=<{key}>')
                    self.handlers[key].set()
                    continue

        elif message_type == CONSOLE_MESSAGE_MAP['CONNECTMESSAGE']:
            self.ready.set()

        # cleanup expect_log_line handers if the matching string was found or timeout happened.
        handlers_dict = self.handlers.copy()
        for key in handlers_dict.keys():
            if self.handlers[key].is_set():
                del self.handlers[key]
