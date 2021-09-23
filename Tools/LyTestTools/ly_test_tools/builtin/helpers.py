"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Helper file for assisting in building workspaces and setting up LTT with the current Lumberyard environment.
"""

import ly_test_tools._internal.pytest_plugin as pytest_plugin
import ly_test_tools._internal.managers.workspace as internal_workspace
from ly_test_tools import LINUX, MAC, WINDOWS
import os, stat


def create_builtin_workspace(
        build_directory=None,  # type: str
        project="AutomatedTesting",  # type: str
        tmp_path=None,  # type: str or None
        output_path=None,  # type: str or None
        ):
    # type: (...) -> internal_workspace.AbstractWorkspaceManager
    """
    Create a new platform-specific workspace manager for the current lumberyard build

    :param build_directory: Custom path to the build directory (i.e. engine_root/dev/windows_vs2017/bin/profile)
        when set to None (default) it will use the value configured by pytest CLI argument --build-directory
    :param project: Project name to use
    :param tmp_path: Path to use as temporal storage, if not specified use default
    :param output_path: Path to use as log storage, if not specified use default

    :return: A workspace manager that works with the current lumberyard instance
    """
    if not build_directory:
        if pytest_plugin.build_directory:
            # cannot set argument default (which executes on import), must set here after pytest starts executing
            build_directory = pytest_plugin.build_directory
        else:
            raise ValueError(
                "Cmake build directory was not set via commandline arguments and not overridden. Please specify with "
                r"CLI argument --build-directory (example: --build-directory C:\lumberyard\dev\Win2019\bin\profile )")

    build_class = internal_workspace.AbstractWorkspaceManager
    if WINDOWS:
        from ly_test_tools._internal.managers.platforms.windows import WindowsWorkspaceManager
        build_class = WindowsWorkspaceManager
    elif MAC:
        from ly_test_tools._internal.managers.platforms.mac import MacWorkspaceManager
        build_class = MacWorkspaceManager
    elif LINUX:
        from ly_test_tools._internal.managers.platforms.linux import LinuxWorkspaceManager
        build_class = LinuxWorkspaceManager
    else:
        raise NotImplementedError("No workspace manager found for current Operating System")

    instance = build_class(
        build_directory=build_directory,
        project=project,
        tmp_path=tmp_path,
        output_path=output_path,
    )

    return instance


def setup_builtin_workspace(workspace, test_name, artifact_folder_count):
    # type: (internal_workspace.AbstractWorkspaceManager, str, int) -> internal_workspace.AbstractWorkspaceManager
    """
    Reconfigures a workspace instance to its defaults.
    Usually test authors should rely on the provided "workspace" fixture, but these helpers can be used to
        achieve the same result.

    :param workspace: workspace to use
    :param test_name: the test name to be used by the artifact manager
    :param artifact_folder_count: the number of folders to create for the test_name, each one will have an index
        appended at the end to handle naming collisions.
    :return: the configured workspace object, useful for method chaining
    """
    workspace.setup()
    workspace.artifact_manager.set_test_name(test_name=test_name, amount=artifact_folder_count)

    return workspace


def teardown_builtin_workspace(workspace):
    # type: (internal_workspace.AbstractWorkspaceManager) -> internal_workspace.AbstractWorkspaceManager
    """
    Stop the asset processor and perform teardown on the specified workspace.
    Usually test authors should rely on the provided "workspace" fixture, but these helpers can be used to
        achieve the same result.

    :param workspace: workspace to use
    :return: the configured workspace object, useful for method chaining
    """
    workspace.teardown()

    return workspace
