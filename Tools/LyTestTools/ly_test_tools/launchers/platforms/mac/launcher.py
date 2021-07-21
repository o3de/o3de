"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Wrapper to manage launching Lumberyard-created apps on OSX
"""
import logging
import os
import subprocess

import ly_test_tools.environment.waiter
import ly_test_tools.launchers.exceptions

from ly_test_tools.launchers.platforms.base import Launcher

log = logging.getLogger(__name__)


class MacLauncher(Launcher):
    def __init__(self, workspace, args):
        super(MacLauncher, self).__init__(workspace, args)
        self._proc = None
        log.debug("Initialized Mac Launcher")

    def binary_path(self):
        """
        Return full path to the launcher for this build's configuration and project

        :return: full path to <project>.GameLauncher.exe
        """
        assert self.workspace.project is not None, "Project is not configured in Workspace"
        appname = f"{self.workspace.project}.GameLauncher"
        return os.path.join(self.workspace.paths.build_directory(), f"{appname}.app", "Contents", "MacOS", appname)

    def launch(self):
        """
        Launch the executable and track the subprocess

        :return: None
        """
        command = [self.binary_path()] + self.args
        self._proc = subprocess.Popen(command)
        log.debug(f"Started Mac Launcher with command: {command}")

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
                    f"Unable to terminate active Mac Launcher with process ID {self._proc.pid}"))
            self._proc = None
            log.debug("Mac Launcher terminated successfully")

    def is_alive(self):
        """
        Check the process to verify activity.  Side effect of setting self._proc to None if it has ended.

        :return: None
        """
        if self._proc is None:
            return False
        else:
            if self._proc.poll() is not None:
                self._proc = None
            return self._proc is not None
