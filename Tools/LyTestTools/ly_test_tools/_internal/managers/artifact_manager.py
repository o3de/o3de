"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Workspace Manager: Provides an API for managing lumberyard installations and file manipulation
"""

import logging
import os
import shutil
import six
import stat
import tempfile

import ly_test_tools.environment.file_system as file_system

logger = logging.getLogger(__name__)


class ArtifactManager(object):
    def __init__(self, root):
        self.artifact_path = root  # i.e.: ~/dev/TestResults/2019-10-15T13_38_42_855000/
        self.dest_path = None
        self.set_dest_path()  # Sets the self.dest_path attribute as the main artifact save path for test files.

    def _get_collision_handled_filename(self, file_path, amount=1):
        # type: (str, int) -> str
        """
        Handles filename collision by appending integers, checking if the file exists, then incrementing if so. Will
        increase up to the amount parameter before stopping.

        :param file_path: The file path as a string to check for name collisions
        :param amount: amount of renames possible for the string if file_path collision occurs by appending integers to
        the name. If the amount is reached, the save will override the file instead.
        :return: The new file_path as a string
        """
        # Name collision handling not needed for 1 name
        if amount == 1:
            return file_path

        # extension will be an empty string if it doesn't exist
        file_without_ext, extension = os.path.splitext(file_path)

        for i in range(1, amount):  # Start at "_1" instead of "_0"
            updated_path = f"{file_without_ext}_{i}{extension}"
            if not os.path.exists(updated_path):
                return updated_path
        logger.warning(f"Maximum number of attempts: {amount} met when trying to handle name collision for file: "
                       f"{file_path}. Ending on {updated_path} which will override the existing file.")
        return updated_path

    def set_dest_path(self, test_name=None, amount=1):
        """
        Sets self.dest_path if not set, and returns the value currently set in self.dest_path. Also creates the
        directory if it already doesn't exist.

        :param test_name: Set the test name used to log the artifacts, format of "module_class_method"
            i.e. "test_module_TestClass_test_BasicTestMethod_ValidInputTest_ReturnsTrue_1"
            If set, all artifacts are saved to a subdir named after this test. This value will get appended to the main
            path in the self.dest_path attribute.
        :param amount: The amount of folders to create matching self.dest_path and adding an index value to each.
        :return: None, but sets the self.dest_path attribute when called.
        """
        if test_name:
            self.dest_path = os.path.join(self.dest_path, file_system.sanitize_file_name(test_name))
        elif not self.dest_path:
            self.dest_path = self.artifact_path

        # Create unique artifact folder
        if not os.path.exists(self.dest_path):
            self.dest_path = self._get_collision_handled_filename(self.dest_path, amount)
            try:
                logger.debug(f'Attempting to create new artifact path: "{self.dest_path}"')
                if not os.path.exists(self.dest_path):
                    os.makedirs(self.dest_path, exist_ok=True)
                    logger.info(f'Created new artifact path: "{self.dest_path}"')
                    return self.dest_path
            except (IOError, OSError, WindowsError) as err:
                problem = WindowsError(f'Failed to create new artifact path: "{self.dest_path}"')
                six.raise_from(problem, err)

    def generate_folder_name(self, test_module, test_class, test_method):
        """
        Takes a test module, class, & method and generates a folder name string with an added
        count value to make the name unique for test methods that run multiple times.
        Returns the newly generated folder name string.
        :param test_module: string for the name of the current test module
        :param test_class: string for the name of the current test class
        :param test_method: string for the name of the current test method
        :return: string for naming a folder that represents the current test, with a maximum length
            of 60 trimmed down to match this requirement.
        """
        folder_name = file_system.reduce_file_name_length(
            file_name="{}_{}_{}".format(test_module, test_class, test_method),
            max_length=60)

        return folder_name

    def save_artifact(self, artifact_path, artifact_name=None, amount=1):
        """
        Store an artifact to be logged.  Will ensure the new artifact is writable to prevent directory from being
        locked later.

        :param artifact_path: string representing the full path to the artifact folder.
        :param artifact_name: string representing a new artifact name for log if necessary, max length: 25 characters.
        :param amount: amount of renames possible for the saved artifact if file name collision occurs by appending
        integers to the name. If the amount is reached, the save will override the file instead.
        :return destination_path: a destination folder if a folder is copied, a destination file path if a file is copied
        """
        if artifact_name:
            artifact_name = file_system.reduce_file_name_length(file_name=artifact_name, max_length=25)
        dest_path = os.path.join(self.dest_path,
                                 artifact_name if artifact_name is not None else os.path.basename(artifact_path))
        if os.path.exists(dest_path):
            dest_path = self._get_collision_handled_filename(dest_path, amount)
        logger.debug("Copying artifact from '{}' to '{}'".format(artifact_path, dest_path))

        dest_folder, _ = os.path.split(dest_path)
        try:
            if not os.path.exists(dest_folder):
                os.makedirs(dest_folder, exist_ok=True)

            if os.path.isdir(artifact_path):
                shutil.copytree(artifact_path, dest_path, dirs_exist_ok=True)
            else:
                shutil.copy(artifact_path, dest_path)
                os.chmod(dest_path, stat.S_IWRITE | stat.S_IREAD | stat.S_IEXEC)
        except FileExistsError as exc:
            logger.exception(f"Error occurred while trying to save {dest_path}. Printing contents of folder:\n"
                             f"{os.listdir(dest_folder)}\n")
            raise exc
        return dest_path

    def generate_artifact_file_name(self, artifact_name):
        """
        Returns a string for generating a new artifact file inside of the artifact folder.
        :param artifact_name: string representing the name for the artifact file (i.e. "ToolsInfo.log")
        :return: string for the full path to the file inside the artifact path even if not valid, i.e.:
            ~/dev/TestResults/2019-10-14T11_36_12_234000/pytest_results/test_module_TestClass_Method_1/ToolsInfo.log
        """
        if not artifact_name:
            raise ValueError(f'artifact_name is a required parameter. Actual: {artifact_name}')
        return os.path.join(self.dest_path, artifact_name)

    def gather_artifacts(self, destination, format='zip'):
        """
        Gather collected artifacts to the specified destination as an archive file (zip by default).
        Destination should not contain file extension, the second parameter automatically determines the best extension
        to use.
        :param destination: where to write the archive file, do not add extension
        :param format: archive format, default is 'zip', possible values: tar, gztar and bztar.
        :return: full path to the generated archive or raises a WindowsError if shutil.mark_archive() fails.
        """
        try:
            return shutil.make_archive(destination, format, self.dest_path)
        except WindowsError:
            logger.exception(
                f'Windows failed to find the target artifact path: "{self.dest_path}" '
                'which may indicate test setup failed.')


class NullArtifactManager(ArtifactManager):
    """
    An ArtifactManager that ignores all calls, used when logging is not configured.
    """
    def __init__(self):
        # The null ArtifactManager redirects all calls to a temp dir
        super(NullArtifactManager, self).__init__(tempfile.mkdtemp())

    def _get_collision_handled_filename(self, artifact_path=None, amount=None):
        raise NotImplementedError("Attempt was made to create artifact save paths through NullArtifactManager.")

    def save_artifact(self, artifact, artifact_name=None, amount=None):
        return None

    def gather_artifacts(self, destination, format='zip'):
        return None
