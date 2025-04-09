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


# These are the launchers *currently* supported by the test tools. While other launchers exist, they are not supported
# by the test tools.
GAME_LAUNCHERS = ['windows', 'linux', 'android']
SERVER_LAUNCHERS = ['windows_dedicated', 'linux_dedicated']

log = logging.getLogger(__name__)


def create_game_launcher(workspace, launcher_platform=ly_test_tools.HOST_OS_PLATFORM, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, list[str]) -> base_launcher.Launcher
    """
    Create a GameLauncher compatible with the specified workspace, defaulting to a generic one otherwise.

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows' or 'android')
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance
    """
    if launcher_platform in GAME_LAUNCHERS:
        launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    else:
        log.warning(f"Using default launcher for '{ly_test_tools.HOST_OS_PLATFORM}' "
                    f"as no option is available for '{launcher_platform}'")
        launcher_class = ly_test_tools.LAUNCHERS.get(ly_test_tools.HOST_OS_PLATFORM)
    return launcher_class(workspace, args)


def create_server_launcher(workspace, launcher_platform=ly_test_tools.HOST_OS_PLATFORM, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, list[str]) -> base_launcher.Launcher
    """
    Create a ServerLauncher compatible with the specified workspace, defaulting to a generic one otherwise.

    :param workspace: lumberyard workspace to use
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows' or 'android')
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Launcher instance
    """
    if launcher_platform in SERVER_LAUNCHERS:
        launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    else:
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


def create_atom_tools_launcher(workspace, app_file_name, launcher_platform=ly_test_tools.HOST_OS_ATOM_TOOLS, args=None):
    # type: (workspace_manager.AbstractWorkspaceManager, str, str, list[str]) -> base_launcher.Launcher
    """
    Create an Atom Tools launcher compatible with the specified workspace.
    Most Atom Tools executables are only officially supported on the Windows Platform.
    :param workspace: lumberyard workspace to use
    :param app_file_name: name of the file to look for
    :param launcher_platform: the platform to target for a launcher (i.e. 'windows_atom_tools' for WinAtomToolsLauncher)
    :param args: List of arguments to pass to the launcher's 'args' argument during construction
    :return: Atom Tools application instance
    """
    launcher_class = ly_test_tools.LAUNCHERS.get(launcher_platform)
    if not launcher_class:
        log.warning(f"Using default Atom Tools launcher for '{ly_test_tools.HOST_OS_ATOM_TOOLS}' "
                    f"as no option is available for '{launcher_platform}'")
        launcher_class = ly_test_tools.LAUNCHERS.get(ly_test_tools.HOST_OS_ATOM_TOOLS)
    return launcher_class(workspace, app_file_name, args)
