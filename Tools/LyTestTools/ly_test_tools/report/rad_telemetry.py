"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Helpers for RAD Telemetry, currently only for Windows
"""

import logging
import subprocess
import os

import ly_test_tools.environment.process_utils as process_utils
from ly_test_tools import WINDOWS

_RAD_DEFAULT_PORT = 4719

_CREATE_NEW_PROCESS_GROUP = 0x00000200
_DETACHED_PROCESS = 0x00000008
_WINDOWS_FLAGS = _CREATE_NEW_PROCESS_GROUP | _DETACHED_PROCESS

RAD_TOOLS_SUBPATH = os.path.join("dev", "Gems", "RADTelemetry", "Tools")

log = logging.getLogger(__name__)


def __set_firewall_rule(direction, port):
    """
    Adds a Windows firewall rule if one does not yet exist. Requires administrator privilege.

    :param direction: Must be 'in' or 'out'
    :param port: target port to open
    :return: None
    """

    assert WINDOWS, "Only implemented for Windows platforms"
    log.info(f"Setting firewall rule on port '{port}' for direction '{direction}'")

    show_rule = ['netsh', 'advfirewall', 'firewall', 'show', 'rule', 'name=RADTelemetry', f'dir={direction}']
    show_result = process_utils.safe_check_call(show_rule)

    if show_result == 0:
        log.debug("Rule already exists")
    else:
        add_rule = ['netsh', 'advfirewall', 'firewall', 'add', 'rule', 'name=RADTelemetry', f'dir={direction}',
                    'action=allow', 'protocol=TCP', f'localport={port}']
        process_utils.check_call(add_rule)
        log.debug("Added new rule")


def set_firewall_rules():
    """
    Opens firewall ports necessary for a remote device to communicate with the RAD Telemetry server.
    Requires administrator privilege.

    :return: None
    """
    assert WINDOWS, "Only implemented for Windows platforms"
    __set_firewall_rule(direction="in", port=_RAD_DEFAULT_PORT)
    __set_firewall_rule(direction="out", port=_RAD_DEFAULT_PORT)


def launch_server(dev_path):
    """
    Launches the RAD Telemetry server to collect telemetry captures.

    :param dev_path: path to the folder containing engineroot.txt
    :return: None
    """
    assert WINDOWS, "Only implemented for Windows platforms"
    server_path = os.path.join(dev_path, RAD_TOOLS_SUBPATH, "tm_server.exe")
    subprocess.Popen([server_path], creationflags=_WINDOWS_FLAGS, close_fds=True)
    log.info(f"Launched RAD Server from {server_path}")


def terminate_servers(dev_path):
    """
    Terminate the RAD Telemetry server and all related tools, important before collecting any of its captures

    :param dev_path: path to the folder containing engineroot.txt
    :return: None
    """
    assert WINDOWS, "Only implemented for Windows platforms"
    rad_path = os.path.join(dev_path, RAD_TOOLS_SUBPATH)
    process_utils.kill_processes_started_from(rad_path)


def get_capture_path(dev_path):
    """
    Returns the path of the tm_server.exe file

    :return: path to the folder containing output for local servers
    """
    assert WINDOWS, "Only implemented for Windows platforms"
    get_folder_path = os.path.join(dev_path, RAD_TOOLS_SUBPATH, "tm_server.exe")
    output = process_utils.check_output([get_folder_path])
    return output.strip()
