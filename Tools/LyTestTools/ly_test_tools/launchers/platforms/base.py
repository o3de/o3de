"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Basic interface to interact with lumberyard launcher
"""
import logging
import os
from configparser import ConfigParser

import six

import ly_test_tools.launchers.exceptions
import ly_test_tools.environment.process_utils
import ly_test_tools.environment.waiter

log = logging.getLogger(__name__)


class Launcher(object):
    def __init__(self, workspace, args):
        # type: (ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager, List[str]) -> None
        """
        Constructor for a generic launcher, requires a reference to the containing workspace and a list of arguments
        to pass to the game during launch.

        :param workspace: Workspace containing the launcher
        :param args: list of arguments passed to the game during launch
        """
        log.debug(f"Initializing launcher for workspace '{workspace}' with args '{args}'")
        self.workspace = workspace  # type: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager

        if args:
            if isinstance(args, list):
                self.args = args
            else:
                raise TypeError(f"Launcher args must be provided as a list, received: '{type(args)}'")
        else:
            self.args = []

    def _config_ini_to_dict(self, config_file):
        """
        Converts an .ini config file to a dict of dicts, then returns it.

        :param config_file: string representing the file path to the .ini file.
        :return: dict of dicts containing the section & keys from the .ini file,
            otherwise raises a SetupError.
        """
        config_dict = {}
        user_profile_directory = os.path.expanduser('~').replace(os.sep, '/')
        if not os.path.exists(config_file):
            raise ly_test_tools.launchers.exceptions.SetupError(
                f'Default file path not found: "{user_profile_directory}/ly_test_tools/devices.ini", '
                f'got path: "{config_file}" instead. '
                f'Please create the following file: "{user_profile_directory}/ly_test_tools/devices.ini" manually. '
                f'Add device IP/ID info inside each section as well.\n'
                'See ~/engine_root/dev/Tools/LyTestTools/README.txt for more info.')

        config = ConfigParser()
        config.read(config_file)

        for section in config.sections():
            config_dict[section] = dict(config.items(section))

        return config_dict

    def setup(self, backupFiles=True, launch_ap=True, configure_settings=True):
        """
        Perform setup of this launcher, must be called before launching.
        Subclasses should call its parent's setup() before calling its own code, unless it changes configuration files

        For testing mobile or console devices, make sure you populate the config file located at:
            ~/ly_test_tools/devices.ini (a.k.a. %USERPROFILE%/ly_test_tools/devices.ini)

        :param backupFiles: Bool to backup setup files
        :return: None
        """
        # Remove existing logs and dmp files before launching for self.save_project_log_files()
        if os.path.exists(self.workspace.paths.project_log()):
            for artifact in os.listdir(self.workspace.paths.project_log()):
                try:
                    artifact_ext = os.path.splitext(artifact)[1]
                    if artifact_ext == '.dmp':
                        os.remove(os.path.join(self.workspace.paths.project_log(), artifact))
                        log.info(f"Removing pre-existing artifact {artifact} from calling Launcher.setup()")
                    # For logs, we are going to keep the file in existance and clear it to play nice with filesystem caching and
                    # our code reading the contents of the file
                    elif artifact_ext == '.log':
                        open(os.path.join(self.workspace.paths.project_log(), artifact), 'w').close() # clear it
                        log.info(f"Clearing pre-existing artifact {artifact} from calling Launcher.setup()")
                except PermissionError:
                    log.warning(f'Unable to remove artifact: {artifact}, skipping.')
                    pass

        # In case this is the first run, we will create default logs to prevent the logmonitor from not finding the file
        os.makedirs(self.workspace.paths.project_log(), exist_ok=True)
        default_logs = ["Editor.log", "Game.log"]
        for default_log in default_logs:
            default_log_path = os.path.join(self.workspace.paths.project_log(), default_log)
            if not os.path.exists(default_log_path):
                open(default_log_path, 'w').close() # Create it

        # Wait for the AssetProcessor to be open.
        if launch_ap:
            self.workspace.asset_processor.start(connect_to_ap=True, connection_timeout=10)  # verify connection
            self.workspace.asset_processor.wait_for_idle()
            log.debug('AssetProcessor started from calling Launcher.setup()')

    def backup_settings(self):
        """
        Perform settings backup, storing copies of bootstrap, platform and user settings in the workspace's temporary
        directory. Must be called after settings have been generated, in case they don't exist.

        These backups will be lost after the workspace is torn down.

        :return: None
        """
        backup_path = self.workspace.settings.get_temp_path()
        log.debug(f"Performing automatic backup of bootstrap, platform and user settings in path {backup_path}")
        self.workspace.settings.backup_platform_settings(backup_path)
        self.workspace.settings.backup_shader_compiler_settings(backup_path)

    def configure_settings(self):
        """
        Perform settings configuration, must be called after a backup of settings has been created with
        backup_settings(). Preferred ways to modify settings are:

        self.workspace.settings.modify_platform_setting()

        :return: None
        """
        log.debug("No-op settings configuration requested")
        pass

    def restore_settings(self):
        """
        Restores the settings backups created with backup_settings(). Must be called during teardown().

        :return: None
        """
        backup_path = self.workspace.settings.get_temp_path()
        log.debug(f"Restoring backup of bootstrap, platform and user settings in path {backup_path}")
        self.workspace.settings.restore_platform_settings(backup_path)
        self.workspace.settings.restore_shader_compiler_settings(backup_path)

    def teardown(self):
        """
        Perform teardown of this launcher, undoing actions taken by calling setup()
        Subclasses should call its parent's teardown() after performing its own teardown.

        :return: None
        """
        self.workspace.asset_processor.stop()
        self.save_project_log_files()

    def save_project_log_files(self):
        # type: () -> None
        """
        Moves all .dmp and .log files from the project log folder into the artifact manager's destination

        :return: None
        """
        # A healthy large limit boundary
        amount_of_log_name_collisions = 100
        if os.path.exists(self.workspace.paths.project_log()):
            for artifact in os.listdir(self.workspace.paths.project_log()):
                if artifact.endswith('.dmp') or artifact.endswith('.log'):
                    self.workspace.artifact_manager.save_artifact(
                        os.path.join(self.workspace.paths.project_log(), artifact),
                        amount=amount_of_log_name_collisions)

    def binary_path(self):
        """
        Return this launcher's path to its binary file (exe, app, apk, etc).
        Only required if the platform supports it.

        :return: Complete path to the binary (if supported)
        """
        raise NotImplementedError("There is no binary file for this launcher")

    def start(self, backupFiles=True, launch_ap=None, configure_settings=True):
        """
        Automatically prepare and launch the application
        When called using "with launcher.start():" it will automatically call stop() when block exits
        Subclasses should avoid overriding this method

        :return: Application wrapper for context management, not intended to be called directly
        """
        return _Application(self, backupFiles, launch_ap=launch_ap, configure_settings=configure_settings)

    def _start_impl(self, backupFiles = True, launch_ap=None, configure_settings=True):
        """
        Implementation of start(), intended to be called via context manager in _Application

        :param backupFiles: Bool to backup settings files
        :return None:
        """
        self.setup(backupFiles=backupFiles, launch_ap=launch_ap, configure_settings=configure_settings)
        self.launch()

    def stop(self):
        """
        Terminate the application and perform automated teardown, the opposite of calling start()
        Called automatically when using "with launcher.start():"

        :return None:
        """
        self.kill()
        self.ensure_stopped()
        self.teardown()

    def is_alive(self):
        """
        Return whether the launcher is alive.

        :return: True if alive, False otherwise
        """
        raise NotImplementedError("is_alive is not implemented")

    def launch(self):
        """
        Launch the game, this method can perform a quick verification after launching, but it is not required.

        :return None:
        """
        raise NotImplementedError("Launch is not implemented")

    def kill(self):
        """
        Force stop the launcher.

        :return None:
        """
        raise NotImplementedError("Kill is not implemented")

    def package(self):
        """
        Performs actions required to create a launcher-package to be deployed for the given target.
        This command will package without deploying.
        This function is not applicable for PC, Mac, and ios.

        Subclasses should override only if needed. The default behavior is to do nothing.

        :return None:
        """
        log.debug("No-op package requested")
        pass

    def wait(self, timeout=30):
        """
        Wait for the launcher to end gracefully, raises exception if process is still running after specified timeout
        """
        ly_test_tools.environment.waiter.wait_for(
            lambda: not self.is_alive(),
            exc=ly_test_tools.launchers.exceptions.WaitTimeoutError("Application is unexpectedly still active"),
            timeout=timeout
        )

    def ensure_stopped(self, timeout=30):
        """
        Wait for the launcher to end gracefully, if the process is still running after the specified timeout, it is
        killed by calling the kill() method.

        :param timeout: Timeout in seconds to wait for launcher to be killed
        :return None:
        """
        try:
            ly_test_tools.environment.waiter.wait_for(
                lambda: not self.is_alive(),
                exc=ly_test_tools.launchers.exceptions.TeardownError("Application is unexpectedly still active"),
                timeout=timeout
            )
        except ly_test_tools.launchers.exceptions.TeardownError:
            self.kill()

    def get_device_config(self, config_file, device_section, device_key):
        """
        Takes an .ini config file path, .ini section name, and key for the value to search
            inside of that .ini section. Returns a string representing a device identifier, i.e. an IP.

        :param config_file: string representing the file path for the config ini file.
            default is '~/ly_test_tools/devices.ini'
        :param device_section: string representing the section to search in the ini file.
        :param device_key: string representing the key to search in device_section.
        :return: value held inside of 'device_key' from 'device_section' section,
            otherwise raises a SetupError.
        """
        config_dict = self._config_ini_to_dict(config_file)
        section_dict = {}
        device_value = ''

        # Verify 'device_section' and 'device_key' are valid, then return value inside 'device_key'.
        try:
            section_dict = config_dict[device_section]
        except (AttributeError, KeyError, ValueError) as err:
            problem = ly_test_tools.launchers.exceptions.SetupError(
                f"Could not find device section '{device_section}' from ini file: '{config_file}'")
            six.raise_from(problem, err)
        try:
            device_value = section_dict[device_key]
        except (AttributeError, KeyError, ValueError) as err:
            problem = ly_test_tools.launchers.exceptions.SetupError(
                f"Could not find device key '{device_key}' "
                f"from section '{device_section}' in ini file: '{config_file}'")
            six.raise_from(problem, err)

        return device_value


class _Application(object):
    """
    Context-manager for opening an application, enables using both "launcher.start()" and "with launcher.start()"
    """
    def __init__(self, launcher, backupFiles = True, launch_ap=None, configure_settings=True):
        """
        Called during both "launcher.start()" and "with launcher.start()"

        :param launcher: launcher-object to manage
        :return None:
        """
        self.launcher = launcher
        launcher._start_impl(backupFiles, launch_ap, configure_settings)

    def __enter__(self):
        """
        PEP-343 Context manager begin-hook
        Runs at the start of "with launcher.start()"

        :return None:
        """
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        PEP-343 Context manager end-hook
        Runs at the end of "with launcher.start()" block

        :return None:
        """
        self.launcher.stop()
