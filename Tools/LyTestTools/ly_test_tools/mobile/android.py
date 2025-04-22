"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Utilities for interacting with Android devices.
"""
import datetime
import logging
import os
import psutil
import subprocess

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
import ly_test_tools._internal.exceptions as exceptions

logger = logging.getLogger(__name__)

SINGLE_DEVICE = 0
MULTIPLE_DEVICES = 1
NO_DEVICES = 2


def can_run_android():
    """
    Determine if android can be run by trying to use adb.

    :return: True if the adb command returns success and False otherwise.
    """
    try:
        with open(os.devnull, 'wb') as DEVNULL:
            return_code = process_utils.safe_check_call(["adb", "version"], stdout=DEVNULL, stderr=subprocess.STDOUT)
            if return_code == 0:
                return True
    except Exception:  # purposefully broad
        logger.info("Android not enabled")
        logger.debug("Attempt to verify adb installation failed", exc_info=True)
        return False


def check_adb_connection_state():
    """
    Wrapper for gathering output of adb get-state command.

    :return: The output of the adb command as an int, raises RunTimeError otherwise.
    """
    with psutil.Popen('adb get-state', stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
        output = proc.communicate()[0].decode('utf-8')

    if 'more than one device' in output:
        return MULTIPLE_DEVICES
    elif 'no devices/emulators found' in output:
        return NO_DEVICES
    elif 'device' == output.strip():
        return SINGLE_DEVICE
    else:
        raise exceptions.LyTestToolsFrameworkException("Detected unhandled output from adb get-state: {}".format(output.strip()))


def reverse_tcp(device, host_port, device_port):
    """
    Tunnels a TCP port over USB from the device to the local host.

    :param device: Device id of a connected device
    :param host_port: Port to reverse to
    :param device_port: Port to reverse from
    :return: None
    """
    logger.debug('Running ADB reverse command')
    cmd = ['adb', '-s', device, 'reverse', f'tcp:{host_port}', f'tcp:{device_port}']
    logger.debug(f'Executing command: "{cmd}"')
    process_utils.check_output(cmd)


def forward_tcp(device, host_port, device_port):
    """
    Tunnels a TCP port over USB from the local host to the device.

    :param device: Device id of a connected device
    :param host_port: Port to forward from
    :param device_port: Port to forward to
    :return: None
    """
    logger.debug('Running ADB forward command')
    cmd = ['adb', '-s', device, 'forward', 'tcp:{}'.format(host_port), 'tcp:{}'.format(device_port)]
    process_utils.check_output(cmd)
    logger.debug('Executing command: %s' % cmd)


def undo_tcp_port_changes(device):
    """
    Undoes all 'adb forward' and 'adb reverse' commands for forwarding and reversing TCP ports.

    :param device: Device id of a connected device
    :return: None
    """
    logger.debug('Reverting "adb forward" and "adb reverse" commands.')
    undo_tcp_forward = ['adb', '-s', device, 'forward', '--remove-all']
    undo_tcp_reverse = ['adb', '-s', device, 'reverse', '--remove-all']
    process_utils.check_output(undo_tcp_forward)
    process_utils.check_output(undo_tcp_reverse)
    logger.debug('Reverted forwarded/reversed TCP ports using commands: {} && {}'.format(
        undo_tcp_forward, undo_tcp_reverse))


def get_screenshots(device, package_name, project):
    """
    Captures a Screenshot for the game and stores it in the project folder on the Devices.

    :param device: Device id of a connected device
    :param package_name: Name of the Android package
    :param project: Name of the lumberyard project
    :return: None
    """
    screenshot_cmd = ['adb',
                      '-s',
                      device,
                      'shell',
                      'screencap',
                      '-p',
                      '/sdcard/Android/data/{}/files/log/{}-{}.png'.format(package_name, project, device)]
    process_utils.check_output(screenshot_cmd)
    logger.debug('Screenshot Command Ran: {}'.format(screenshot_cmd))


def pull_files_to_pc(package_name, logs_path, device=None):
    """
    Pulls a file from the package installed on the device to the PC.

    :param device: ID of a connected Android device
    :param package_name: Name of the Android package
    :param logs_path: Path to the logs location on the local machine
    :return: None
    """
    directory = os.path.join(logs_path, device)
    if not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)

    pull_cmd = ['adb']
    if device is not None:
        pull_cmd.extend(['-s', device])
    pull_cmd.extend(['pull', '/sdcard/Android/data/{}/files/log/'.format(package_name), directory])

    try:
        process_utils.check_output(pull_cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as err:
        if 'does not exist' in err.output:
            logger.info('Could not pull logs since none exist on device {}'.format(device))
        else:
            raise exceptions.LyTestToolsFrameworkException from err

    logger.debug('Pull File Command Ran successfully: {}'.format(str(pull_cmd)))


def push_files_to_device(source, destination, device=None):
    """
    Pushes a file to a specific location. Source being from the PC and the destination is the Android destination
    Params.

    :param source: The file location on the host machine
    :param destination: The destination on the Android device we want to push the files
    :param device: The device ID of the device to push files to
    :return: None
    """

    logger.debug('Pushing files from windows location {} to device {} location {}'.format(source, device, destination))

    cmd = ['adb']
    if device is not None:
        cmd.extend(["-s", device])
    cmd.extend(["push", source, destination])

    push_result = process_utils.check_output(cmd)
    logger.debug('Push File Command Ran: {}'.format(str(cmd)))
    if 'pushed' not in push_result:
        raise exceptions.LyTestToolsFrameworkException('[AndroidLauncher] Failed to push file to device: {}!'.format(device))


def start_adb_server():
    """
    Starts the ADB server.

    :return: None
    """
    logger.debug('Starting adb server')
    cmd = 'adb start-server'
    process_utils.check_call(cmd)


def kill_adb_server():
    """
    Kills the ADB server.

    :return: None
    """
    logger.debug('Killing adb server')
    cmd = 'adb kill-server'
    process_utils.check_call(cmd)


def wait_for_android_device_load(android_device_id, timeout=60):
    """
    Utilizes adb logcat commands to make sure the device is fully loaded before connecting the RemoteConsole()
    Helps deal with race conditions that may occur when calls are made to the LY client before loading is complete.

    :param android_device_id: string ID for the Android device to target.
    :param timeout: int seconds to wait until raising an exception.
    :return: output from the command if it succeeds, raises an exception otherwise.
    """
    adb_prefix = ['adb', '-s', android_device_id]
    current_time = datetime.datetime.now().strftime('%m-%d %H:%M:%S.%f')  # Example output: '01-28 16:56:28.271000'
    wait_command = []
    wait_command.extend(adb_prefix)
    wait_command.extend(['logcat',
                         '-e',
                         '\\bFinished loading textures\\b',  # exact regex match for 'Finished loading textures'
                         '-t',
                         current_time])

    try:
        waiter.wait_for(
            lambda: process_utils.check_output(wait_command),
            timeout=timeout,
            exc=subprocess.CalledProcessError)
    except subprocess.CalledProcessError:
        logger.exception("Android device with ID: {} never finished loading".format(android_device_id))


def get_devices():
    """
    Utilizes the 'adb devices' command to check that a device is connected to the host machine.

    :return: A list of connected device IDs or an empty list if none are found.
    """
    devices_list = []
    cmd = 'adb devices'
    # Example cmd_output: 'List of devices attached\r\nemulator-5554\tdevice\r\nA1B2C3D4E5\tdevice\r\n\r\n'
    cmd_output = process_utils.check_output(cmd)
    # Example raw_devices_output: ['List of devices attached', 'emulator-5554\tdevice', 'A1B2C3D4E5\tdevice']
    raw_devices_output = cmd_output.strip().splitlines()
    for raw_output in raw_devices_output:
        updated_raw_output = raw_output.split('\t')  # ['emulator-5554', 'device'] or ['List of devices attached']
        if len(updated_raw_output) > 1:
            devices_list.append(updated_raw_output[0])

    return devices_list  # Example devices_list: ['emulator-5554', 'A1B2C3D4E5']
