"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Linux compatible launcher
"""

import logging
import os
import subprocess

import ly_test_tools.environment.waiter
import ly_test_tools.launchers.exceptions
import ly_test_tools.environment.process_utils as process_utils

from ly_test_tools.launchers.platforms.base import Launcher
from ly_test_tools.launchers.exceptions import TeardownError, ProcessNotStartedError
from tempfile import TemporaryFile

log = logging.getLogger(__name__)


class LinuxLauncher(Launcher):
    def __init__(self, build, args):
        super(LinuxLauncher, self).__init__(build, args)
        self._proc = None
        self._ret_code = None
        self._tmpout = None
        log.debug("Initialized Linux Launcher")

    def binary_path(self):
        """
        Return full path to the launcher for this build's configuration and project

        :return: full path to <project>.GameLauncher
        """
        assert self.workspace.project is not None
        return os.path.join(self.workspace.paths.build_directory(), f"{self.workspace.project}.GameLauncher")

    def setup(self, backupFiles=True, launch_ap=True, configure_settings=True):
        """
        Perform setup of this launcher, must be called before launching.
        Subclasses should call its parent's setup() before calling its own code, unless it changes configuration files

        :param backupFiles: Bool to backup setup files
        :param lauch_ap: Bool to lauch the asset processor
        :return: None
        """
        # Backup
        if backupFiles:
            self.backup_settings()

        # Base setup defaults to None
        if launch_ap is None:
            launch_ap = True

        # Modify and re-configure
        if configure_settings:
            self.configure_settings()
        super(LinuxLauncher, self).setup(backupFiles, launch_ap)

    def launch(self):
        """
        Launch the executable and track the subprocess

        :return: None
        """
        command = [self.binary_path()] + self.args
        self._tmpout = TemporaryFile()
        self._proc = subprocess.Popen(command, stdout=self._tmpout, stderr=self._tmpout, universal_newlines=True, env=process_utils.get_display_env())
        log.debug(f"Started Linux Launcher with command: {command}")

    def get_output(self, encoding="utf-8"):
        if self._tmpout is None:
            raise ProcessNotStartedError("Process must be started before retrieving output")

        self._tmpout.seek(0)
        return self._tmpout.read().decode(encoding)

    def teardown(self):
        """
        Perform teardown of this launcher, undoing actions taken by calling setup()
        Subclasses should call its parent's teardown() after performing its own teardown.

        :return: None
        """
        self.restore_settings()
        super(LinuxLauncher, self).teardown()

    def kill(self):
        """
        This is a hard kill, and then wait to make sure until it actually ended.

        :return: None
        """
        if self._proc is not None:
            self._proc.kill()
            ly_test_tools.environment.waiter.wait_for(
                lambda: not self.is_alive(),
                exc=ly_test_tools.launchers.exceptions.TeardownError(
                    f"Unable to terminate active Linux Launcher with process ID {self._proc.pid}")
            )
            self._proc = None
            self._ret_code = None
            log.debug("Linux Launcher terminated successfully")

    def is_alive(self):
        """
        Check the process to verify activity.  Side effect of setting self.proc to None if it has ended.

        :return: None
        """
        if self._proc is None:
            return False
        else:
            if self._proc.poll() is not None:
                self._ret_code = self._proc.poll()
                self._proc = None
                return False
            return True

    def get_pid(self):
        # type: () -> int or None
        """
        Returns the pid of the launcher process if it exists, else it returns None

        :return: process id or None
        """
        if self._proc:
            return self._proc.pid
        return None

    def get_returncode(self):
        # type: () -> int or None
        """
        Returns the returncode of the launcher process if it exists, else return None.
        The returncode attribute is set when the process is terminated.

        :return: The returncode of the launcher's process
        """
        if self._proc:
            return self._proc.poll()
        else:
            return self._ret_code

    def check_returncode(self):
        # type: () -> None
        """
        Checks the return code of the launcher if it exists. Raises a CrashError if the returncode is non-zero. Returns
        None otherwise. This function must be called after exiting the launcher properly and NOT using its provided
        teardown(). Provided teardown() will always return a non-zero returncode and should not be checked.

        :return: None
        """
        return_code = self.get_returncode()
        if return_code != 0:
            log.error(f"Launcher exited with non-zero return code: {return_code}")
            raise ly_test_tools.launchers.exceptions.CrashError()
        return None

    def configure_settings(self):
        """
        Configures system level settings and syncs the launcher to the targeted console IP.

        :return: None
        """
        # Update settings via the settings registry to avoid modifying the bootstrap.cfg
        host_ip = '127.0.0.1'
        self.args.append(f'--regset="/Amazon/AzCore/Bootstrap/project_path={self.workspace.paths.project()}"')
        self.args.append(f'--regset="/Amazon/AzCore/Bootstrap/remote_ip={host_ip}"')
        self.args.append('--regset="/Amazon/AzCore/Bootstrap/wait_for_connect=1"')
        self.args.append(f'--regset="/Amazon/AzCore/Bootstrap/allowed_list={host_ip}"')

        self.workspace.settings.modify_platform_setting("r_ShaderCompilerServer", host_ip)
        self.workspace.settings.modify_platform_setting("log_RemoteConsoleAllowedAddresses", host_ip)


class DedicatedLinuxLauncher(LinuxLauncher):

    def setup(self, backupFiles=True, launch_ap=False):
        """
        Perform setup of this launcher, must be called before launching.
        Subclasses should call its parent's setup() before calling its own code, unless it changes configuration files

        :param backupFiles: Bool to backup setup files
        :param lauch_ap: Bool to lauch the asset processor
        :return: None
        """
        # Base setup defaults to None
        if launch_ap is None:
            launch_ap = False

        super(DedicatedLinuxLauncher, self).setup(backupFiles, launch_ap)

    def binary_path(self):
        """
        Return full path to the dedicated server launcher for the build directory.

        :return: full path to <project>Launcher_Server
        """
        assert self.workspace.project is not None, (
            'Project cannot be NoneType - please specify a project name string.')
        return os.path.join(f"{self.workspace.paths.build_directory()}",
                            f"{self.workspace.project}.ServerLauncher")


class LinuxEditor(LinuxLauncher):

    def __init__(self, build, args):
        super(LinuxEditor, self).__init__(build, args)
        self.args.append('--regset="/Amazon/Settings/EnableSourceControl=false"')
        self.args.append('--regset="/Amazon/AWS/Preferences/AWSAttributionConsentShown=true"')
        self.args.append('--regset="/Amazon/AWS/Preferences/AWSAttributionEnabled=false"')

    def binary_path(self):
        """
        Return full path to the Editor for this build's configuration and project

        :return: full path to Editor
        """
        assert self.workspace.project is not None
        return os.path.join(self.workspace.paths.build_directory(), "Editor")


class LinuxGenericLauncher(LinuxLauncher):

    def __init__(self, build, exe_file_name, args=None):
        super(LinuxLauncher, self).__init__(build, args)
        self.exe_file_name = exe_file_name
        self.expected_executable_path = os.path.join(
            self.workspace.paths.build_directory(), f"{self.exe_file_name}")

        if not os.path.exists(self.expected_executable_path):
            raise ProcessNotStartedError(
                f"Unable to locate executable '{self.exe_file_name}' "
                f"in path: '{self.expected_executable_path}'")

    def binary_path(self):
        """
        Return full path to the executable file for this build's configuration and project
        Relies on the build_directory() in self.workspace.paths to be accurate

        :return: full path to the given exe file
        """
        assert self.workspace.project is not None, (
            'Project cannot be NoneType - please specify a project name string.')
        return self.expected_executable_path
