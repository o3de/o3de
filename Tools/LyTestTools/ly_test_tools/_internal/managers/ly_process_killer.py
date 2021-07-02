"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Holds functions for killing pre-existing Lumberyard processes before a test.
"""

import logging

import ly_test_tools.builtin.helpers
import ly_test_tools.environment.process_utils

LY_PROCESS_KILL_LIST = ly_test_tools.environment.process_utils.LY_PROCESS_KILL_LIST  # copy to preserve interface

logger = logging.getLogger(__name__)


class LyProcessKillerException(Exception):
    """Custom exception class for this file."""
    pass


def detect_lumberyard_processes(processes_list):
    # type: (str or list) -> list
    """
    Parses the strings in the processes_list.
    Process names must not include file extensions or they will not be found.
    :param processes_list: str or list of strings representing the Lumberyard processes to search for.
    :return: list of all detected processes parsed from processes_list.
    """
    processes_detected = []

    if type(processes_list) is str:
        processes_list = [processes_list]

    for process_to_detect in processes_list:
        logger.debug('Checking if process named "{}" is running.'.format(process_to_detect))
        if ly_test_tools.environment.process_utils.process_exists(name=process_to_detect, ignore_extensions=True):
            logger.debug('Detected process: "{}" is running.'.format(process_to_detect))
            processes_detected.append(process_to_detect)

    return processes_detected


def kill_processes(processes_list):
    # type: (list) -> None
    """
    Kills Lumberyard processes by name (without file extensions included).
    :param processes_list: list of strings representing process names to kill
    :return: None.
    """
    if type(processes_list) is not list:
        raise LyProcessKillerException('processes_list must be of type "list" for the kill_processes() function.')

    logger.info('Killing list of processes by name: {}'.format(processes_list))
    ly_test_tools.environment.process_utils.kill_processes_named(names=processes_list, ignore_extensions=True)
