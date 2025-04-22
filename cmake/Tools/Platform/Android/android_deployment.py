#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import datetime
import logging
import os
import json
import platform
import subprocess
import sys
import time
import pathlib
import shutil

# Resolve the common python module
ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_support

# The following is the list of known android external storage paths that we will attempt to verify on a device and
# return the first one that is detected
KNOWN_ANDROID_EXTERNAL_STORAGE_PATHS = [
    '/sdcard/',
    '/storage/emulated/0/',
    '/storage/emulated/legacy/',
    '/storage/sdcard0/',
    '/storage/self/primary/',
]

ANDROID_TARGET_TIMESTAMP_FILENAME = 'deploy.timestamp'


class AndroidDeployment(object):
    """
    Class to manage the deployment of game assets to an android device (Separately from the APK)
    """

    DEPLOY_APK_ONLY = 'APK'
    DEPLOY_ASSETS_ONLY = 'ASSETS'
    DEPLOY_BOTH = 'BOTH'

    def __init__(self, dev_root, build_dir, configuration, android_device_filter, clean_deploy, android_sdk_path, deployment_type, game_name=None, asset_mode=None, asset_type=None, embedded_assets=True, is_unit_test=False, kill_adb_server=False):
        """
        Initialize the Android Deployment Worker

        :param dev_root:                The dev-root of the engine
        :param android_device_filter:   An optional list of devices to filter on the connected devices to deploy to. If not supplied, deploy to all devices
        :param clean_deploy:            Option to clean the target device's assets before deploying the game's assets from the host
        :param android_sdk_path:        Path to the android SDK (to use the adb tool)
        :param deployment_type:         The type of deployment (DEPLOY_APK_ONLY, DEPLOY_ASSETS_ONLY, or DEPLOY_BOTH)
        :param game_name:               The name of the game whose assets are being deployed. None if is_test_project is True
        :param asset_mode:              The asset mode of deployment (LOOSE, PAK, VFS). None if is_test_project is True
        :param asset_type:              The asset type. None if is_test_project is True
        :param embedded_assets:         Boolean to indicate if the assets are embedded in the APK or not
        :param is_unit_test:            Boolean to indicate if this is a unit test deployment
        :param kill_adb_server:         Boolean to indicate if it should kill adb server at the end of deployment
        """

        self.dev_root = pathlib.Path(dev_root)

        self.build_dir = self.dev_root / build_dir

        self.configuration = configuration

        self.game_name = game_name

        self.asset_mode = asset_mode

        self.asset_type = asset_type

        self.clean_deploy = clean_deploy

        self.embedded_assets = embedded_assets

        self.deployment_type = deployment_type

        self.is_test_project = is_unit_test

        self.kill_adb_server = kill_adb_server

        if self.embedded_assets:
            if self.deployment_type == AndroidDeployment.DEPLOY_ASSETS_ONLY:
                raise common.LmbrCmdError(f"Deployment type {deployment_type} set but the assets are embedded in the APK. To deploy assets, build the APK again.")

            # If assets are embedded in the APK then deploying both (apk and assets) just means deploy the APK.
            if self.deployment_type == AndroidDeployment.DEPLOY_BOTH:
                self.deployment_type = AndroidDeployment.DEPLOY_APK_ONLY

        if not self.is_test_project:

            if asset_mode == 'PAK':
                self.local_asset_path = self.dev_root / 'Pak' / f'{game_name.lower()}_{asset_type}_paks'
            else:
                # Assets layout folder when assets are not included into APK is 'app/src/assets'
                self.local_asset_path = self.build_dir / 'app/src/assets'

            assert game_name is not None, f"'game_name' is required"
            self.game_name = game_name

            assert asset_mode is not None, f"'asset_mode' is required"
            self.asset_mode = asset_mode

            assert asset_type is not None, f"'asset_type' is required"
            self.asset_type = asset_type

            self.files_in_asset_path = list(self.local_asset_path.glob('**/*'))

            self.android_settings = AndroidDeployment.read_android_settings(self.dev_root, game_name)
        else:
            self.local_asset_path = None

            if asset_mode:
                logging.warning(f"'asset_mode' argument '{asset_mode}' ignored for unit test deployment.")
            if asset_type:
                logging.warning(f"'asset_type' argument '{asset_type}' ignored for unit test deployment.")

            self.files_in_asset_path = []

        self.apk_path = self.build_dir / 'app' / 'build' / 'outputs' / 'apk' / configuration / f'app-{configuration}.apk'

        self.android_device_filter = [android_device.strip() for android_device in android_device_filter.split(',')] if android_device_filter else []

        self.adb_path = AndroidDeployment.resolve_adb_tool(pathlib.Path(android_sdk_path))

        self.adb_started = False

    @staticmethod
    def read_android_settings(dev_root, game_name):
        """
        Read and parse the android_project.json file into a dictionary to process the specific attributes needed for the manifest template

        :param dev_root:    The dev root we are working from
        :param game_name:   Name of the game under the dev root
        :return: The android settings for the game project if any
        """
        game_folder = dev_root / game_name
        game_folder_project_properties_path = game_folder / 'Platform' / 'Android' / 'android_project.json'
        game_project_properties_content = game_folder_project_properties_path.resolve(strict=True)\
                                                                             .read_text(encoding=common.DEFAULT_TEXT_READ_ENCODING,
                                                                                        errors=common.ENCODING_ERROR_HANDLINGS)
        # Extract the key attributes we need to process and build up our environment table
        game_project_json = json.loads(game_project_properties_content)
        android_settings = game_project_json.get('android_settings', {})

        return android_settings

    @staticmethod
    def resolve_adb_tool(android_sdk_path):
        """
        Resolve the location of the adb tool, first check if the system can
        find one, otherwise look for it based on the input Android SDK Path.
        :param android_sdk_path: The android SDK path to search for the adb tool
        :return: The absolute path to the adb tool
        """
        adb_target = shutil.which('adb')
        if adb_target:
            return pathlib.Path(adb_target)
        else:
            adb_target = 'adb.exe' if platform.system() == 'Windows' else 'adb'
            check_adb_target = android_sdk_path / 'platform-tools' / adb_target
            if not check_adb_target.exists():
                raise common.LmbrCmdError(f"Invalid Android SDK path '{str(android_sdk_path)}': Unable to locate '{adb_target}'.")
            return check_adb_target

    def get_android_project_settings(self, key_name, default_value):
        return self.android_settings.get(key_name, default_value)

    def adb_call(self, arg_list, device_id=None):
        """
        Wrapper to execute the adb command-line tool

        :param arg_list:    Argument list to send to the tool
        :param device_id:   Optional device id (from the 'get_target_android_devices' call) to invoke the call to.
        :return: The stdout result of the call
        """

        if isinstance(arg_list, str):
            arg_list = [arg_list]

        call_arguments = [str(self.adb_path.resolve())]

        if device_id:
            call_arguments.extend(['-s', device_id])

        call_arguments.extend(arg_list)
        logging.debug(f"adb command: {subprocess.list2cmdline(call_arguments)}")

        try:
            output = subprocess.check_output(subprocess.list2cmdline(call_arguments),
                                             shell=True,
                                             stderr=subprocess.PIPE).decode(common.DEFAULT_TEXT_READ_ENCODING,
                                                                               common.ENCODING_ERROR_HANDLINGS)
            logging.debug(f"adb output:\n{output}")
            return output
        except subprocess.CalledProcessError as err:
            std_out = err.stdout.decode(common.DEFAULT_TEXT_READ_ENCODING, common.ENCODING_ERROR_HANDLINGS)
            std_err = err.stderr.decode(common.DEFAULT_TEXT_READ_ENCODING, common.ENCODING_ERROR_HANDLINGS)
            logging.debug(f"adb returned non-zero.\noutput:\n{std_out}\nerror:\n{std_err}\n")
            raise common.LmbrCmdError(std_err)

    def adb_shell(self, command, device_id):
        """
        Special wrapper around calling "adb shell" which will invoke a shell command on the android device

        :param command:         The shell command to invoke on the android device
        :param device_id:       The device id (from the 'get_target_android_devices' call) to invoke the shell call on
        :return: The stdout result of the call
        """
        shell_command = ['shell', command]

        return self.adb_call(shell_command, device_id=device_id)

    def adb_ls(self, path, device_id, args=None):
        """
        Request an 'ls' call on the android device

        :param path:        The path to perform the 'ls' call on
        :param device_id:   device id (from the 'get_target_android_devices' call) to invoke the shell call on
        :param args:        Additional args to pass into the l'ls' call
        :return: Tuple of Boolean result of the call and the output of the call
        """
        error_messages = [
            'No such file or directory',
            'Permission denied'
        ]

        shell_command = ['ls']

        if args:
            shell_command.extend(args)

        shell_command.append(path)

        raw_output = self.adb_shell(command=' '.join(shell_command),
                                    device_id=device_id)

        if not raw_output:
            return False, None

        if raw_output is None or any([error for error in error_messages if error in raw_output]):
            status = False
        else:
            status = True

        return status, raw_output

    def get_target_android_devices(self):
        """
        Gets all of the connected android devices with adb, filtered by the set optional device filter
        :return: list of serial numbers of optionally filtered connected devices.
        """

        connected_devices = []

        # Call adb to get the device list and process the raw response
        raw_devices_output = self.adb_call("devices")
        if not raw_devices_output:
            raise common.LmbrCmdError("Error getting connected devices through adb")

        device_output_list = raw_devices_output.split(os.linesep)
        for device_output in device_output_list:
            if any(x in device_output for x in ['List', '*']):
                logging.debug(f"Skipping the following line as it has 'List', '*' or 'emulator' in it: {device_output}")
                continue

            device_serial = device_output.split()
            if device_serial:
                if 'unauthorized' in device_output.lower():
                    logging.warning(f"Device {device_serial[0]} is not authorized for development access. Please reconnect the device and check for a confirmation dialog.")
                elif device_serial[0] in self.android_device_filter or not self.android_device_filter:
                    connected_devices.append(device_serial[0])
                else:
                    logging.debug(f"Skipping filtered out Device {device_serial[0]} .")

        if not connected_devices:
            raise common.LmbrCmdError("No connected android devices found")

        return connected_devices

    def check_known_android_paths(self, device_id):
        """
        Look for a known android path that is writeable and return the first one that is found

        :param device_id:   The device id (from the 'get_target_android_devices' call) to invoke the shell call on
        :return: The first available android path if found, None if not
        """
        for path in KNOWN_ANDROID_EXTERNAL_STORAGE_PATHS:
            logging.debug(f"Checking known path '{path}' on device '{device_id}'")

            # Test the path by performing an 'ls' call on it and checking if an error is returned from the result
            result, output = self.adb_ls(path=path,
                                         args=None,
                                         device_id=device_id)
            if result:
                return path[:-1]

        return None

    def detect_device_storage_path(self, device_id):
        """
        Uses the device's environment variable "EXTERNAL_STORAGE" to determine the correct
        path to public storage that has write permissions.  If at any point does the env var
        validation fail, fallback to checking known possible paths to external storage.
        :param device_id:
        :return: The first available storage device
        """

        external_storage = self.adb_shell(command="set | grep EXTERNAL_STORAGE",
                                          device_id=device_id)
        if not external_storage:
            logging.debug(f"Unable to get 'EXTERNAL_STORAGE' environment from device '{device_id}'. Falling back to known android paths.")
            return self.check_known_android_paths(device_id)

        # Given the 'EXTERNAL_STORAGE' environment, parse out the value and validate it
        storage_path_key_value = external_storage.split('=')
        if len(storage_path_key_value) != 2:
            logging.debug(f"The value for 'EXTERNAL_STORAGE' environment from device '{device_id}' does not represent a valid key-value pair: {storage_path_key_value}. Falling back to known android paths")
            return self.check_known_android_paths(device_id)
        # Check the existence and permissions issue of the storage path
        storage_path = storage_path_key_value[1].strip()
        is_external_valid, _ = self.adb_ls(path=storage_path,
                                           device_id=device_id)
        if is_external_valid:
            return storage_path

        # The set external path has an issue, attempt to determine its real path through an adb shell call
        logging.debug(f"The path specified in EXTERNAL_STORAGE seems to have permission issues, attempting to resolve with realpath for device {device_id}.")
        real_path = self.adb_shell(command=f'realpath {storage_path}',
                                   device_id=device_id)
        if not real_path:
            logging.debug(f"Unable to determine the real path '{storage_path}' (from EXTERNAL_STORAGE) for {self.game_name} on device {device_id}. Falling back to known android paths")
            return self.check_known_android_paths(device_id)
        real_path = real_path.strip()
        is_external_valid, _ = self.adb_ls(path=real_path,
                                           device_id=device_id)
        if is_external_valid:
            return real_path

        logging.debug(f'Unable to validate the resolved EXTERNAL_STORAGE environment variable path for device {device_id}.')
        return self.check_known_android_paths(device_id)

    def get_device_file_timestamp(self, remote_file_path, device_id):
        """
        Get the integer timestamp value of a file from a given device.

        :param remote_file_path:    The path to the timestamp file on the android device
        :param device_id:           The device id (from the 'get_target_android_devices' call) to invoke the shell call on
        :return:    The time value if found,  None if not
        """
        try:
            timestamp_string = self.adb_shell(command=f'cat {remote_file_path}',
                                              device_id=device_id).strip()
        except (common.LmbrCmdError, AttributeError):
            return None

        if not timestamp_string:
            return None

        for fmt in ('%Y-%m-%d %H:%M:%S', '%Y-%m-%d %H:%M:%S.%f'):
            try:
                target_time = time.mktime(time.strptime(timestamp_string, fmt))
                break
            except ValueError:
                target_time = None

        return target_time

    def update_device_file_timestamp(self, relative_assets_path, device_id):
        """
        Update the device timestamp file on an android device to track files that need updating on pushes

        :param relative_assets_path:    The relative path to the assets on the android device
        :param device_id:               The device id (from the 'get_target_android_devices' call) to invoke the shell call on
        """
        timestamp_str = str(datetime.datetime.now())
        logging.debug(f"Updating timestamp on device {device_id} to {timestamp_str}")

        local_timestamp_file_path = self.local_asset_path / ANDROID_TARGET_TIMESTAMP_FILENAME
        local_timestamp_file_path.write_text(timestamp_str)
        target_timestamp_file_path = f'{relative_assets_path}/{ANDROID_TARGET_TIMESTAMP_FILENAME}'

        self.adb_call(arg_list=['push', str(local_timestamp_file_path), target_timestamp_file_path],
                      device_id=device_id)

    @staticmethod
    def should_copy_file(check_path, check_time):
        """
        Check if a source file should be copied, by checking if its timestamp is newer than the 'check_time'
        :param check_path:  The path to the source file whose timestamp will be evaluated
        :param check_time:  The baseline 'check_time' value to compare the source file timestamp against
        :return: True if the source file is newer than the baseline 'check_time', False if not
        """

        if not check_path.is_file():
            return False

        stat_src = check_path.stat()
        should_copy = stat_src.st_mtime >= check_time
        return should_copy

    def check_package_installed(self, package_name, target_device):
        """
        Checks if the package for the game is currently installed or not
        @param package_name:    The name of the package to search for
        @param target_device:   The target device to search for the package on
        @return: True if there an existing package on the device with the same package name, false if not
        """
        output_result = self.adb_call(['shell', 'cmd', 'package', 'list', 'packages', package_name],
                                      target_device)
        return output_result != ''

    def install_apk_to_device(self, target_device):
        """
        Install the APK to a target device
        @param target_device: The device id of the connected device to install to
        """
        if self.is_test_project:
            android_package_name = android_support.TEST_RUNNER_PACKAGE_NAME
        else:
            android_package_name = self.get_android_project_settings(key_name='package_name',
                                                                     default_value='org.o3de.sdk')

        if self.clean_deploy and self.check_package_installed(android_package_name, target_device):
            logging.info(f"Device '{target_device}': Uninstalling pre-existing APK for {self.game_name} ...")
            self.adb_call(arg_list=['uninstall', android_package_name],
                          device_id=target_device)

        logging.info(f"Device '{target_device}': Installing APK for {self.game_name} ...")
        self.adb_call(arg_list=['install', '-t', '-r', str(self.apk_path.resolve())],
                      device_id=target_device)

    def path_exists_on_device(self, path, device_id):
        try:
            result, _ = self.adb_ls(path=path,
                                        args=None,
                                        device_id=device_id)
            return result
        except (common.LmbrCmdError, AttributeError):
            return False

    def create_path_on_device(self, path, device_id):
        if not self.path_exists_on_device(path, device_id):
            self.adb_shell(command=f'mkdir {path}', device_id=device_id)

    def install_assets_to_device(self, detected_storage, target_device):
        """
        Install the assets for the game to a target device

        @param detected_storage:    The detected storage path on the target device
        @param target_device:       The ID of the target device
        """
        assert not self.is_test_project

        android_package_name = self.get_android_project_settings(key_name='package_name',
                                                                 default_value='org.o3de.sdk')
        output_package = f'{detected_storage}/Android/data/{android_package_name}'
        output_target = f'{output_package}/files'
        device_timestamp_file = f'{output_target}/{ANDROID_TARGET_TIMESTAMP_FILENAME}'

        # Track the current timestamp if possible to see if we can incrementally push files rather
        # than always pushing all files
        target_timestamp = self.get_device_file_timestamp(remote_file_path=device_timestamp_file,
                                                          device_id=target_device)

        if self.clean_deploy:
            logging.info(f"Device '{target_device}': Cleaning target assets before deployment...")
            self.adb_shell(command=f'rm -rf {output_target}',
                           device_id=target_device)
            logging.info(f"Device '{target_device}': Target cleaned.")

        # On certain devices pushing files with adb fails with 'remote secure_mkdirs failed' error.
        # Creating all dirs first on target to surpass the issue.
        self.create_path_on_device(output_package, device_id=target_device)
        self.create_path_on_device(output_target, device_id=target_device)
        for asset_path in self.files_in_asset_path:
            if asset_path.is_dir():
                relative_path = asset_path.relative_to(self.local_asset_path).as_posix()
                target_path = f"{output_target}/{relative_path}"
                self.create_path_on_device(target_path, device_id=target_device)

        if self.clean_deploy or not target_timestamp:
            logging.info(f"Device '{target_device}': Pushing {len(self.files_in_asset_path)} files from {str(self.local_asset_path)} to device {output_target} ...")
            try:
                # '/.' is necessary in the source path to not copy folder 'assets' to destination, but its content.
                self.adb_call(arg_list=['push', f'{str(self.local_asset_path)}/.', output_target],
                                device_id=target_device)
            except common.LmbrCmdError as err:
                # Something went wrong, clean up before leaving
                self.adb_shell(command=f'rm -rf {output_target}',
                                device_id=target_device)
                raise err

        else:
            # If no clean was specified, individually inspect all files to see if it needs to be updated
            files_to_copy = []
            for asset_file in self.files_in_asset_path:
                # TODO: Check if the target exists in the destination as well?
                if AndroidDeployment.should_copy_file(asset_file, target_timestamp):
                    files_to_copy.append(asset_file)

            if len(files_to_copy) > 0:
                logging.info(f"Copying {len(files_to_copy)} assets to device {target_device}")

            for src_path in files_to_copy:
                relative_path = src_path.relative_to(self.local_asset_path).as_posix()
                target_path = f"{output_target}/{relative_path}"
                self.adb_call(arg_list=['push', str(src_path), target_path],
                              device_id=target_device)

        self.update_device_file_timestamp(relative_assets_path=output_target,
                                          device_id=target_device)

    def execute(self):
        """
        Execute the asset deployment
        """

        if self.is_test_project:
            if not self.apk_path.is_file():
                raise common.LmbrCmdError(f"Missing apk for {android_support.TEST_RUNNER_PROJECT} ({str(self.apk_path)}). Make sure it is built and is set as a signed APK.")
        else:
            if self.deployment_type in (AndroidDeployment.DEPLOY_APK_ONLY, AndroidDeployment.DEPLOY_BOTH):
                if not self.apk_path.is_file():
                    raise common.LmbrCmdError(f"Missing apk for game {self.game_name} ({str(self.apk_path)}). Make sure it is built and is set as a signed APK.")

            if self.deployment_type in (AndroidDeployment.DEPLOY_ASSETS_ONLY, AndroidDeployment.DEPLOY_BOTH):
                if not self.local_asset_path.is_dir():
                    raise common.LmbrCmdError(f"Missing {self.asset_type} assets for game {self.game_name} .")

        try:
            logging.debug("Starting ADB Server")

            self.adb_call('start-server')
            self.adb_started = True

            # Get the list of target devices to deploy to
            target_devices = self.get_target_android_devices()
            if not target_devices:
                raise common.LmbrCmdError("No connected and eligible android devices found")

            for target_device in target_devices:
                detected_storage = self.detect_device_storage_path(target_device)
                if not detected_storage:
                    logging.warning(f"Unable to resolve storage path for device '{target_device}'. Skipping.")
                    continue

                if self.is_test_project:
                    # If this is the unit test runner, then only install the APK, assets are not applicable
                    self.install_apk_to_device(target_device=target_device)
                else:
                    # Otherwise install the apk and assets based on the deployment type
                    if self.deployment_type in (AndroidDeployment.DEPLOY_APK_ONLY, AndroidDeployment.DEPLOY_BOTH):
                        self.install_apk_to_device(target_device=target_device)

                    if self.deployment_type in (AndroidDeployment.DEPLOY_ASSETS_ONLY, AndroidDeployment.DEPLOY_BOTH):
                        # Make sure the APK is installed first
                        android_package_name = self.get_android_project_settings(key_name='package_name',
                                                                                    default_value='org.o3de.sdk')
                        if not self.check_package_installed(package_name=android_package_name,
                                                            target_device=target_device):
                            raise common.LmbrCmdError(f"Unable to locate APK for {self.game_name} on device '{target_device}'. Make sure it is installed "
                                                        f"first before installing the assets.")

                        self.install_assets_to_device(detected_storage=detected_storage,
                                                      target_device=target_device)

                logging.info(f"{self.game_name} deployed to device {target_device}")
        finally:
            if self.adb_started and self.kill_adb_server:
                self.adb_call('kill-server')
