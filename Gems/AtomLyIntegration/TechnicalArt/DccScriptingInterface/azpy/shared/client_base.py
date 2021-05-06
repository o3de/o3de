# coding:utf-8
# !/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# File Description:
# This file is contains OpenImageIO operations for file texture conversions
# -------------------------------------------------------------------------


import socket
import time
import traceback
import json


class ClientBase(object):
    PORT = 17344
    HEADER_SIZE = 10

    def __init__(self, timeout=2):
        self.timeout = timeout
        self.client_socket = None
        self.port = self.__class__.PORT
        self.discard_count = 0

    def connect(self, port=-1):
        if port >= 0:
            self.port = port
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(('localhost', self.port))
            self.client_socket.setblocking(False)
        except:
            traceback.print_exc()
            return False
        return True

    def disconnect(self):
        try:
            self.client_socket.close()
        except:
            traceback.print_exc()
            return False
        return True

    def send(self, cmd):
        json_cmd = json.dumps(cmd)
        message = ['{0:10d}'.format(len(json_cmd.encode())), json_cmd]  # header

        try:
            msg_str = ''.join(message)
            self.client_socket.sendall(msg_str.encode())
        except Exception:
            traceback.print_exc()
            return None
        return self.recv()

    def recv(self):
        total_data = []
        data = ''
        reply_length = 0
        bytes_remaining = ClientBase.HEADER_SIZE

        start_time = time.time()
        while time.time() - start_time < self.timeout:
            try:
                data = self.client_socket.recv(bytes_remaining)
            except:
                time.sleep(0.01)
                continue

            if data:
                total_data.append(data)

                bytes_remaining -= len(data)
                if bytes_remaining <= 0:
                    for i in range(len(total_data)):
                        total_data[i] = total_data[i].decode()

                    if reply_length == 0:
                        header = ''.join(total_data)
                        reply_length = int(header)
                        bytes_remaining = reply_length
                        total_data = []
                    else:
                        if self.discard_count > 0:
                            self.discard_count -= 1
                            return self.recv()
                        reply_json = ''.join(total_data)
                        return json.loads(reply_json)

        self.discard_count += 1
        raise RuntimeError('Timeout waiting for response.')

    def ping(self):
        cmd = {
            'cmd': 'ping'
        }

        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return True
        else:
            return False

    @staticmethod
    def is_valid_reply(reply):
        if not reply:
            return False
        if not reply['success']:
            print('[ERROR] {} failed: {}'.format(reply['cmd'], reply['msg']))
            return
        return True


if __name__ == '__main__':
    client = ClientBase()
    if not client.connect():
        print('Failed to connect')
