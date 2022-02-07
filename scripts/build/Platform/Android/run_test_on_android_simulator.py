#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import os
import pathlib
import re
import sys
import subprocess
import time
import logging

CURRENT_PATH = pathlib.Path(os.path.dirname(__file__)).absolute()

# The engine root is based on the location of this file (<ENGINE_ROOT>/scripts/build/Platform/Android). Walk up to calculate the engine root
ENGINE_ROOT = CURRENT_PATH.parents[3]


class AndroidEmuError(Exception):
    pass


def get_android_sdk_path():
    try:
        android_sdk_path = pathlib.Path(os.getenv('LY_ANDROID_SDK'))
        if not android_sdk_path:
            raise AndroidEmuError(f"LY_ANDROID_SDK environment variable is not set")
        if not android_sdk_path.is_dir():
            raise AndroidEmuError(f"Android SDK Path ('{android_sdk_path}') set with the LY_ANDROID_SDK variable is invalid")
        #TODO: Sanity check on necessary files
        return android_sdk_path
    except Exception as err:
        raise AndroidEmuError(f"Unable to determine android SDK path: {err}")


class Command(object):

    def __init__(self, tool_name, tool_path, run_as_shell=True):

        if not tool_path.is_file():
            raise AndroidEmuError(f"Invalid path for {tool_name}. Cannot find ('{tool_path.absolute()}')")

        self.tool_path = tool_path
        self.run_as_shell = run_as_shell

    def run_return_output(self, cmd_args):

        args = [str(self.tool_path)]
        if isinstance(cmd_args, str):
            args.append(cmd_args)
        elif isinstance(cmd_args, list):
            args.extend(cmd_args)
        else:
            assert False, "run_return_output argument must be a string or list of strings"

        full_cmd = subprocess.list2cmdline(args)
        logging.debug(f"run_return_output: {full_cmd}")

        run_result = subprocess.run(args,
                                    capture_output=True,
                                    encoding='UTF-8',
                                    errors='ignore',
                                    shell=self.run_as_shell)
        if run_result.returncode != 0:
            raise AndroidEmuError(f"Error executing command '{full_cmd}' (return code {run_result.returncode}): {run_result.stderr}")

        return run_result.stdout

    def run(self, cmd_args, cwd=None, suppress_output=False):
        args = [str(self.tool_path)]
        if isinstance(cmd_args, str):
            args.append(cmd_args)
        elif isinstance(cmd_args, list):
            args.extend(cmd_args)
        else:
            assert False, "run_return_output argument must be a string or list of strings"

        full_cmd = subprocess.list2cmdline(args)
        logging.debug(f"run: {full_cmd}")

        run_result = subprocess.run(args,
                                    #stdout=subprocess.DEVNULL if suppress_output else subprocess.STDOUT,
                                    capture_output=False,
                                    shell=self.run_as_shell,
                                    cwd=cwd)
        if run_result.returncode != 0:
            raise AndroidEmuError(f"Error executing command '{full_cmd}' (return code {run_result.returncode}): {run_result.stderr}")

    def run_process(self, cmd_args):
        args = [str(self.tool_path)]
        if isinstance(cmd_args, str):
            args.append(cmd_args)
        elif isinstance(cmd_args, list):
            args.extend(cmd_args)
        else:
            assert False, "run_return_output argument must be a string or list of strings"

        full_cmd = subprocess.list2cmdline(args)
        logging.debug(f"run_process: {full_cmd}")

        process = subprocess.Popen(args,
                                   shell=True,
                                   stdout=subprocess.PIPE,
                                   creationflags=subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.NORMAL_PRIORITY_CLASS |
                                                 subprocess.CREATE_NO_WINDOW,
                                   encoding='UTF-8',
                                   errors='ignore')
        return process


