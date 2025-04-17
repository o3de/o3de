"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A class to control functionality of Lumberyard's asset processor.
The class manages a workspace's asset processor and asset configurations.
"""
import logging
import os
import psutil
import shutil
import socket
import stat
import subprocess
import tempfile
import time

from typing import List, Tuple
from enum import IntEnum

import ly_test_tools
import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
import ly_test_tools._internal.exceptions as exceptions
import ly_test_tools.o3de.pipeline_utils as utils
from ly_test_tools.o3de.ap_log_parser import APLogParser

logger = logging.getLogger(__name__)

POLLING_INTERVAL_SEC = 0.5
DEFAULT_TIMEOUT_HOURS = 8
DEFAULT_TIMEOUT_SECONDS = 900

ASSET_PROCESSOR_PLATFORM_MAP = {
    'android': 'android',
    'ios': 'ios',
    'linux': 'linux',  # Not fully implemented, see SPEC-2501
    'mac': 'mac',
    'windows': 'pc',
}

ASSET_PROCESSOR_SETTINGS_ROOT_KEY = '/Amazon/AssetProcessor/Settings'


class AssetProcessorError(Exception):
    """ Indicates that the AssetProcessor raised an error """

class StopReason(IntEnum):
    """
    Indicates what stop reason was used during the stop() function.
        NOT_RUNNING = 0
        NO_CONTROL = 1
        NO_QUIT = 2
        IO_ERROR = 3
        TIMEOUT = 4
        NO_STOP = 5
    """
    NOT_RUNNING = 0
    NO_CONTROL = 1
    NO_QUIT = 2
    IO_ERROR = 3
    TIMEOUT = 4
    NO_STOP = 5

class AssetProcessor(object):
    def __init__(self, workspace, port=45643):
        # type: (AbstractWorkspaceManager, int) -> AssetProcessor
        """
        The AssetProcessor requires a workspace to set the paths and platforms.

        :param workspace: The WorkspaceManager where the AP lives
        :param port: The port for the AP to use
        :return: None
        """
        self._workspace = workspace
        self._port = port
        self._ap_proc = None
        self._temp_asset_directory = None
        self._temp_asset_root = None
        self._project_path = self._workspace.paths.project()
        self._override_scan_folders = []
        self._test_assets_source_folder = None
        self._cache_folder = None
        self._assets_source_folder = None
        self._control_connection = None
        self._function_name = None
        self._failed_log_root = None
        self._temp_log_directory = None
        self._temp_log_root = None
        self._disable_all_platforms = False
        self._enabled_platform_overrides = dict()

    # Starts AP but does not by default run until idle.
    def start(self, connection_timeout=30, quitonidle=False, add_gem_scan_folders=None, add_config_scan_folders=None,
              accept_input=True, run_until_idle=False, scan_folder_pattern=None, connect_to_ap=False):
        self.gui_process(quitonidle=quitonidle, add_gem_scan_folders=add_gem_scan_folders,
                         add_config_scan_folders=add_config_scan_folders, timeout=connection_timeout,
                         connect_to_ap=connect_to_ap, accept_input=accept_input, run_until_idle=run_until_idle,
                         scan_folder_pattern=scan_folder_pattern)

    def wait_for_idle(self, timeout=DEFAULT_TIMEOUT_SECONDS):
        """
        Communicate with asset processor to request a response if either AP is currently idle,
        or that it becomes idle within the timeout
        """
        if not self.process_exists():
            logger.warning("Not currently managing a running Asset Processor, cannot request idle")
            return False

        self.send_message("waitforidle")
        result = self.read_message(read_timeout=timeout)
        if not self.process_exists():
            raise exceptions.LyTestToolsFrameworkException("Asset Processor has crashed or unexpectedly shut down during idle wait.")
        if not result == "idle":
            raise exceptions.LyTestToolsFrameworkException(f"Did not get idle state from AP, message was instead: {result}")
        return True

    def next_idle(self):
        """
        Communicate with asset processor to request AP respond the next time it becomes idle.  This method
        does NOT return immediately if AP is currently idle, rather it assumes there is work about to be
        done and no response is desired until it is completed.  This is useful when making source file changes
        which will be picked up by the scanner but may take a couple seconds to begin, and you only want
        to hear back after it's done.
        """
        if not self.process_exists():
            logger.warning("Not currently managing a running Asset Processor, cannot request idle")
            return

        self.send_message("signalidle")
        result = self.read_message()
        if not self.process_exists():
            raise exceptions.LyTestToolsFrameworkException("Asset Processor has crashed or unexpectedly shut down during idle request.")
        if not result == "idle":
            raise exceptions.LyTestToolsFrameworkException(f"Did not get idle state from AP, message was instead: {result}")

    def send_quit(self):
        """
        Send the quit command request to AP over the control channel
        """
        return self.send_message("quit")

    def send_windowid(self):
        """
        Send the windowid command to AP over the control channel to request the main window WId
        """
        return self.send_message("windowid")

    def send_message(self, message):
        """
        Communicate with asset processor.  Must be running through gui_process/start method
        :param message: message to send
        """
        if not self._ap_proc:
            logger.warning("Attempted to send message to AP but not currently running")
            return False

        if not self._control_connection:
            self.connect_control()

        if not self._control_connection:
            return False

        try:
            self._control_connection.sendall(message.encode())
            logger.debug(f"Sent input {message}")
            return True
        except IOError as e:
            logger.warning(f"Failed in LyTestTools to send message {message} to AP with error {e}")
        return False

    def read_message(self, read_timeout=DEFAULT_TIMEOUT_SECONDS):
        """
        Read the next message from the AP Control socket. Must be running through gui_process/start method
        """
        if not self._ap_proc:
            logger.warning("Attempted to read message to AP but not currently running")
            return "no_process"

        if not self._control_connection:
            self.connect_control()

        if not self._control_connection:
            return "no_connection"

        self._control_connection.settimeout(read_timeout)
        try:
            result = self._control_connection.recv(4096)
            result_message = result.decode()
            logger.debug(f"Got result message {result_message}")
            return result_message
        except IOError as e:
            logger.warning(f"Failed in LyTestTools to read message from with error {e}")
            return f"error_{e}"

    def read_control_port(self):
        return self.read_port_from_log("Control Port")

    def read_listening_port(self):
        return self.read_port_from_log("Listening Port")

    def read_port_from_log(self, port_type):
        """
        Read the a port chosen by AP from the log
        """
        port = None

        def _get_port_from_log():
            nonlocal port
            if not os.path.exists(self._workspace.paths.ap_gui_log()):
                return False

            log = APLogParser(self._workspace.paths.ap_gui_log())
            if len(log.runs):
                try:
                    port = log.runs[-1][port_type]
                    logger.debug(f"Read port type {port_type} : {port}")
                    return True
                except Exception as ex:  # intentionally broad
                    logger.debug(f"Failed in LyTestTools to read port type {port_type} : {port} from file", exc_info=ex)
            return False

        # the timeout needs to be large enough to load all the dynamic libraries the AP-GUI loads since the control port
        # is opened after all the DLL loads, this can take a long time in a Debug build
        ap_max_activate_time = 60
        err = AssetProcessorError(f"Failed in LyTestTools to read port type {port_type} from {self._workspace.paths.ap_gui_log()}. "
                                  f"Waited for {ap_max_activate_time} seconds.")
        waiter.wait_for(_get_port_from_log, timeout=ap_max_activate_time, exc=err)
        return port

    def set_control_connection(self, connection):
        self._control_connection = connection

    def connect_control(self):
        """
        Set up the control socket connection with asset processor.  The control connection is to allow for basic
        two way communication with AP for things such as querying state and making basic requests such as shutdown.
        """
        if not self._control_connection:
            control_timeout = 60
            return self.connect_socket("Control Connection", self.read_control_port,
                                       set_port_method=self.set_control_connection, timeout=control_timeout)
        return True, None

    def using_temp_workspace(self):
        return self._temp_asset_root is not None

    def connect_socket(self, port_name, read_port_method, host='127.0.0.1', set_port_method=None,
                       timeout=10):
        """
        :param port_name: Str defining the name of the port for logging
        :param read_port_method: Method to retrieve the port number to use
        :param host: Host ip to connect to
        :param set_port_method: If set, method to call with the established connection
        :param timeout: Max seconds to attempt connection for
        """
        connect_port = read_port_method()
        logger.debug(f"Attempting to for connect to AP {port_name}: {host}:{connect_port} for {timeout} seconds")

        def _attempt_connection():
            nonlocal connect_port
            if self._ap_proc.poll() is not None:
                raise AssetProcessorError(f"Asset processor exited early with errorcode: {self._ap_proc.returncode}")

            connection_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            connection_socket.settimeout(timeout)
            try:
                connection_socket.connect((host, connect_port))
                logger.debug(f"Connection to AP {port_name} was successful")
                if set_port_method is not None:
                    set_port_method(connection_socket)
                return True
            except Exception as ex:  # Purposefully broad
                logger.debug(f"Failed to connect to {host}:{connect_port}", exc_info=ex)
                if not connect_port or not self.using_temp_workspace():
                    # If we're not using a temp workspace with a fresh log it's possible we're reading a port from
                    # a previous run and the log just hasn't written yet, we need to keep checking the log for a new
                    # port to use
                    try:
                        new_connect_port = read_port_method()
                        if new_connect_port != connect_port:
                            logger.debug(f"Found new connect port for {port_name}: {host}:{new_connect_port}")
                            connect_port = new_connect_port
                    except Exception as read_exception:  # Purposefully broad
                        logger.debug(f"Failed in LyTestTools to read port data for {port_name}: {host}:{new_connect_port}",
                                       exc_info=read_exception)
            return False

        err = AssetProcessorError(f"Could not connect to AP {port_name} on {host}:{connect_port}. Waited for {timeout}.")
        waiter.wait_for(_attempt_connection, timeout=timeout, exc=err)
        return True, None

    def stop(self, timeout=60):
        """
         Stops the AssetProcessor. First attempts a clean shutdown over the control channel
         if open.  Calls terminate if no control connection is open
         :param timeout: How long to wait, in seconds, for AP to shut down after receiving quit message.
            This timeout is a longer default because AP doesn't quit immediately on receiving the quit message,
            it waits until finishing its current task, which can sometimes take a while.
         :return: Returns the StopReason as an Int
         """
        if not self._ap_proc:
            logger.warning("Attempting to quit AP but none running")
            return StopReason.NOT_RUNNING

        if not self._control_connection:
            logger.debug("No control connection open, using terminate")
            self.terminate()
            return StopReason.NO_CONTROL

        try:
            if not self.send_quit():
                logger.warning("Failed to send quit command, using terminate")
                self.terminate()
                return StopReason.NO_QUIT
        except IOError as e:
            logger.warning(f"Failed in LyTestTools to send quit request with error {e}, stopping")
            self.terminate()
            return StopReason.IO_ERROR

        wait_timeout = timeout
        try:
            waiter.wait_for(lambda: not self.process_exists(), exc=AssetProcessorError, timeout=wait_timeout)
        except AssetProcessorError:
            logger.warning(f"Timeout in LyTestTools attempting to quit asset processor after {wait_timeout} seconds, using terminate")
            self.terminate()
            return StopReason.TIMEOUT

        if self.process_exists():
            logger.warning(f"Failed in LyTestTools to stop process {self.get_pid()} after {wait_timeout} seconds, using terminate")
            self.terminate()
            return StopReason.NO_STOP

        self._ap_proc = None

    def terminate(self):
        """
        Forcibly stops AP and child processes
        :return: None
        """

        process_list = self.get_process_list()
        # An Asset Processor process can be running but if _ap_proc is None, it means we don't own it.
        # If we don't own it, we won't kill it
        if len(process_list) == 0:
            logger.debug("Attempted to stop asset processor, but it's already closed.")
            return
        process_list.reverse()  # Kill child processes first

        if self._control_connection:
            self._control_connection.close()
            self._control_connection = None

        def term_success(proc):
            logger.debug(f"Successfully terminated {proc} with code {proc.returncode}")

        for this_process in process_list:
            logger.debug(f"Terminating: {this_process.name()} pid: {this_process.pid}")
            try:
                this_process.terminate()
            except psutil.NoSuchProcess:
                # Already terminated
                pass
        _, remaining = psutil.wait_procs(process_list, timeout=10, callback=term_success)
        for process in remaining:
            logger.debug(f"Killing: {process.name()} pid: {process.pid}")
            process.kill()
        logger.debug("Finished terminating asset processor")
        self._ap_proc = None

    def get_pid(self):
        """
        Returns pid of asset processor proc currently executing the start command (Ap Gui)

        :return: pid if exists, else -1
        """
        return self._ap_proc.pid if self._ap_proc else -1

    def get_process_list(self, name_filter: str = None):
        """
        Returns a list of all processes currently executing under an AP start run, such as AP Gui and Builders

        :param name_filter: Name to match against, such as AssetBuilder
        :return: List of processes
        """
        my_pid = self.get_pid()
        if my_pid == -1:
            return []
        return_list = []
        try:
            return_list = [psutil.Process(my_pid)]
            return_list.extend(utils.child_process_list(my_pid))
        except psutil.NoSuchProcess:
            logger.warning("Process already finished calling get_process_list")
        return return_list

    def process_cpu_usage_below(self, cpu_usage_threshold: float) -> bool:
        """
        Checks whether CPU usage by AP and any child processes falls below a threshold

        :param cpu_usage_threshold: Float at or above which CPU usage by a process instance is too high
        :return: True if the CPU usage for each instance of the specified process is below the threshold, False if not
        """
        # Get all instances of targeted process
        targeted_processes = self.get_process_list()

        # Return whether all instances of targeted process are idle
        for targeted_process in targeted_processes:
            process_cpu_load = targeted_process.cpu_percent(interval=1)
            if process_cpu_load >= cpu_usage_threshold:
                logger.info(f"Process name: {targeted_process.name()}")
                if hasattr(targeted_process, "pid"):
                    logger.info(f"Process ID: {targeted_process.pid}")
                logger.info(f"Process CPU load: {process_cpu_load}")
                return False
        return True

    def backup_ap_settings(self):
        # type: () -> None
        """
        Backs up the asset processor settings file via the LySettings class

        :return: None
        """
        backup_path = self._workspace.settings.get_temp_path()
        logger.debug(f"Performing backup of asset processor in path {backup_path}")
        self._workspace.settings.backup_asset_processor_settings(backup_path)

    def restore_ap_settings(self):
        # type: () -> None
        """
        Restores the asset processor settings file via the LySettings class

        :return: None
        """
        backup_path = self._workspace.settings.get_temp_path()
        logger.debug(f"Restoring backup of asset processor in path {backup_path}")
        self._workspace.settings.restore_asset_processor_settings(backup_path)

    def teardown(self):
        # type: () -> None
        """
        Restores backup of asset processor settings and stops the asset processor

        :return: None
        """
        self.stop()
        self.restore_ap_settings()

    def process_exists(self):
        if self._ap_proc:
            return self._ap_proc.poll() is None
        return False

    def batch_process(self, timeout=DEFAULT_TIMEOUT_SECONDS, fastscan=True, capture_output=False, platforms=None,
                      extra_params=None, add_gem_scan_folders=None, add_config_scan_folders=None, decode=True,
                      expect_failure=False, scan_folder_pattern=None, create_temp_log=True, debug_output=False,
                      skip_atom_output=False):
        if create_temp_log:
            self.create_temp_log_root()
        ap_path = self._workspace.paths.asset_processor_batch()
        command = self.build_ap_command(ap_path=ap_path, fastscan=fastscan, platforms=platforms,
                                        extra_params=extra_params, add_gem_scan_folders=add_gem_scan_folders,
                                        add_config_scan_folders=add_config_scan_folders,
                                        scan_folder_pattern=scan_folder_pattern, debug_output=debug_output,
                                        skip_atom_output=skip_atom_output)
        return self.run_ap_process_command(command, timeout=timeout, capture_output=capture_output, decode=decode,
                                           expect_failure=expect_failure)

    # Start AP gui and run until idle.  Useful for asset processing tests where the assets should be seen and processed
    # by AP on startup and in the test you do not wish to continue execution until you know AP has gone idle.
    def gui_process(self, timeout=DEFAULT_TIMEOUT_SECONDS, fastscan=True, capture_output=False, platforms=None,
                    extra_params=None, add_gem_scan_folders=None, add_config_scan_folders=None, decode=True,
                    expect_failure=False, quitonidle=False, connect_to_ap=False, accept_input=True, run_until_idle=True,
                    scan_folder_pattern=None, create_temp_log=True, debug_output=False, skip_atom_output=False):
        ap_path = os.path.abspath(self._workspace.paths.asset_processor())
        ap_exe_path = os.path.dirname(ap_path)
        extra_gui_params = []
        if quitonidle:
            extra_gui_params.append("--quitonidle")
        if accept_input:
            extra_gui_params.append("--acceptInput")

        logger.debug("Starting asset processor")
        if self.process_exists():
            logger.error("Asset processor already started.  Stop first")
            return False, None

        if not platforms:
            if hasattr(self._workspace, "asset_processor_platform"):
                ap_target = self._workspace.asset_processor_platform
            else:
                ap_target = ly_test_tools.HOST_OS_PLATFORM
            ap_platform = ASSET_PROCESSOR_PLATFORM_MAP.get(ap_target, ly_test_tools.HOST_OS_PLATFORM)
            logger.debug(f"Setting AP platform to: {ap_platform}")
            extra_gui_params.append('--platforms')
            extra_gui_params.append(ap_platform)

        if extra_params:
            if isinstance(extra_params, list):
                extra_gui_params.extend(extra_params)
            else:
                extra_gui_params.append(extra_params)

        if create_temp_log:
            self.create_temp_log_root()
        command = self.build_ap_command(ap_path=ap_path, fastscan=fastscan, platforms=platforms,
                                        extra_params=extra_gui_params, add_gem_scan_folders=add_gem_scan_folders,
                                        add_config_scan_folders=add_config_scan_folders,
                                        scan_folder_pattern=scan_folder_pattern, debug_output=debug_output,
                                        skip_atom_output=skip_atom_output)

        # If the AP is quitting on idle just run it like AP batch.
        if quitonidle:
            return self.run_ap_process_command(command, timeout=timeout, capture_output=capture_output, decode=decode,
                                               expect_failure=expect_failure)

        logger.debug(f"Launching AP at path: {ap_path}")

        if capture_output:
            logger.warning(f"Cannot capture output when leaving AP connection open.")

        logger.info(f"Launching AP with command: {command}")
        try:
            self._ap_proc = subprocess.Popen(command, cwd=ap_exe_path, env=process_utils.get_display_env())
            time.sleep(1)
            if self._ap_proc.poll() is not None:
                raise AssetProcessorError(f"AssetProcessor immediately quit with errorcode {self._ap_proc.returncode} in LyTestTools ")

            if accept_input:
                self.connect_control()

            if connect_to_ap:
                self.connect_listen()

            if quitonidle:
                waiter.wait_for(lambda: not self.process_exists(), timeout=timeout,
                                exc=AssetProcessorError(f"Failed in LyTestTools to quit on idle within {timeout} seconds"))
            elif run_until_idle and accept_input:
                if not self.wait_for_idle():
                    return False, None
            return True, None
        except BaseException as be:  # purposefully broad
            logger.exception("Exception while starting Asset Processor")
            # clean up to avoid leaking open AP process to future tests
            try:
                if self._ap_proc:
                    self._ap_proc.kill()
            except Exception as ex:
                logger.exception("Ignoring exception while trying to terminate Asset Processor from LyTestTools")
            raise exceptions.LyTestToolsFrameworkException from be  # raise whatever prompted us to clean up

    def connect_listen(self, timeout=DEFAULT_TIMEOUT_SECONDS):
        # Wait for the AP we launched to be ready to accept a connection
        return self.connect_socket("Listen Connection", self.read_listening_port, timeout=timeout)

    def build_ap_command(self, ap_path, fastscan=True, platforms=None,
                         extra_params=None, add_gem_scan_folders=None, add_config_scan_folders=None,
                         scan_folder_pattern=None, debug_output=False, skip_atom_output=False):
        """
        Launch asset processor batch and wait for it to complete or until the timeout expires.
        Returns true on success and False if an error is reported.

        :param fastscan: Enable "zero analysis mode"
        :param platforms: Different set of platforms to run against
        :param add_gem_scan_folders: Should gem scan folders be added to the processing - by default this is off
            if scan folder overrides are set, on if not
        :param add_config_scan_folders: Should config scan folders be added to the processing - by default this is off
            if scan folder overrides are set, on if not
        :param debug_output: Enables builder debug output of dbgsg files
        :param skip_atom_output: Disables builder output of atom product assets, such as azmeshes, azlods, materials, ect.
        :return: Command list ready to pass to subprocess
        """
        logger.debug(f"Starting {ap_path}")
        command = [ap_path]
        if fastscan:
            command.append("--zeroAnalysisMode")

        if self._project_path:
            command.append(f'--regset="/Amazon/AzCore/Bootstrap/project_path={self._project_path}"')

        # When using a scratch workspace we will set several options.  The asset root controls where our
        # cache lives.
        # The logDir gives us a unique directory to place our logs in, and indicates we wish to randomize our
        # listening port to avoid collisions.  The port will be written to the logs which we'll retrieve it from
        if self._temp_asset_root:
            command.append("--assetroot")
            command.append(f"{self._temp_asset_root}")
            command.append(f'--regset="/Amazon/AzCore/Bootstrap/project_cache_path={self.temp_project_cache_path()}"')
            command.append("--randomListeningPort")
        if self._temp_log_root:
            command.append("--logDir")
            command.append(f"{self._temp_log_root}")
        if self._override_scan_folders:
            command.append("--scanfolders")
            command.append(f"{','.join(self._override_scan_folders)}")
        if add_gem_scan_folders is None:
            add_gem_scan_folders = not self._override_scan_folders
        if add_config_scan_folders is None:
            add_config_scan_folders = not self._override_scan_folders
        if not add_gem_scan_folders:
            command.append("--noGemScanFolders")
        if not add_config_scan_folders:
            command.append("--noConfigScanFolders")
        if scan_folder_pattern:
            command.append("--scanfolderpattern")
            if not isinstance(scan_folder_pattern, list):
                scan_folder_pattern = [scan_folder_pattern]
            command.append(f"{','.join(scan_folder_pattern)}")

        if self._disable_all_platforms:
            command.append(f'--regremove="f{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/Platforms"')
        if platforms:
            if isinstance(platforms, list):
                platforms = ','.join(platforms)
            command.append(f'--platforms={platforms}')
        for key, value in self._enabled_platform_overrides.items():
            command.append(f'--regset="f{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/Platforms/{key}={value}"')

        if debug_output:
            command.append("--debugOutput")

        if skip_atom_output:
            command.append("--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\"")

        if extra_params:
            if isinstance(extra_params, list):
                command.extend(extra_params)
            else:
                command.append(extra_params)
        return command

    def run_ap_process_command(self, command, timeout=DEFAULT_TIMEOUT_SECONDS, decode=True, capture_output=False,
                               expect_failure=False):
        """
        In case of a timeout, the asset processor and associated processes are killed and the function returns False.

        :param timeout: seconds to wait before aborting
        :param capture_output = Capture output which will be returned in the second of the return pair
        :param decode: decode byte strings from captured output to utf-8
        :param expect_failure: asset processing is expected to fail, so don't error on a failure, and assert on no failure.
        """
        logger.info(f"Launching AP with command: {command}")
        start = time.time()
        if type(timeout) not in [int, float] or timeout < 1:
            logger.warning(f"Invalid timeout {timeout} - defaulting to {DEFAULT_TIMEOUT_SECONDS} seconds")
            timeout = DEFAULT_TIMEOUT_SECONDS

        run_result = subprocess.run(command, close_fds=True, timeout=timeout, capture_output=capture_output)
        output_list = None
        if capture_output:
            if decode:
                output_list = run_result.stdout.decode('utf-8', errors="replace").splitlines()
            else:
                output_list = run_result.stdout.splitlines()

        if run_result.returncode != 0:
            errorMessage = f"{command} returned error code: {run_result.returncode}"
            if expect_failure:
                logger.debug(f"Expected error occurred. {errorMessage}")
            else:
                logger.error(errorMessage)
                self.check_copy_logs()
            return False, output_list
        elif expect_failure:
            logger.error(f"{command} was expected to fail, but instead ran without failure.")
            return True, output_list
        logger.debug(f"{command} completed successfully in {time.time() - start} seconds")
        return True, output_list

    def set_failure_log_folder(self, log_root):
        self._failed_log_root = log_root

    def check_copy_logs(self):
        if self._temp_asset_root and self._failed_log_root:
            source_path = os.path.join(self._temp_asset_root, "logs")
            failed_log_folder = f"{self._function_name}." if self._function_name else ''
            dest_path = os.path.join(self._failed_log_root, f"{failed_log_folder}{int(round(time.time() * 1000))}")
            logger.debug(f"Copying {source_path} to {dest_path}")
            shutil.copytree(source_path, dest_path)

    def disable_all_asset_processor_platforms(self):
        """
        Disables all platforms via the SettingsRegistry --regremove option
        
        :return: None
        """
        self._disable_all_platforms = True

    def enable_asset_processor_platform(self, platform):
        # type: (str) -> None
        """
        Add the platform to the dict of platforms to enable via the SettingsRegistry --regset option
        
        :param platform: The platform to enable
        :return: None
        """
        self._enabled_platform_overrides[platform] = 'enabled'

    def temp_asset_root(self):
        """
        Returns the path to the "Temp asset root" which acts as a temporary folder approximating the engine's
        /dev folder.

        :return: Absolute path of current temp asset root
        """
        return self._temp_asset_root

    def _copy_asset_root_files(self):
        """
        Copies the minimum required files for AP Batch to run from our current workspace dev environment to our
        "temp asset root" workspace

        :return: None
        """
        for copy_dir in [self._workspace.project, 'Assets/Engine', 'Registry']:
            make_dir = os.path.join(self._temp_asset_root, copy_dir)
            if not os.path.isdir(make_dir):
                os.makedirs(make_dir, exist_ok=True)
        for copyfile_name in ['Registry/AssetProcessorPlatformConfig.setreg',
                              os.path.join(self._workspace.project, "project.json"),
                              os.path.join('Assets', 'Engine', 'exclude.filetag')]:
            shutil.copyfile(os.path.join(self._workspace.paths.engine_root(), copyfile_name),
                            os.path.join(self._temp_asset_root, copyfile_name))

    def delete_temp_asset_root(self):
        """
        Cleanup our current temporary asset directory

        :return: None
        """
        if self._temp_asset_directory:
            self._temp_asset_directory = None

    def _del_readonly(self, action, name, exc):
        os.chmod(name, stat.S_IWRITE)
        if not os.access(name, os.W_OK):
            os.chmod(name, stat.S_IWUSR)
        try:
            os.remove(name)
        except OSError as e:
            logger.error(f'In LyTestTools Failed to clean up {name} : {e}')

    def delete_temp_asset_root_folder(self):
        """
        Clean the contents of our temporary asset root folder

        :return: None
        """
        if self._temp_asset_root:
            logger.debug(f'Cleaning up old asset root at {self._temp_asset_root}')
            shutil.rmtree(self._temp_asset_root, onerror=self._del_readonly)

    def create_temp_asset_root(self, project_scan_folder=True):
        """
        Create a new temporary asset root folder.  Cleans up the previous one if set.
        Copies the minimum necessary files to the workspace for APBatch to run

        :param project_scan_folder: Set the current active project's folder as one of our scan folders
        :return: None
        """
        if self._temp_asset_root:
            logger.debug(f'Cleaning up old asset root at {self._temp_asset_root}')
            shutil.rmtree(self._temp_asset_root, True)
        self._temp_asset_directory = tempfile.TemporaryDirectory()
        self._temp_asset_root = self._temp_asset_directory.name
        self._copy_asset_root_files()
        self._project_path = os.path.join(self._temp_asset_root, self._workspace.project)
        if project_scan_folder:
            self.add_scan_folder(self._project_path)

    def log_root(self):
        """
        Return the temp log root
        """
        return self._temp_log_root

    def create_temp_log_root(self):
        """
        Create a temp folder for logs.  We want a unique temp folder for logs for each test using the AssetProcessor
        test class rather than using the default logs folder which could contain any old data or be used by other
        runs of asset processor.
        """
        if self._temp_log_directory:
            logger.debug(f'Cleaning up old log root at {self._temp_log_root}')
            # The finalizer will clean up the old temporary directory when the TemporaryDirectory() reference count
            # hits 0
            self._temp_log_directory = None
        self._temp_log_directory = tempfile.TemporaryDirectory()
        self._temp_log_root = self._temp_log_directory.name
        self._workspace.paths.set_ap_log_root(self._temp_log_root)

    def add_scan_folder(self, folder_name) -> str:
        """
        Add a new folder as one of the default folders which AP Batch should scan on the next run

        :param folder_name: Path to the folder to use.  Absolute folders will be accepted as is, relative folders
            assume relative from the current temp asset root if set or otherwise the path to the workspace's dev folder
        :return: Absolute path of added scan folder
        """
        if os.path.isabs(folder_name):
            if folder_name not in self._override_scan_folders:
                self._override_scan_folders.append(folder_name)
                logger.debug(f'Adding override scan folder {folder_name}')
            return folder_name
        else:
            if not self._temp_asset_root:
                logger.warning(f"Can not create scan folder, no temporary asset workspace has been created")
                return ""
            scan_folder = os.path.join(self._temp_asset_root if self._temp_asset_root else self._workspace.paths.engine_root(),
                                       folder_name)
            if not os.path.isdir(scan_folder):
                os.makedirs(scan_folder, exist_ok=True)
            if folder_name not in self._override_scan_folders:
                self._override_scan_folders.append(scan_folder)
                logger.debug(f"Adding scan folder {scan_folder}")
            return scan_folder

    def clear_scan_folders(self) -> None:
        """
        Remove all override scan folders

        :return: None
        """
        self._override_scan_folders = None

    def create_temp_asset(self, file_path, content):
        """
        Helper to create a simple test asset under the asset root.

        :param file_path: Relative path under the temp asset root to the file to create
        :param content: Content to write into the file
        :return: None
        """
        write_path = os.path.join(self._temp_asset_root, file_path)
        f = open(write_path, 'w')
        f.write(content)
        f.close()
        logger.debug(f'Wrote to asset at path: {write_path}')

    def prepare_test_environment(self, assets_path: str, function_name: str, use_current_root=False,
                                 relative_asset_root=None, add_scan_folder=True, cache_platform=None,
                                 existing_function_name=None) -> Tuple[str, str]:
        """
        Creates a temporary test workspace, copies the specified test assets and sets the folder as a scan
        folder for processing.

        :param assets_path: Path to tests assets folder
        :param function_name: Name of a function that corresponds to folder with assets
        :param use_current_root: Expect an already created asset root we wish to re-use
        :param relative_asset_root: If the function folder should live somewhere else relative to the temp_asset_root
            than the Project folder.  Use '' for temp_asset_root
        :param add_scan_folder: Should the new assets be automatically added as a scan folder
        :param cache_platform: Cache platform to use in returned cache path, or assumes workspace asset processor
            platform
        :return: Returning path to copied assets folder
        """
        if not use_current_root:
            self.create_temp_asset_root()
        test_asset_root = os.path.join(self._temp_asset_root, self._workspace.project if relative_asset_root is None
                                       else relative_asset_root)
        test_folder = os.path.join(test_asset_root, function_name if existing_function_name is None
                                   else existing_function_name)
        if not os.path.isdir(test_folder):
            os.makedirs(test_folder, exist_ok=True)
        if add_scan_folder:
            self.add_scan_folder(test_asset_root)
        self._function_name = function_name
        self._assets_source_folder = utils.prepare_test_assets(assets_path, function_name, test_folder)
        self._test_assets_source_folder = test_folder
        self._cache_folder = os.path.join(self._temp_asset_root, self._workspace.project,'Cache', cache_platform or
                                          ASSET_PROCESSOR_PLATFORM_MAP[self._workspace.asset_processor_platform],
                                          function_name.lower() if existing_function_name is None
                                          else existing_function_name)

        return self._test_assets_source_folder, self._cache_folder

    def project_test_cache_folder(self):
        """
        Get the project cache folder from a test run using prepare_test_environment

        :return: path to folder
        """
        return self._cache_folder

    def assets_source_folder(self):
        """
        Get the original project source folder from a test run using prepare_test_environment
        which the assets under test were copied FROM into the test environment

        :return: path to folder
        """
        return self._assets_source_folder

    def project_test_source_folder(self):
        """
        Get the project source folder from a test run using prepare_test_environment

        :return: path to folder
        """
        return self._test_assets_source_folder

    def compare_assets_with_cache(self) -> Tuple[List[str], List[str]]:
        """
        Helper to compare output assets from a test run using prepare_test_environment

        :return: Results of the comparison as a pair (Missing Assets, Found matching assets) - see
        pipeline_utils.compare_assets_with_cache for more info
        """
        return utils.compare_assets_with_cache(os.listdir(self._test_assets_source_folder), self._cache_folder)

    def clear_project_test_assets_dir(self):
        """
        Helper to cleanup the test_assets_source_folder created for a test using prepare_test_environment

        :return: None
        """
        utils.clear_project_test_assets_dir(self._test_assets_source_folder)

    def delete_temp_cache(self):
        """
        Helper to clean up the current cache folder from a test using prepare_test_environment

        :return: None
        """
        shutil.rmtree(self._cache_folder, onerror=self._del_readonly)

    def run_and_check_output(self, error_expected, error_search_terms, platforms=None, decode=True):
        """
        Helper to call batch_process which respects temporary workspaces.

        :param error_expected: Is the error_search terms list a list of errors which should or should not be found
        :param error_search_terms: Errors to be found if error_expected is true, errors not to be found if false
        :return: Output list
        """
        _, output = self.batch_process(capture_output=True, platforms=platforms, decode=decode,
                                       expect_failure=error_expected)
        # Check for error in output
        if error_expected:
            utils.validate_log_output(output, error_search_terms, [])
        else:
            utils.validate_log_output(output, [], error_search_terms)

    def add_relative_source_asset(self, relative_asset, source_root=None, dest_root=None):
        """
        Helper to add a source asset from our workspace to our temporary asset workspace

        :param relative_asset: Relative path to the asset under dev which should be added
        :param source_root: Path to the folder which relative_asset is relative to.  workspace's dev folder if not
            supplied
        :param dest_root: Path to the folder which relative asset should be output relative to.  Current temp
            asset root if not supplied
        :return: None
        """
        source_root = source_root or self._workspace.paths.engine_root()
        if not self._temp_asset_root:
            logger.warning("Can't add relative source asset, no temporary asset root created.")
            return
        dest_root = dest_root or self._temp_asset_root
        make_dir = os.path.dirname(os.path.join(dest_root, relative_asset))
        if not os.path.isdir(make_dir):
            os.makedirs(make_dir, exist_ok=True)
        shutil.copyfile(os.path.join(source_root, relative_asset),
                        os.path.join(dest_root, relative_asset))

    def temp_project_cache(self, project_name=None, asset_platform=None):
        """
        Getter to return the absolute path to the temp workspace's project cache

        :param project_name: Project name to use, defaults to workspace's project name
        :param asset_platform: Asset platform to use.  Workspace's asset platform if not supplied
        :return: path to project cache
        """
        asset_platform = asset_platform or ASSET_PROCESSOR_PLATFORM_MAP[self._workspace.asset_processor_platform]
        return os.path.join(self._temp_asset_root, self._workspace.project, 'Cache', asset_platform.lower())

    def temp_project_cache_path(self, project_name=None):
        """
        Get the project cache root folder from a test run using prepare_test_environment
        The project cache root folder does not include the asset platform

        :param project_name: Project name to use, defaults to workspace's project name

        :return: path to project cache root folder
        """
        return os.path.join(self._temp_asset_root, self._workspace.project, 'Cache')

    def clear_readonly(self, relative_dest):
        """
        Clear the readonly flag on a relative file or all files within a directory underneath the temporary
        asset root (Only works within temporary directory)

        :param relative_dest: Relative path to file or folder within the temp workspace which should be cleared
            of readonly flag
        :return: None
        """
        if not self._temp_asset_root:
            logger.warning("clear_readonly called without temp_asset_root set")
            return
        clear_path = os.path.join(self._temp_asset_root, relative_dest)
        if os.path.isfile(clear_path):
            logger.debug(f"Clearing readonly flag for {clear_path}")
            os.chmod(clear_path, stat.S_IWRITE)
        elif os.path.isdir(clear_path):
            logger.debug(f"Clearing readonly flag for files in {clear_path}")
            file_system.change_permissions(clear_path, stat.S_IWRITE)
        else:
            logger.warning(f"clear_readonly called with invalid path {clear_path}")

    def add_source_folder_assets(self, relative_source, relative_dest=None):
        """
        Add an entire tree of source assets to our temporary workspace

        :param relative_source: path to relative folder under workspace's dev folder to start at
        :param relative_dest: Destination within the temporary workspace to copy to.  relative_source if not
            supplied
        :return: path to project cache
        """
        source_folder = os.path.join(self._workspace.paths.engine_root(), relative_source)
        dest_relative = relative_dest or relative_source
        dest_folder = os.path.join(self._temp_asset_root, dest_relative)
        shutil.copytree(source_folder, dest_folder)
        self.clear_readonly(dest_relative)

    @staticmethod
    def copy_assets_to_project(assets: List[str], source_directory: str, target_asset_dir: str) -> None:
        """
        Simple wrapper for pipeline_utils' copy_assets_to_project to remove the need to import pipeline_utils

        :return: None
        """
        utils.copy_assets_to_project(assets, source_directory, target_asset_dir)


def _build_ap_batch_call_params(ap_path, project, platforms, extra_params=None):
    project_str = f"--regset=/Amazon/AzCore/Bootstrap/project_path={project}"
    project_str.rstrip(' ')
    param_list = [ap_path, project_str]

    if platforms:
        if isinstance(platforms, list):
            platforms = ','.join(platforms)
        param_list.append(f'--platforms={platforms}')

    if extra_params:
        if isinstance(extra_params, list):
            param_list.extend(extra_params)
        else:
            param_list.append(extra_params)
    return param_list


def assetprocessorbatch_check_output(workspace, project=None, platforms=None, extra_params=None, log_info=False,
                                     ap_override_path=None, no_split=False, expect_failure=False):
    ap_path = ap_override_path or workspace.paths.asset_processor_batch()
    project = project or workspace.project
    platforms = platforms or ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]
    param_list = _build_ap_batch_call_params(ap_path, project, platforms, extra_params)
    try:
        output_list = subprocess.check_output(param_list).decode('utf-8')
        if not no_split:
            output_list = output_list.splitlines()
        if log_info:
            logger.info(f'AssetProcessorBatch output:\n{output_list}')
        return output_list
    except subprocess.CalledProcessError as e:
        if not expect_failure:
            logger.error(f"AssetProcessorBatch returned error {ap_path} to LyTestTools with error {e}")
        # This will sometimes be due to expected asset processing errors - we'll return the output and let the tests
        # decide what to do
        if not no_split:
            return e.output.decode('utf-8').splitlines()
        return e.output.decode('utf-8')
    except FileNotFoundError as e:
        logger.error(f"File Not Found - Failed to call {ap_path} from LyTestTools with error {e}")
    if expect_failure:
        raise exceptions.LyTestToolsFrameworkException("AP failure didn't occur as expected")


def parse_output_value(output_list, start_value, end_value=None):
    for output in output_list:
        found_pos = output.find(start_value)
        if found_pos >= 0:
            start_pos = found_pos + len(start_value)
            if not end_value:
                return output[start_pos:]
            end_pos = output.find(end_value, start_pos)
            if end_pos == -1:
                continue
            return output[start_pos:end_pos]
    return None


def get_num_processed_assets(output):
    result = parse_output_value(output, 'Number of Assets Successfully Processed:', '.')
    logger.info(f'Assets Successfully Processed returned {result}')
    return -1 if not result else int(result)


def get_num_failed_processed_assets(output):
    result = parse_output_value(output, 'Number of Assets Failed to Process:', '.')
    logger.info(f'Failed assets returned {result}')
    return -1 if not result else int(result)


def has_invalid_server_address(output, serverAddress):
    address_invalid_warning = f"({serverAddress}) is invalid"
    for line in output:
        if address_invalid_warning in line:
            return True
    return False
