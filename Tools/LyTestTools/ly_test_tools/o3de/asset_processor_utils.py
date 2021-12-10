"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import subprocess
import psutil

import ly_test_tools.environment.process_utils as process_utils

logger = logging.getLogger(__name__)
processList = ["AssetProcessor_tmp","AssetProcessor","AssetProcessorBatch","AssetBuilder","rc","Lua Editor"]

def start_asset_processor(bin_dir):
    """
    Starts the AssetProcessor from the given bin directory. Raises a RuntimeError if the process fails.

    :param bin_dir: The bin directory from which to launch the AssetProcessor executable.
    :return: A subprocess.Popen object for the AssetProcessor process.
    """
    os.chdir(bin_dir)
    asset_processor = subprocess.Popen(['AssetProcessor'], env=process_utils.get_display_env())
    return_code = asset_processor.poll()

    if return_code is not None and return_code != 0:
        logger.error("Failed to start AssetProcessor")
        raise RuntimeError("AssetProcessor exited with code {}".format(return_code))
    else:
        logger.info("AssetProcessor is running")
        return asset_processor


def kill_asset_processor():
    """
    Kill the AssetProcessor and all its related processes.

    :return: None
    """
    for n in processList:
        process_utils.kill_processes_named(n, ignore_extensions=True)


# Uses psutil to check if a specified process is running.
def check_ap_running(processName):
    for proc in psutil.process_iter():
        try:
            if processName.lower() in proc.name().lower():
                return True
        except (psutil.AccessDenied, psutil.NoSuchProcess, psutil.ZombieProcess):
            pass
        return False
