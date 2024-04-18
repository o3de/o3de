"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import signal
import argparse
import logging
import ly_remote_console.ly_remote_console.remote_console_commands as remote_console_commands

client_message_logger = logging.getLogger('remote_console.client_message')
client_message_logger.setLevel(logging.INFO)
message_handler = logging.FileHandler('remote_console.log', mode='w+', encoding='utf-8')  # use mode='a' to append
message_formatter = logging.Formatter(fmt='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
message_handler.setFormatter(message_formatter)
client_message_logger.addHandler(message_handler)

diagnostic_logger = logging.getLogger('remote_console.diagnostic')
diagnostic_handler = logging.StreamHandler(stream=sys.stdout)
diagnostic_logger.setLevel(logging.DEBUG)
diagnostic_logger.addHandler(diagnostic_handler)

remoteConsole = None


def main():
    global remoteConsole

    parser = argparse.ArgumentParser(description="Provides common dev functionality", add_help=False)
    parser.add_argument('--addr', default='127.0.0.1', dest='address', help="Address for Remote Console to connect")
    parser.add_argument('--port', default='4600', dest='port', help="Port for Remote Console to connect")

    args, unknown = parser.parse_known_args(sys.argv[1:])

    remoteConsole = remote_console_commands.RemoteConsole(addr=args.address, port=int(args.port), on_message_received=printLog)
    remoteConsole.start()


def printLog(raw: str):
    refined = raw.strip()
    if len(refined):
        if raw[-1] == '\n':
            client_message_logger.info(raw[:-1])
        else:
            client_message_logger.info(raw)


def handler(signal_received, frame):
    global remoteConsole

    diagnostic_logger.info("Received interrupt, exiting.")
    remoteConsole.stop()
    sys.exit()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handler)
    main()
