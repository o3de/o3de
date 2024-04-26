"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import argparse
import logging
import ly_remote_console.ly_remote_console.remote_console_commands as remote_console_commands
import os
import signal
import sys
import threading
import time

client_message_logger = logging.getLogger('remote_console.client_message')
client_message_logger.setLevel(logging.INFO)
message_handler = logging.FileHandler('remote_console.log', mode='w+', encoding='utf-8')  # use mode='a' to append
message_formatter = logging.Formatter(fmt='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
message_handler.setFormatter(message_formatter)
client_message_logger.addHandler(message_handler)

diagnostic_logger = logging.getLogger('remote_console.diagnostic')
diagnostic_logger.setLevel(logging.DEBUG)
diagnostic_handler = logging.StreamHandler(stream=sys.stdout)
diagnostic_logger.addHandler(diagnostic_handler)

remote_console = None
messages_to_console = False


def inputCommand():
    global remote_console
    global diagnostic_logger

    while remote_console.connected:
        command = input()
        if remote_console.connected:
            remote_console.send_command(command)
            diagnostic_logger.info(f'sent command "{command}"')


def main():
    global remote_console
    global client_message_logger
    global messages_to_console

    parser = argparse.ArgumentParser(description="Provides common dev functionality", add_help=False)
    parser.add_argument('--addr', default='127.0.0.1', dest='address', help="Address for Remote Console to connect")
    parser.add_argument('--port', default='4600', dest='port', help="Port for Remote Console to connect")
    parser.add_argument('--console', action='store_true', dest='console', help="Log all the client messages to console in addition to remote_console.log")

    args, unknown = parser.parse_known_args(sys.argv[1:])

    message_handler = None
    if args.console:
        messages_to_console = True
        message_handler = logging.StreamHandler(stream=sys.stdout)
        message_formatter = logging.Formatter(fmt='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
        message_handler.setFormatter(message_formatter)
        client_message_logger.addHandler(message_handler)

    remote_console = remote_console_commands.RemoteConsole(addr=args.address, port=int(args.port), on_message_received=printLog)
    remote_console.start()

    command_thread = threading.Thread(target=inputCommand)
    command_thread.start()

    while remote_console.connected:
        time.sleep(0.5)

    if not remote_console.connected:
        os.kill(os.getpid(), signal.SIGINT)


def printLog(raw: str):
    global messages_to_console

    refined = raw.strip()
    if len(refined):
        if len(refined) > 4 and refined[0] == '$' and refined[1] == '3' and ('$5' in refined) and not messages_to_console:  # command starts with $3 and contains $5
            diagnostic_logger.info(refined.replace('$3', '').replace('$4', '').replace('$5', '').replace('$6', ''))  # drop color markers
        if raw[-1] == '\n':
            client_message_logger.info(raw[:-1])
        else:
            client_message_logger.info(raw)


def handler(signal_received, frame):
    global remote_console

    diagnostic_logger.info("Received interrupt, exiting.")
    remote_console.stop()
    sys.exit()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handler)
    main()