class AndroidEmulatorManager(object):

    UNIT_TEST_AVD_NAME = "LY_UNITTEST_AVD"

    UNIT_TEST_SYSTEM_IMAGE_PACKAGE = "android-30;google_apis;x86_64"

    UNIT_TEST_DEVICE_TEMPLATE_NAME = "pixel_xl"

    UNIT_TEST_DEVICE_SETTINGS_MAP = {
        "disk.dataPartition.size": "32G",
        "vm.heapSize": "1024",
        "hw.ramSize": "2048",
        "hw.sdCard": "no"
    }
    EMULATOR_STARTUP_TIMEOUT_SECS = 60*5  # Set the emulator startup timeout to 5 minutes

    def __init__(self, base_android_sdk_path, hide_emulator_windows=True, force_avd_creation=False, emulator_startup_timeout=EMULATOR_STARTUP_TIMEOUT_SECS):

        self.android_sdk_path = base_android_sdk_path

        self.force_avd_creation = force_avd_creation

        self.unit_test_avd_name = AndroidEmulatorManager.UNIT_TEST_AVD_NAME
        self.unit_test_device_template_name = AndroidEmulatorManager.UNIT_TEST_DEVICE_TEMPLATE_NAME
        self.unit_test_device_settings_map = AndroidEmulatorManager.UNIT_TEST_DEVICE_SETTINGS_MAP
        self.unit_test_avd_system_image = AndroidEmulatorManager.UNIT_TEST_SYSTEM_IMAGE_PACKAGE
        self.hide_emulator_windows = hide_emulator_windows

        self.emulator_startup_timeout = emulator_startup_timeout
        self.emulator_cmd = Command("Emulator", self.android_sdk_path / 'emulator' / 'emulator.exe')

        self.avd_manager_cmd = Command("AVD Manager", self.android_sdk_path / 'tools' / 'bin' / 'avdmanager.bat')

        self.sdk_manager_cmd = Command("SDK Manager", self.android_sdk_path / 'tools' / 'bin' / 'sdkmanager.bat')

        self.adb_cmd = Command("ADB", self.android_sdk_path / 'platform-tools' / 'adb.exe')

    def collect_android_sdk_list(self):
        """
        Use the SDK Manager to get the list of installed, available, and updateable packages
        :return:  tuple of 3 lists: installed, available, and updateable packages
        """

        result_str = self.sdk_manager_cmd.run_return_output(['--list'])

        # the result will be listed out in 3 sections: Installed packages, Available Packages, and Available updates
        # and each item is represented by 3 columns separated by a '|' character
        installed_packages = []
        available_packages = []
        available_updates = []

        current_append_list = None
        for avd_item in result_str.split('\n'):
            avd_item_stripped = avd_item.strip()
            if not avd_item_stripped:
                continue
            if '|' not in avd_item_stripped:
                if avd_item_stripped.upper() == 'INSTALLED PACKAGES:':
                    current_append_list = installed_packages
                elif avd_item_stripped.upper() == 'AVAILABLE PACKAGES:':
                    current_append_list = available_packages
                elif avd_item_stripped.upper() == 'AVAILABLE UPDATES:':
                    current_append_list = available_updates
                else:
                    current_append_list = None
                continue
            item_parts = [split.strip() for split in avd_item_stripped.split('|')]
            if len(item_parts) < 3:
                continue
            elif item_parts[1].upper() in ('VERSION', 'INSTALLED', '-------'):
                continue
            elif current_append_list is None:
                continue

            if current_append_list is not None:
                current_append_list.append(item_parts)

        return installed_packages, available_packages, available_updates

    def update_installed_sdks(self):
        """
        Run an SDK Manager update to make sure the SDKs are all up-to-date
        """
        logging.info(f"Updating android SDK...")
        self.sdk_manager_cmd.run(['--update'])


    def install_system_package_if_necessary(self):
        """
        Make sure that we have the correct system image installed, and install if not
        """

        installed_packages, available_packages, _ = self.collect_android_sdk_list()

        unit_test_sdk_package_name = f'system-images;{self.unit_test_avd_system_image}'
        detected_sdk_package_version = None
        for package_line_items in installed_packages:
            if package_line_items[0] == unit_test_sdk_package_name:
                detected_sdk_package_version = package_line_items[0]

        if detected_sdk_package_version:
            # Already installed
            logging.info(f"Detected installed system image {self.unit_test_avd_system_image} version {detected_sdk_package_version}")
            return

        # Make sure its an available image to install
        detected_available_sdk_package_version = None
        for package_line_items in available_packages:
            if package_line_items[0] == unit_test_sdk_package_name:
                detected_available_sdk_package_version = package_line_items[0]
        if not detected_available_sdk_package_version:
            raise AndroidEmuError(f"Unable to install required system image {self.unit_test_avd_system_image}, not found by the Android SDK Manager")

        # Install the package
        logging.info(f"Installing system image {self.unit_test_avd_system_image}...")
        self.sdk_manager_cmd.run(['--install', unit_test_sdk_package_name])
        logging.info(f"Installed Completed")

    def find_device_id_by_name(self, device_name):
        """
        Find a device id (from AVD Manager) by the device name
        :param device_name: Name to lookup
        :return: The device id
        """
        result_str = self.avd_manager_cmd.run_return_output(['list', 'device'])
        result_lines = [result_line.strip() for result_line in result_str.split('\n')]

        result_line_count = len(result_lines)
        current_index = 0

        device_to_id_map = {}

        while current_index < result_line_count:

            current_line = result_lines[current_index]
            current_index += 1

            # This assumes the pattern "id: <id> or "<device name>"
            if current_line.startswith('id:') and 'or' in current_line:
                id_and_name_combo = current_line.split('or')
                id_and_value_combo = id_and_name_combo[0].split(' ')
                name = id_and_name_combo[1].replace('"', '').strip().upper()
                id = id_and_value_combo[1]
                device_to_id_map[name] = id

            if current_line.startswith('Available Android targets:'):
                break

        device_id = device_to_id_map.get(device_name.upper())
        if not device_id:
            raise AndroidEmuError(f"Unable to locate device id for '{device_name}'")
        return device_id

    def query_installed_avds(self):
        """
        Get maps of all valid and invalid AVDs installed on the current system
        :return: tuple of 2 maps (AVD Name -> Path): Valid and invalid
        """
        result_str = self.avd_manager_cmd.run_return_output(['list', 'avd'])
        result_lines = [result_line.strip() for result_line in result_str.split('\n')]

        line_count = len(result_lines)
        current_index = 0
        current_name = None
        current_path = None

        valid_avd_to_path_map = {}
        invalid_avd_to_path_map = {}
        current_avd_to_path_map = valid_avd_to_path_map

        while current_index < line_count:
            current_line = result_lines[current_index]
            current_index += 1

            if current_line.startswith('Name:'):
                name = current_line[6:].strip()
                if current_name is not None:
                    current_avd_to_path_map[current_name] = current_path
                current_path = None
                current_name = name

            elif current_line.startswith('Path:'):
                current_path = current_line[6:].strip()
            elif current_line.startswith('Device:'):
                pass
            elif 'could not be loaded:' in current_line:
                if current_name is not None:
                    current_avd_to_path_map[current_name] = current_path
                current_avd_to_path_map = invalid_avd_to_path_map
                current_path = None
                current_name = None

        if current_name is not None:
            current_avd_to_path_map[current_name] = current_path

        return valid_avd_to_path_map, invalid_avd_to_path_map

    def create_unitest_avd(self):
        """Create the unit test AVD"""

        self.install_system_package_if_necessary()

        device_id = self.find_device_id_by_name(self.unit_test_device_template_name)

        self.avd_manager_cmd.run(['--silent',
                                  'create', 'avd',
                                  '--name', self.unit_test_avd_name,
                                  '--package', f'system-images;{self.unit_test_avd_system_image}',
                                  '--device', device_id])

        valid_avd_map, _ = self.query_installed_avds()
        unit_test_avd_path = valid_avd_map.get(self.unit_test_avd_name)
        if not unit_test_avd_path:
            raise AndroidEmuError(f"Unable to create unit test AVD {self.unit_test_avd_name}")

        unit_test_avd_config_path = pathlib.Path(unit_test_avd_path) / 'config.ini'
        if not unit_test_avd_config_path.is_file():
            raise AndroidEmuError(f"Unable to create unit test AVD {self.unit_test_avd_name}: The expected config file '{unit_test_avd_config_path}' does not exist.")

        config_content_full = unit_test_avd_config_path.read_text(encoding='UTF-8', errors='ignore')
        for item, value in self.unit_test_device_settings_map.items():
            regex_friendly_str = item.replace('.', '\\.')
            repl_pattern = f"{regex_friendly_str}\\s*=\\s*[\\d]+"
            repl_value = f"{item}={value}"
            if re.search(repl_pattern, config_content_full):
                config_content_full = re.sub(repl_pattern, repl_value, config_content_full)
            else:
                if not config_content_full.endswith('\n'):
                    config_content_full += '\n'
                config_content_full += f"{repl_value}\n"

        unit_test_avd_config_path.write_text(config_content_full)

    def query_emulator_device_id(self):
        result_str = self.adb_cmd.run_return_output(['devices', '-l'])
        emulators = []
        for result_line in result_str.split('\n'):
            if not result_line.startswith('emulator-'):
                continue
            emulator = result_line[:result_line.find(' ')].strip()
            emulators.append(emulator)

        if len(emulators) > 1:
            logging.warning(f"Found multiple emulators connect ({','.join(emulators)}). Defaulting to {emulators[0]}")

        return emulators[0] if len(emulators) > 0 else None

    def install_unit_test_avd(self):
        """
        Install the unit test AVD (Android Virtual Device)
        """

        valid_avd_map, invalid_avd_map = self.query_installed_avds()

        if not self.unit_test_avd_name in valid_avd_map:
            create_avd = True
        elif self.force_avd_creation or self.unit_test_avd_name in invalid_avd_map:
            logging.info(f"Deleting AVD {self.unit_test_avd_name}..")
            self.avd_manager_cmd.run(['delete', 'avd', '--name', self.unit_test_avd_name])
            create_avd = True
        else:
            create_avd = False

        if create_avd:
            self.create_unitest_avd()

    def uninstall_unit_test_avd(self):
        """
        Uninstall the unit test AVD
        """
        logging.info(f"Uninstalling AVD {self.unit_test_avd_name}..")
        self.avd_manager_cmd.run(['delete', 'avd', '--name', self.unit_test_avd_name])

    def launch_emulator_process(self):
        """
        Launch the emulator process for the unit test avd and return the process handle and its device id
        :return: tuple of the process handle and the device id for the emulator
        """
        emulator_device_id = None
        process = None
        try:
            # Launch the emulator process
            emulator_process_args = [
                "-avd",
                self.unit_test_avd_name
            ]
            if self.hide_emulator_windows:
                emulator_process_args.append("-no-window")
            process = self.emulator_cmd.run_process(emulator_process_args)

            # Wait for the emulator to signal that its bootup is complete
            boot_completed = False
            start_time = time.time()
            timeout_secs = 360
            while process.poll() is None:
                elapsed_time = time.time() - start_time
                if elapsed_time > timeout_secs > 0:
                    break
                line = process.stdout.readline()
                print(line, end='')
                if "boot completed" in line:
                    boot_completed = True
                    break
            if not boot_completed:
                raise AndroidEmuError("Bootup of emulator timed out")

            # query ADB to get the emulator ID
            emulator_device_id = self.query_emulator_device_id()

            return process, emulator_device_id

        except Exception:
            if process:
                if emulator_device_id:
                    self.terminate_emulator_process(emulator_device_id)
                else:
                    process.kill()
            raise

    def terminate_emulator_process(self, device_id):
        # Terminate the emulator
        kill_emu_args = [
            '-s', device_id,
            'emu', 'kill'
        ]
        self.adb_cmd.run(kill_emu_args)

    def run_emulation_process(self, process_func):
        """
        Execute a function that relies on the session based android simulator.

        :param process_func:    The process function to execute. Function requires one argument which will be the device id
        :return: The return value of the process function
        """

        emulator_device_id = None
        try:
            emulator_process, emulator_device_id = self.launch_emulator_process()

            return process_func(emulator_device_id)

        finally:
            if emulator_device_id is not None:
                self.terminate_emulator_process(emulator_device_id)


