"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Workspace Manager: Provides an API for managing lumberyard installations and saving of files
"""
import abc
import datetime
import logging
import os
import subprocess
import tempfile

import ly_test_tools.environment.file_system
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.o3de.asset_processor
import ly_test_tools.o3de.settings as settings
import ly_test_tools.o3de.shader_compiler
import ly_test_tools._internal.managers.artifact_manager as artifact_manager
import ly_test_tools._internal.managers.abstract_resource_locator as arl

logger = logging.getLogger(__name__)


class AbstractWorkspaceManager:
    __metaclass__ = abc.ABCMeta
    """
    Base workspace manager: provides a simple managed setup/teardown.
    All workspace managers are subclasses of this.
    """

    def __init__(self,  # type: AbstractWorkspaceManager
                 resource_locator,  # type: arl.AbstractResourceLocator
                 project,  # type: str
                 tmp_path=None,  # type: str or None
                 output_path=None,  # type: str or None
                 ):  # type: (...) -> None
        """
        Create a workspace manager with an associated AbstractResourceLocator object and initialize temp and logs dirs.
        The workspace contains information about the workspace being used and the running pytest test.

        :param resource_locator: A resource locator to create paths for the workspace
        :param project: O3DE project to use for the LumberyardRelease object
        :param tmp_path: A path to use for storing temp files, if not specified default to the system's tmp
        :param output_path: A path used to store artifacts, if not specified defaults to
            "<build>\\dev\\TestResults\\<timestamp>"
        """
        self.paths = resource_locator
        self.project = project
    
        self.artifact_manager = artifact_manager.NullArtifactManager()
        self.asset_processor = ly_test_tools.o3de.asset_processor.AssetProcessor(self)
        self.shader_compiler = ly_test_tools.o3de.shader_compiler.ShaderCompiler(self)
        self._original_cwd = os.getcwd()
        self.tmp_path = tmp_path
        self.output_path = output_path

        if not self.tmp_path:
            self.tmp_path = tempfile.mkdtemp()

        self.settings = settings.LySettings(self.tmp_path, self.paths)

        if not self.output_path:
            self.output_path = os.path.join(self.paths.test_results(),
                                            datetime.datetime.now().strftime('%Y-%m-%dT%H_%M_%S_%f'))
            logger.info(f"No logs path set, using default based on timestamp: {self.output_path}")

    def setup(self):
        """
        Perform default setup for this workspace. Configures tmp and logs path, loggers

        Derived classes should call this before calling its own setup code (Unless you really know what you are doing).

        :return: None
        """
        if self.tmp_path:
            print(f"Checking for tmp path {self.tmp_path}")

            if os.path.exists(self.tmp_path):
                print("Found existing tmp path, deleting")
                ly_test_tools.environment.file_system.delete([self.tmp_path], True, True)

            print("Creating tmp path")
            os.makedirs(self.tmp_path, exist_ok=True)

        if not os.path.exists(self.output_path):
            print(f"Creating logs path at {self.output_path}")
            os.makedirs(self.output_path, exist_ok=True)

        print(f"Configuring Artifact Manager with path {self.output_path}")
        self.artifact_manager = artifact_manager.ArtifactManager(self.output_path)

    def teardown(self):
        """
        Perform teardown on this workspace: call teardown() on the LumberyardRelease object and delete tmp_path.

        Derived classes should call this after calling its own teardown code (Unless you really know what you are doing)

        :return: None
        """
        logger.debug("Deleting tmp path")
        os.chdir(self._original_cwd)

        if self.tmp_path:
            ly_test_tools.environment.file_system.delete([self.tmp_path], True, True)

    def clear_cache(self):
        """
        Clears the Cache folder located at dev/Cache
        :return:  None
        """
        if os.path.exists(self.paths.cache()):
            logger.debug(f"Clearing {self.paths.cache()}")
            ly_test_tools.environment.file_system.delete([self.paths.cache()], True, True)
            return
        logger.debug(f"Cache directory: {self.paths.cache()} could not be found.")

    def clear_bin(self):
        """
        Clears the relative Bin folder (i.e. engine_root/dev/windows/bin/profile/)
        :return: None
        """
        if os.path.exists(self.paths.build_directory()):
            logger.debug(f"Clearing {self.paths.build_directory()}")
            ly_test_tools.environment.file_system.delete([self.paths.build_directory()], True, True)
            return
        logger.debug(f"build_directory directory: {self.paths.build_directory()} could not be found.")

    def _execute_and_save_log(self, command, log_file_name):
        """
        Executes a subprocess command and saves its output with the artifacts of current test

        :param command: command to execute
        :param log_file_name: artifact name to save
        :raises subprocess.CalledProcessError, OSError: on command failure
        """
        temp_file_dir = os.path.join(tempfile.gettempdir(), "LyTestTools")
        temp_file_path = os.path.join(temp_file_dir, log_file_name)
        if not os.path.exists(temp_file_dir):
            os.makedirs(temp_file_dir, exist_ok=True)

        if os.path.exists(temp_file_path):
            # assume temp is not being used for long-term storage, and safe to delete
            os.remove(temp_file_path)

        try:
            with open(temp_file_path, "w+") as logfile:
                try:
                    output = process_utils.check_output(command, stderr=subprocess.STDOUT)
                    logfile.write(output)
                except subprocess.CalledProcessError as err:
                    failure_output = err.output if err.output else "<no output>"
                    logger.exception(
                        f'Command "{command}" failed with exit code "{err.returncode}" '
                        f'and output: {failure_output.decode()}')
                    logfile.write(failure_output.decode())
                    raise
                finally:
                    self.artifact_manager.save_artifact(temp_file_path)
        except OSError:
            logger.exception(f'Command "{command}" failed due to a filesystem error.')
            raise
        finally:
            if os.path.exists(temp_file_path):
                try:
                    os.remove(temp_file_path)
                except Exception:  # purposefully broad
                    logger.warning(
                        f"Ignored exception while cleaning up file: {temp_file_dir}", exc_info=True)
