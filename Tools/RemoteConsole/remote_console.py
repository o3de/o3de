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

logger = logging.getLogger(__name__)
logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p', filename='remote_console.log', encoding='utf-8', level=logging.INFO)
remoteConsole = None

def main():
    parser = argparse.ArgumentParser(description="Provides common dev functionality", add_help=False)
    parser.add_argument('--addr', default='127.0.0.1', dest='address', help="Address for Remote Console to connect")
    parser.add_argument('--port', default=4600, dest='port', help="Port for Remote Console to connect")

    args, unknown = parser.parse_known_args(sys.argv[1:])

    remoteConsole = remote_console_commands.RemoteConsole(addr=args.address,port=args.port,on_message_received=printLog)
    remoteConsole.start()

def printLog(raw):
    # type: (str) -> None
    refined = raw.strip()
    if len(refined):
        logger.info(raw)
        print(refined)

def handler(signal_received, frame):
    print("Received Interrupt.  Exiting")
    remoteConsole.stop()
    sys.exit()

if __name__ == "__main__":
    signal.signal(signal.SIGINT, handler)
    main()
