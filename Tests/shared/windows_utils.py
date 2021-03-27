"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Small library of functions to support autotests for utilizing Windows Utilities

"""

import logging
import psutil
import os
import subprocess
import pyscreenshot as winScreenshot

from _winreg import *

logger = logging.getLogger(__name__)

def kill_app_by_name(project):
    """
    Kills the app on windows for the specified app name
    :param name: name of app running on the devkit to kill
    """
    process_name = project  + 'Launcher.exe'
    for proc in psutil.process_iter():
        # check whether the process to kill name matches
        if proc.name() == process_name:
            proc.kill()

def take_screenshot(result_path, image_filename, project=None):
    """
    Takes a windows screenshot
    :param result_path: path for the output file
    :param image_filename: filename for the new image
    :param project: project name for the running process, if provided only the window for this process will be captured
    """
    if not os.path.exists(result_path):
        os.mkdir(result_path)

    filename = "{}.png".format(image_filename)
    filename = os.path.join(result_path, filename)

    image = winScreenshot.grab() # bbox=(10, 10, 510, 510))  # X1,Y1,X2,Y2
    image.save(filename)

    if not os.path.exists(filename):
        # Capture failed
        return False

    return True

def launch(bin_path, project, parameters = None):
    command_line = [os.path.join(bin_path, project + 'Launcher.exe')]
    if parameters != None:
       command_line = command_line + parameters
    process = subprocess.Popen(command_line, stdout=subprocess.PIPE)
    process.poll()

def check_registry_key_exits(registry_key):
    """
    Searches the Windows registry for the existance of a registry key
    :param registry_key: The bin directory from which to launch the AssetProcessor executable.
    :return: A boolean value of the existance of the key
    """
    try:
        registryHandle = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
        registryKey = OpenKey(registryHandle, registry_key)
        
        logger.info("Registry Key: {0} found.".format(registry_key))
        registryKey.Close()
        registryHandle.Close()

        return True
    except:
        logger.error("Registry Key: {0} was not found.".format(registry_key))
        return False

def get_registry_key_value(registry_key):
    """
    If a registry key exists, it will return the value else return None
    :param registry_key: The bin directory from which to launch the AssetProcessor executable.
    :return: The value of the registry key value
    """
    if check_registry_key_exits(registry_key):
        logger.log("Retrieving the value of Registry Key '{0}'"
            .format(registry_key))

        registryHandle = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
        registryKey = OpenKey(registryHandle, registry_key)

        keyValue = registryKey.Value()

        registryKey.Close()
        registryHandle.Close()

        return keyValue
    else:
        logger.error("Could not retrieve Registry Key Value since Registry Key '{0}' was not found."
            .format(registry_key))
        return None


def check_registry_key_value(registry_key, expected):
    """
    If registry key exists, then checks that the registry key value is as expected
    :param registry_key: The bin directory from which to launch the AssetProcessor executable.
    :return: A boolean value if the registry key value matches expected
    """
    if check_registry_key_exits(registry_key):

        keyValue = get_registry_key_value(registry_key)

        if str.lower(keyValue) == str.lower(expected):
            logger.info("The value of Registry Key '{0}' matched the expected '{1}'"
                .format(registry_key, expected))
            return True
        else:
            logger.error("The value of Registry Key '{0}' die not match the expected '{1}'"
                .format(registry_key, expected))
            return False
    else:
        logger.error("Could not compare Registry Key Value to expected Registry Key '{0}' was not found."
            .format(registry_key))
        return False
