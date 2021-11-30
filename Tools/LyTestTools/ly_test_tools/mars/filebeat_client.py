"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import datetime
import json
import socket


class FilebeatExn(Exception):
    pass


class FilebeatClient(object):
    def __init__(self, logger, host="127.0.0.1", port=9000, timeout=20):
        self._logger = logger.getChild("filebeat_client")
        self._filebeat_host = host
        self._filebeat_port = port
        self._socket_timeout = timeout
        self._socket = None

        self._open_socket()

    def send_event(self, payload, index, timestamp=None, pipeline="filebeat"):
        if timestamp is None:
            timestamp = datetime.datetime.utcnow().timestamp()

        event = {
            "index": index,
            "timestamp": timestamp,
            "pipeline": pipeline,
            "payload": json.dumps(payload)
        }

        # Serialise event, add new line and encode as UTF-8 before sending to Filebeat.
        data = json.dumps(event, sort_keys=True) + "\n"
        data = data.encode()

        self._logger.debug(f"-> {data}")
        self._send_data(data)

    def _open_socket(self):
        self._logger.info(f"Connecting to Filebeat on {self._filebeat_host}:{self._filebeat_port}")

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.settimeout(self._socket_timeout)

        try:
            self._socket.connect((self._filebeat_host, self._filebeat_port))
        except (ConnectionError, socket.timeout):
            raise FilebeatExn("Failed to connect to Filebeat") from None

    def _send_data(self, data):
        total_sent = 0

        while total_sent < len(data):
            try:
                sent = self._socket.send(data[total_sent:])
            except BrokenPipeError:
                self._logger.debug("Filebeat socket closed by peer")
                self._socket.close()
                self._open_socket()
                total_sent = 0
            else:
                total_sent = total_sent + sent
