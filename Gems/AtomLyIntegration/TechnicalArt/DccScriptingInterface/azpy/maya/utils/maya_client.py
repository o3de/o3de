# coding:utf-8
#!/usr/bin/python
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
# -- This line is 75 characters -------------------------------------------
# -- Standard Python modules --
import socket
import time
import logging as _logging

# -- External Python modules --
from azpy.shared.client_base import ClientBase

# -- Extension Modules --
# none (yet)

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.maya.utils.maya_client'
_LOGGER = _logging.getLogger(_MODULENAME)

_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class MayaClient(ClientBase):
    PORT = 17337

    def echo(self, text):
        cmd = {
            'cmd': 'echo',
            'text': text
        }
        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return reply['result']
        else:
            return None

    def set_title(self, title):
        cmd = {
            'cmd': 'set_title',
            'title': title
        }
        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return reply['result']
        else:
            return None

    def sleep(self):
        cmd = {
            'cmd':  'sleep',
        }
        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return reply['result']
        else:
            return None


if __name__ == '__main__':
    client = MayaClient(timeout=10)
    if client.connect():
        _LOGGER.info('Connected successfully')
        _LOGGER.info(client.ping())
        _LOGGER.info(client.echo('HelloWorld!'))
        _LOGGER.info(client.set_title('Maya Server New'))
        _LOGGER.info(client.sleep())
        if client.disconnect():
            _LOGGER.info('Disconnected successfully')
