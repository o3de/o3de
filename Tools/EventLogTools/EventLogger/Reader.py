#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from EventLogger.Utils import EventBoundary, EventHeader, EventNameHash, LogHeader, Prolog, PrologId


def size_align_up(size, align):
    return (size + (align - 1)) & ~(align - 1)


class Reader(object):

    ReadStatus_Success = 0
    ReadStatus_InsufficientFileSize = -1
    ReadStatus_InvalidFormat = -2
    ReadStatus_NoEvents = -3

    def __init__(self):
        self.log_header = LogHeader()

        self.current_event = EventHeader()
        self.current_thread_id = None

        self.buffer = None
        self.buffer_size = 0
        self.buffer_pos = 0

    def read_log_file(self, file_path):
        with open(file_path, mode='rb') as log_file:
            self.buffer = log_file.read()
            self.buffer_size = len(self.buffer)

        if self.buffer_size < LogHeader.size():
            return Reader.ReadStatus_InsufficientFileSize

        self.log_header.unpack(self.buffer)
        self.buffer_pos = LogHeader.size()

        if self.log_header.get_format() not in LogHeader.accepted_formats():
            return Reader.ReadStatus_InvalidFormat

        if self.buffer_pos + EventHeader.size() > self.buffer_size:
            return Reader.ReadStatus_NoEvents

        self.current_event.unpack(self._get_next(EventHeader.size()))
        self._update_thread_id()

        return Reader.ReadStatus_Success

    def get_log_header(self):
        return self.log_header

    def get_thread_id(self):
        return self.current_thread_id

    def get_event_name(self):
        return EventNameHash(self.current_event.event_id)

    def get_event_size(self):
        return self.current_event.size

    def get_event_flags(self):
        return self.current_event.flags

    def get_event_data(self):
        start = self.buffer_pos + EventHeader.size()
        return self._get_next(self.get_event_size(), override_start=start)

    def get_event_string(self):
        string_data = self.get_event_data()
        return string_data.decode('utf-8')

    def next(self):
        real_size = EventHeader.size() + self.get_event_size()
        self.buffer_pos += size_align_up(real_size, EventBoundary)
        if self.buffer_pos < self.buffer_size:
            self.current_event.unpack(self._get_next(EventHeader.size()))
            self._update_thread_id()
            return True
        return False

    def _get_next(self, size, override_start=None):
        start = override_start or self.buffer_pos
        end = start + size
        return self.buffer[start:end]

    def _update_thread_id(self):
        if self.get_event_name() == PrologId:
            prolog = Prolog()
            prolog.unpack(self._get_next(Prolog.size()))
            self.current_thread_id = prolog.thread_id

