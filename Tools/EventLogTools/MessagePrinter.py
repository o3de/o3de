#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from EventLogger.Reader import Reader
from EventLogger.Utils import EventNameHash, PrologId

import argparse
import os
import sys

AssertId = EventNameHash("Assert")
ErrorId = EventNameHash("Error")
MessageId = EventNameHash("Message")
PrintfId = EventNameHash("Printf")
WarningId = EventNameHash("Warning")


def main(args):
    parser = argparse.ArgumentParser(description='Simple Event Logger Printer')

    parser.add_argument('file', type=str, help='Path log file')

    parsed_args = parser.parse_args(args)

    log_file = parsed_args.file
    if not os.path.exists(log_file):
        print('[ERROR] Invalid file path supplied')
        exit(1)

    log_reader = Reader()
    status = log_reader.read_log_file(log_file)
    if status == Reader.ReadStatus_InsufficientFileSize:
        print('File size too small to contain Event Logger information')
        return
    elif status == Reader.ReadStatus_InvalidFormat:
        print('Invalid Event Logger format detected')
        return

    log_header = log_reader.get_log_header()
    print(f'Log File: {log_file}')
    print(f'Format: {log_header.get_format()}')
    print(f'Version: {log_header.get_version()}')

    has_event = (status == Reader.ReadStatus_Success)
    while has_event:
        event_id = log_reader.get_event_name()

        if event_id == PrologId:
            print(f'Thread: {log_reader.get_thread_id()}')

        elif event_id in (AssertId, ErrorId, WarningId, PrintfId, MessageId):
            print(f'> {log_reader.get_event_string()}')

        else:
            print(f'Event ID {event_id}, Size {log_reader.get_event_size()}')

        has_event = log_reader.next()

if __name__ == '__main__':
    main(sys.argv[1:])
