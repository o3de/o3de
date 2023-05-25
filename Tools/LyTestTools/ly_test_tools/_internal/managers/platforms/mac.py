"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Inside this module are 2 classes used for Mac directory & workspace mappings:
1. _MacResourceLocator(AbstractResourceLocator) derived class.
2. MacWorkspaceManager(AbstractWorkspaceManager) derived class.
"""

import os
import logging

from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools._internal.managers.abstract_resource_locator import AbstractResourceLocator

logger = logging.getLogger(__name__)

CACHE_DIR = 'mac'


class _MacResourceLocator(AbstractResourceLocator):
    """
    Override for locating resources in a Mac operating system running LyTestTools.
    """

    def platform_cache(self):
        """
        Return path to the cache for the Mac operating system.
        :return: path to cache for the Mac operating system
        """
        return os.path.join(self.project_cache(), CACHE_DIR)

    def project_log(self):
        """
        Return path to the project's log dir for the Mac operating system.
        :return: path to 'log' dir in the platform cache dir
        """
        return os.path.join(self.project(), 'user', 'log')

    def project_screenshots(self):
        """
        Return path to the project's screenshot dir for the Mac operating system.
        :return: path to 'screenshot' dir in the platform cache dir
        """
        return os.path.join(self.project(), 'user', 'ScreenShots')

    def crash_log(self):
        """
        Return path to the project's crash log dir using the builds project and platform
        :return: path to crash.log
        """
        return os.path.join(self.project_log(), "crash.log")


class MacWorkspaceManager(AbstractWorkspaceManager):
    """
    A Mac host WorkspaceManager. Contains Mac overridden functions for the AbstractWorkspaceManager class.
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
        super(MacWorkspaceManager, self).__init__(
            _MacResourceLocator(build_directory, project),
            project=project,
            tmp_path=tmp_path,
            output_path=output_path,
        )
