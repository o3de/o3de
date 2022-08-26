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
    PORT = 17344
    HEADER_SIZE = 10

    def __init__(self, timeout=2):
        self.timeout = timeout
        self.client_socket = None
        self.port = self.__class__.PORT

    def connect(self, port=-1):
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
        message = json_cmd
        message = f'{len(message):<{self.HEADER_SIZE}}' + message
        _LOGGER.info(f'Message: {message}')
        try:
            _LOGGER.info(f'Encode: {message.encode()}')
            self.client_socket.sendall(message.encode())
        except Exception as e:
            _LOGGER.info(f'Send Exception [{type(e)}] ::: {e}')
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
                _LOGGER.info(f'RECV DATA: {data.decode()}')
            except Exception:
                time.sleep(0.01)
                continue

            if data:
                total_data.append(data)

                bytes_remaining -= len(data)
                if bytes_remaining <= 0:
                    for i in range(len(total_data)):
                        total_data[i] = total_data[i].decode()
                        _LOGGER.info(f'---> {total_data[i]}')

                    if reply_length == 0:
                        header = ''.join(total_data)
                        reply_length = int(header)
                        bytes_remaining = reply_length
                        total_data = []
                    else:
                        reply_json = ''.join(total_data)
                        return json.loads(reply_json)
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
        if not reply:
            _LOGGER.info('[ERROR] Invalid Reply')
            return False

        if not reply['success']:
            _LOGGER.info(f"[ERROR] {reply['cmd']} failed: {reply['msg']}")
            return
        return True


if __name__ == '__main__':
    _LOGGER.info('Socket Communication- ClientBase started...')
    client = ClientBase(timeout=10)
    if client.connect():
        _LOGGER.info('Connected successfully')
        _LOGGER.info(client.ping())
        _LOGGER.info(client.echo('Hello World!'))
        _LOGGER.info(client.set_title('New Window Title'))
        _LOGGER.info(client.sleep())

        if client.disconnect():
            _LOGGER.info('Disconnected successfully')
    else:
        _LOGGER.info('Failed to connect')

