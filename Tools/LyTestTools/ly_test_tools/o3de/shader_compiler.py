"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A class to control functionality of Lumberyard's shader compiler.
The class manages a shader compiler process in a workspace.
"""
import logging
import subprocess

import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.process_utils as process_utils
from ly_test_tools import MAC

logger = logging.getLogger(__name__)


class ShaderCompiler():
    def __init__(self, workspace):
        # type: (AbstractWorkspaceManager) -> ShaderCompiler
        """
        Takes in an WorkspaceManager to set the path from the ResourceLocator

        :param workspace: The workspace to use to locate path to shader compiler executable
        """
        self._workspace = workspace
        self._sc_proc = None

    def start(self):
        """
        Starts the shader compiler and stores the process in self._sc_proc.

        :return: None
        """
        if self._sc_proc is not None:
            logger.info(
                f'Attempted to start shader compiler at the path: {self._workspace.paths.get_shader_compiler_path()}, '
                'but we already have one open!')
            return

        if MAC:
            raise NotImplementedError('Mac shader compiler not implemented.')

        logger.info(f"Build::setup_start_shadercompiler -- {self._workspace.paths.get_shader_compiler_path()}")
        # Running with basic user permissions since the shader compile server
        # warns against running as admin
        self._sc_proc = subprocess.Popen([
            'RunAs',
            '/trustlevel:0x20000',
            self._workspace.paths.get_shader_compiler_path(),
        ])

    def stop(self):
        """
        Stops the shader compiler stored in _sc_proc.

        :return: None
        """
        if self._sc_proc is None:
            logger.info(
                f'Attempted to stop shader compiler at the path: {self._workspace.paths.get_shader_compiler_path()}, '
                'but we do not have any open!')
            return
        logger.info('Killing shader compiler process.')
        process_utils.kill_processes_started_from(self._workspace.paths.get_shader_compiler_path())
        waiter.wait_for(lambda: self._sc_proc.poll() is not None)
        self._sc_proc = None

