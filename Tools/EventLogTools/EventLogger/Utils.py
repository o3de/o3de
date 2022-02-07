#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import struct
import sys

# Simple hash structure based on DJB2a
class EventNameHash(object):

    def __init__(self, name):
        if isinstance(name, int):
            self.hash = name
        else:
            value = 5381
            for i in range(len(name)):
                value = ((value << 5) + value) ^ ord(name[i])
            self.hash = value & 0xFFFFFFFF

    def __hash__(self):
        return self.hash

    def __eq__(self, rhs):
        if isinstance(rhs, EventNameHash):
            return self.hash == rhs.hash
        else:
            return self.hash == rhs


PrologId = EventNameHash("Prolog")
EventBoundary = 8


class LogStructBase(object):

    def _unpack(self, member_info, data):
        pos = 0
        for (member, data_format, data_size) in member_info:
            pos_end = pos + data_size
            unpacked_value = struct.unpack(data_format, data[pos:pos_end])
            setattr(self, member, unpacked_value[0])
            pos = pos_end


class LogHeader(LogStructBase):

    @staticmethod
    def size():
        return 16

    @staticmethod
    def accepted_formats():
        return ['AZEL']

    def __init__(self):
        self.four_cc = None
        self.major_version = None
        self.minor_version = None
        self.user_version = None

    def unpack(self, data):
        member_info = [
            ('four_cc', '@4s', 4),
            ('major_version', '@I', 4),
            ('minor_version', '@I', 4),
            ('user_version', '@I', 4),
        ]
        self._unpack(member_info, data)

    def get_format(self):
        return self.four_cc.decode('utf-8')

    def get_version(self):
        return f'{self.major_version}.{self.minor_version} ({self.user_version})'


class EventHeader(LogStructBase):

    @staticmethod
    def size():
        return 8

    def __init__(self):
        self.event_id = None
        self.size = None
        self.flags = None

    def unpack(self, data):
        member_info = [
            ('event_id', '@I', 4),
            ('size', '@H', 2),
            ('flags', '@H', 2),
        ]
        self._unpack(member_info, data)


class Prolog(EventHeader):

    @staticmethod
    def size():
        return 16

    def __init__(self):
        super().__init__()
        self.thread_id = None

    def unpack(self, data):
        member_info = [
            ('event_id', '@I', 4),
            ('size', '@H', 2),
            ('flags', '@H', 2),
            ('thread_id', '@Q', 8)
        ]
        self._unpack(member_info, data)
