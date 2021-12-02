"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Main launchers module, provides a facade for creating launchers.
"""

import logging

import ly_test_tools
import ly_test_tools._internal.managers.workspace as workspace_manager
import ly_test_tools.launchers.platforms.base as base_launcher

log = logging.getLogger(__name__)


def create_launcher(workspace, launcher_platform=ly_test_tools.HOST_OS_PLATFORM, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, list[str]) -> base_launcher.Launcher
    """
    Create a launcher compatible with the specified workspace, if no specific launcher is found return a generic one.

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows' or 'android')
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    if not launcher_class:
        log.warning(f"Using default launcher for '{ly_test_tools.HOST_OS_PLATFORM}' "
                    f"as no option is available for '{launcher_platform}'")
        launcher_class = ly_test_tools.LAUNCHERS.get(ly_test_tools.HOST_OS_PLATFORM)
    return launcher_class(workspace, args)


def create_dedicated_launcher(workspace, launcher_platform=ly_test_tools.HOST_OS_DEDICATED_SERVER, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, list[str]) -> base_launcher.Launcher
    """
    Create a dedicated launcher compatible with the specified workspace.  Dedicated Launcher is only supported on the
    Linux and Windows Platform

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows_dedicated' for DedicatedWinLauncher)
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    if not launcher_class:
        log.warning(f"Using default dedicated launcher for '{ly_test_tools.HOST_OS_DEDICATED_SERVER}' "
                    f"as no option is available for '{launcher_platform}'")
        launcher_class = ly_test_tools.LAUNCHERS.get(ly_test_tools.HOST_OS_DEDICATED_SERVER)
    return launcher_class(workspace, args)


def create_editor(workspace, launcher_platform=ly_test_tools.HOST_OS_EDITOR, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, list[str]) -> base_launcher.Launcher
    """
    Create an Editor compatible with the specified workspace.
    Editor is only officially supported on the Windows Platform.

    :param workspace: lumberyard workspace to use
     :param launcher_platform: the platform to target for a launcher (i.e. 'windows_dedicated' for DedicatedWinLauncher)
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Editor instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    if not launcher_class:
        log.warning(f"Using default editor launcher for '{ly_test_tools.HOST_OS_EDITOR}' "
                    f"as no option is available for '{launcher_platform}'")
        launcher_class = ly_test_tools.LAUNCHERS.get(ly_test_tools.HOST_OS_EDITOR)
    return launcher_class(workspace, args)


def create_generic_launcher(workspace, launcher_platform, exe_file_name, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, str, list[str]) -> base_launcher.Launcher
    """
    Create a generic launcher compatible with the specified workspace.
    Allows custom .exe files to serve as the launcher instead of ones listed in the ly_test_tools.LAUNCHERS constant

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows' for WinLauncher)
    :param exe_file_name: .exe file name which has to be launched for this launcher (i.e. 'MaterialEditor.exe')
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance.
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    if not launcher_class:
        log.warning(f"Using default generic executable launcher for '{ly_test_tools.HOST_OS_GENERIC_EXECUTABLE}' "
                    f"as no option is available for '{launcher_platform}'")
        launcher_class = ly_test_tools.LAUNCHERS.get(ly_test_tools.HOST_OS_GENERIC_EXECUTABLE)
    return launcher_class(workspace, exe_file_name, args)
