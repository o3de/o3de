"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Linux directory and workspace mappings
"""

import os
import logging

from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools._internal.managers.abstract_resource_locator import AbstractResourceLocator

logger = logging.getLogger(__name__)

CACHE_DIR = 'linux'


class _LinuxResourceManager(AbstractResourceLocator):
    """
    Override for locating resources in a Linux operating system running LyTestTools.
    """

    def platform_cache(self):
        """
        :return: path to cache for the Linux operating system
        """
        return os.path.join(self.project_cache(), CACHE_DIR)

    def project_log(self):
        """
        :return: path to 'log' dir in the platform cache dir
        """
        return os.path.join(self.project(), 'user', 'log')

    def project_screenshots(self):
        """
        :return: path to 'screenshot' dir in the platform cache dir
        """
        return os.path.join(self.project(), 'user', 'ScreenShots')

    def crash_log(self):
        """
        Return path to the project's crash log dir using the builds project and platform
        :return: path to Crash.log
        """
        return os.path.join(self.project_log(), "crash.log")


class LinuxWorkspaceManager(AbstractWorkspaceManager):
    """
    A Linux host WorkspaceManager. Contains Mac overridden functions for the AbstractWorkspaceManager class.
    Also creates a Mac host ResourceLocator for directory and build mappings.
    """
    def __init__(
            self,
            build_directory=None,
            project=None,
            tmp_path=None,
            output_path=None,
    ):
        # Type: (str,str,str,str) -> None
        super(LinuxWorkspaceManager, self).__init__(
            _LinuxResourceManager(build_directory, project),
            project=project,
            tmp_path=tmp_path,
            output_path=output_path,
        )
