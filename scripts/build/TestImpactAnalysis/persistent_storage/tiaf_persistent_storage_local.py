#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pathlib
import shutil
from persistent_storage import PersistentStorage
from tiaf_logger import get_logger

logger = get_logger(__file__)

# Implementation of local persistent storage


class PersistentStorageLocal(PersistentStorage):

    HISTORIC_KEY = "historic"
    DATA_KEY = "data"

    def __init__(self, config: dict, suites_string: str, commit: str, active_workspace: str, unpacked_coverage_data_file_path: str, previous_test_run_data_file_path: str, historic_workspace: str, historic_data_file_path: str, temp_workspace: str):
        """
        Initializes the persistent storage with any local historic data available.

        @param config: The runtime config file to obtain the data file paths from.
        @param suites_string:  The concatenated test suite string for which the historic data will be obtained for.
        @param commit: The commit hash for this build.
        """

        super().__init__(config, suites_string, commit, active_workspace, unpacked_coverage_data_file_path, previous_test_run_data_file_path, temp_workspace)
        self._retrieve_historic_data(config, historic_workspace, historic_data_file_path)

    def _store_historic_data(self, historic_data_json: str):
        """
        Stores then historical data in historic workspace location specified in the runtime config file.

        @param historic_data_json: The historic data (in JSON format) to be stored in persistent storage.
        """

        try:
            self._historic_workspace.mkdir(exist_ok=True, parents=True)
            with open(self._historic_data_file, "w") as historic_data_file:
                historic_data_file.write(historic_data_json)
        except EnvironmentError as e:
            logger.error(
                f"There was a problem the historic data file '{self._historic_data_file}': '{e}'.")

    def _retrieve_historic_data(self, config: dict, historic_workspace: str, historic_data_file_path: str):
        try:
            # Attempt to obtain the local persistent data location specified in the runtime config file
            self._historic_workspace = pathlib.Path(historic_workspace)
            self._historic_workspace = self._historic_workspace.joinpath(pathlib.Path(self._suites_string))
            historic_data_file = pathlib.Path(historic_data_file_path)
            
            # Attempt to unpack the local historic data file
            self._historic_data_file = self._historic_workspace.joinpath(
                historic_data_file)
            logger.info(
                f"Attempting to retrieve historic data at location '{self._historic_data_file}'...")
            if self._historic_data_file.is_file():
                with open(self._historic_data_file, "r") as historic_data_raw:
                    historic_data_json = historic_data_raw.read()
                    self._unpack_historic_data(historic_data_json)

        except KeyError as e:
            raise SystemError(f"The config does not contain the key {str(e)}.")
        except EnvironmentError as e:
            raise SystemError(
                f"There was a problem the historic data file '{self._historic_data_file}': '{e}'.")

    def _store_runtime_artifacts(self, runtime_artifact_dir : str):
        """
        Copy runtime artifacts from the provided directory to the historic storage directory.

        @param runtime_artifact_dir: Path to runtime artifacts to copy.
        """
        source_directory = pathlib.Path(runtime_artifact_dir)
        try:
            storage_directory = self._historic_workspace.joinpath(pathlib.Path(self.RUNTIME_ARTIFACT_DIRECTORY))
            storage_directory.mkdir(exist_ok=True, parents=True)

            self._copy_files_from_directory_to_destination(source_directory, storage_directory)
        except OSError as e:
            logger.error(f"Error copying runtime artifacts from {runtime_artifact_dir}. Error thrown: {e}")

    def _store_coverage_artifacts(self, runtime_coverage_dir : str):
        """
        Copy runtime coverage artifacts from the provided directory to the historic storage directory.

        @param runtime_coverage_dir: Path to the runtime coverage artifacts to copy.
        """
        source_directory = pathlib.Path(runtime_coverage_dir)
        try:
            storage_directory = self._historic_workspace.joinpath(pathlib.Path(self.RUNTIME_COVERAGE_DIRECTORY))
            storage_directory.mkdir(exist_ok=True, parents=True)

            self._copy_files_from_directory_to_destination(source_directory, storage_directory)
        except OSError as e:
            logger.error(f"Error copying coverage artifacts from {runtime_coverage_dir}. Error thrown: {e}")

    def _copy_files_from_directory_to_destination(self, source_directory: pathlib.Path, target_directory : pathlib.Path):
        """
        Copies all files in source directory to the target directory.

        @param source_directory: pathlib.Path to directory to copy files from.
        @param target_direcotry: pathlib.Path to directory to store files in.
        """
        try:
            shutil.copytree(source_directory, target_directory, dirs_exist_ok=True)
        except OSError as e:
                logger.error(f"Error copying tree '{source_directory}' to '{target_directory}'")
                logger.error(f"Error thrown: {e}")
