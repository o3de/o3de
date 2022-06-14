"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Inside this module are 2 classes used for Windows directory & workspace mappings:
1. _WindowsResourceLocator(AbstractResourceLocator) derived class.
2. WindowsWorkspaceManager(AbstractWorkspaceManager) derived class.
"""

import logging
import os
import sys

import ly_test_tools.environment.process_utils
import ly_test_tools.environment.reg_cleaner
import ly_test_tools.environment.waiter
import ly_test_tools.launchers.exceptions

from ly_test_tools._internal.managers.abstract_resource_locator import AbstractResourceLocator
from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager

logger = logging.getLogger(__name__)

CACHE_DIR = 'pc'
CONFIG_FILE = 'system_windows_pc.cfg'


class _WindowsResourceLocator(AbstractResourceLocator):
    """
    Override for locating resources in a Windows operating system running LyTestTools.
    """

    def platform_config_file(self):
        """
        Return the path to the platform config file.
        ex. engine_root/dev/system_osx_mac.cfg
        :return: path to the platform config file
        """
        return os.path.join(self.engine_root(), CONFIG_FILE)

    def platform_cache(self):
        """
        Return path to the cache for the Windows operating system.
        :return: path to cache for the Windows operating system
        """
        return os.path.join(self.project_cache(), CACHE_DIR)

    def project_log(self):
        """
        Return path to the project's log dir for the Windows operating system.
        :return: path to 'log' dir in the platform cache dir
        """
        return os.path.join(self.project(), 'user', 'log')

    def project_screenshots(self):
        """
        Return path to the project's screenshot dir for the Windows operating system.
        :return: path to 'screenshot' dir in the platform cache dir
        """
        return os.path.join(self.project(), 'user', 'ScreenShots')

    def editor_log(self):
        """
        Return path to the project's editor log dir using the builds project and platform
        :return: path to Editor.log
        """
        return os.path.join(self.project_log(), "Editor.log")

    def crash_log(self):
        """
        Return path to the project's crash log dir using the builds project and platform
        :return: path to Error.log
        """
        return os.path.join(self.project_log(), "error.log")

class WindowsWorkspaceManager(AbstractWorkspaceManager):
    """
    A Windows host WorkspaceManager. Contains Windows overridden functions for the AbstractWorkspaceManager class.
    Also creates a Windows host ResourceLocator.
    """
    def __init__(
            self,
            build_directory=None,
            project=None,
            tmp_path=None,
            output_path=None,
    ):
        # Type: (str,str,str,str,str) -> None 
        super(WindowsWorkspaceManager, self).__init__(
            _WindowsResourceLocator(build_directory, project),
            project=project,
            tmp_path=tmp_path,
            output_path=output_path,
        )

    def set_registry_keys(self):
        """
        Set the certain registry flags that are required to be set before compilation will succeed.
        """
        if sys.platform == 'win32':
            ly_test_tools.environment.reg_cleaner.create_ly_keys()

    def clear_settings(self):
        logger.debug("Build::setup_clear_registry")
        if sys.platform == "win32":
            ly_test_tools.environment.reg_cleaner.clean_ly_keys(exception_list=r"SOFTWARE\O3DE\O3DE\Identity")
