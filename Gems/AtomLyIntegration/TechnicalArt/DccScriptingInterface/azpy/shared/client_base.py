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
import config
import socket
import time
import traceback
import json
import logging

_MODULENAME = 'azpy.shared.client_base'
_LOGGER = logging.getLogger(_MODULENAME)


class ClientBase(object):
    def __init__(self, port=17344, timeout=2):
        self.timeout = timeout
        self.header_size = 10
        self.buffer_size = 4096
        self.client_socket = None
        self.port = port
        self.client = None
        self.addr = None
        self.discard_count = 0

    def connect(self, port=-1):
        _LOGGER.info(f'Client connecting to port [{self.port}]')

        if port >= 0:
            self.port = port

        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(('localhost', self.port))
        except Exception as e:
            _LOGGER.info(f'CONNECTION EXCEPTION [{type(e)}] :::: {e}')
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
        try:
            msg_str = f'{len(json_cmd):<{self.header_size}}{json_cmd}'
            _LOGGER.info(f"Message: {msg_str.encode('ascii')}")
            self.client_socket.sendall(msg_str.encode('ascii'))
        except Exception as e:
            _LOGGER.info(f'Send Exception [{type(e)}] ::: {e}')
            traceback.print_exc()
            return None
        return self.recv()

    def recv(self):
        total_data = []
        reply_length = 0
        bytes_remaining = self.header_size

        start_time = time.time()
        while time.time() - start_time < self.timeout:
            try:
                data = self.client_socket.recv(bytes_remaining)
            except Exception:
                time.sleep(0.01)
                continue

            if data:
                total_data.append(data)
                _LOGGER.info(f'Data: {data}')

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
        cmd = {'cmd': 'ping'}
        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return True
        else:
            return False

    @staticmethod
    def is_valid_reply(reply):
        _LOGGER.info(f'Is Valid Reply: {reply}')
        if not reply:
            _LOGGER.info('[ERROR] Invalid Reply')
            return False

        if not reply['success']:
            _LOGGER.info(f"[ERROR] {reply['cmd']} failed: {reply['msg']}")
            return
        return True


if __name__ == '__main__':
    _LOGGER.info('Socket Communication- ClientBase started...')
    # client = ClientBase(timeout=10)
    # if client.connect():
    #     _LOGGER.info('Connected successfully')
    #     _LOGGER.info(client.ping())
    #
    #     if client.disconnect():
    #         _LOGGER.info('Disconnected successfully')
    # else:
    #     _LOGGER.info('Failed to connect')