def process_unit_test_on_simulator(base_android_sdk_path, build_path, build_config):
    """
    Run the android unit tests on a sessioned simulator

    :param base_android_sdk_path:   The path to where the Android SDK exists
    :param build_path:              The build path relative to the engine root where the android unit test project is configured and built
    :param build_config:            The configuration of the build unit test APK to run
    """

    python_cmd = Command("Python", ENGINE_ROOT / 'python' / 'python.cmd')

    android_script_root = ENGINE_ROOT / 'cmake' / 'Tools' / 'Platform' / 'Android'
    assert android_script_root.is_dir(), "Missing the android scripts path in the engine folder hierarchy"
    deploy_android_py_path = android_script_root / 'deploy_android.py'
    assert deploy_android_py_path.is_file(), "Missing the android deployment script in the engine folder hierarchy"
    launch_android_ptest_py_path = android_script_root / 'launch_android_test.py'
    assert launch_android_ptest_py_path.is_file(), "Missing the android unit test launcher script in the engine folder hierarchy"

    def _install_and_run_unit_tests(emulator_id):

        # install unit test on the emulator
        install_apk_args = [
            str(deploy_android_py_path),
            '-b', build_path,
            '-c', build_config,
            '--device-id-filter', emulator_id,
            '--clean'
        ]
        python_cmd.run(cmd_args=install_apk_args,
                       cwd=os.path.normpath(str(ENGINE_ROOT)))

        try:
            # Launch the unit test on the emulator
            launch_apk_args = [
                str(launch_android_ptest_py_path),
                '-b', build_path,
                '-c', build_config,
                '--device-serial', emulator_id
            ]
            python_cmd.run(cmd_args=launch_apk_args,
                           cwd=os.path.normpath(str(ENGINE_ROOT)))
            return True
        except AndroidEmuError:
            print("\n\n")
            raise AndroidEmuError("Unit Tests Failed")

    # Prepare the emulator manager
    manager = AndroidEmulatorManager(base_android_sdk_path=base_android_sdk_path,
                                     force_avd_creation=True)

    # Make sure that the android SDK is up to date
    manager.update_installed_sdks()

    # First Install or overwrite the unit test emulator
    manager.install_unit_test_avd()

    # Run the emulator-dependent process based on the session AVD created by the manager
    manager.run_emulation_process(_install_and_run_unit_tests)

    # Uninstall the AVD when done
    manager.uninstall_unit_test_avd()


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="Install and an android unit test APK on a android simulator.")

    parser.add_argument('--android-sdk-path',
                        help='Path to the Android SDK')
    parser.add_argument('--build-path',
                        help='The build path (relative to the engine root) where the project was generated and the APK is built',
                        required=True)
    parser.add_argument('--build-config',
                        help='The build config of the built APK',
                        required=True)
    parser.add_argument('--debug',
                        help='Enable debug messages from this script',
                        action="store_true")

    parsed_args = parser.parse_args(sys.argv[1:])
    logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.DEBUG if parsed_args.debug else logging.INFO)

    try:
        base_android_sdk_path = pathlib.Path(parsed_args.android_sdk_path) if parsed_args.android_sdk_path else get_android_sdk_path()
        process_unit_test_on_simulator(base_android_sdk_path=base_android_sdk_path,
                                       build_path=parsed_args.build_path,
                                       build_config=parsed_args.build_config)
        exit(0)
    except AndroidEmuError as e:
        print(e)
        exit(1)

