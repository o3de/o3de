"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import psutil
import socket


logger = logging.getLogger(__name__)


def check_for_listening_port(port):
    """
    Checks to see if the connection to the designated port was established.
    :param port: Port to listen to.
    :return: True if port is listening.
    """
    port_listening = False
    for conn in psutil.net_connections():
        if 'port={}'.format(port) in str(conn):
            port_listening = True
    return port_listening


def check_for_remote_listening_port(port, ip_addr='127.0.0.1'):
    """
    Tries to connect to a port to see if port is listening.
    :param port: Port being tested.
    :param ip_addr: IP address of the host being connected to.
    :return: True if connection to the port is established.
    """
    port_listening = True
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((ip_addr, port))
    except socket.error as err:
        # Socket error: Connection refused, error code 10061
        if err.errno == 10061:
            port_listening = False
    finally:
        sock.close()
    return port_listening


def get_local_ip_address():
    """
    Finds the IP address for the primary ethernet adapter by opening a connection and grabbing its IP address.
    :return: The IP address for the adapter used to make the connection.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Connecting to Google's public DNS so there is an open connection
        # and then getting the address used for that connection
        sock.connect(('8.8.8.8', 80))
        host_ip = sock.getsockname()[0]
    finally:
        sock.close()
    return host_ip
