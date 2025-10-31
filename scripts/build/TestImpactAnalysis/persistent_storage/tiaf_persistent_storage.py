#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import pathlib
from abc import ABC, abstractmethod
from tiaf_logger import get_logger

logger = get_logger(__file__)

# Abstraction for the persistent storage required by TIAF to store and retrieve the branch coverage data and other meta-data
class PersistentStorage(ABC):

    COMMON_CONFIG_KEY = "common"
    WORKSPACE_KEY = "workspace"
    HISTORIC_SEQUENCES_KEY = "historic_sequences"
    ACTIVE_KEY = "active"
    ROOT_KEY = "root"
    RELATIVE_PATHS_KEY = "relative_paths"
    TEST_IMPACT_DATA_FILE_KEY = "test_impact_data_file"
    PREVIOUS_TEST_RUN_DATA_FILE_KEY = "previous_test_run_data_file"
    LAST_COMMIT_HASH_KEY = "last_commit_hash"
    COVERAGE_DATA_KEY = "coverage_data"
    PREVIOUS_TEST_RUNS_KEY = "previous_test_runs"
    RUNTIME_ARTIFACT_DIRECTORY = "RuntimeArtifacts"
    RUNTIME_COVERAGE_DIRECTORY = "RuntimeCoverage"

    def __init__(self, config: dict, suites_string: str, commit: str, active_workspace: str, unpacked_coverage_data_file_path: str, previous_test_run_data_file_path: str, temp_workspace: str):
        """
        Initializes the persistent storage into a state for which there is no historic data available.

        @param config: The runtime configuration to obtain the data file paths from.
        @param suites_string: The unique key to differentiate the different suite combinations from one another different for which the historic data will be obtained for.
        @param commit: The commit hash for this build.
        """

        # Work on the assumption that there is no historic meta-data (a valid state to be in, should none exist)
        self._suites_string = suites_string
        self._last_commit_hash = None
        self._has_historic_data = False
        self._has_previous_last_commit_hash = False
        self._this_commit_hash = commit
        self._this_commit_hash_last_commit_hash = None
        self._historic_data = None
        logger.info(f"Attempting to access persistent storage for the commit '{self._this_commit_hash}' for suites '{self._suites_string}'")

        self._temp_workspace = pathlib.Path(temp_workspace)
        self._active_workspace = pathlib.Path(active_workspace).joinpath(pathlib.Path(self._suites_string))
        self._unpacked_coverage_data_file = self._active_workspace.joinpath(unpacked_coverage_data_file_path)
        self._previous_test_run_data_file = self._active_workspace.joinpath(previous_test_run_data_file_path)

        
    def _unpack_historic_data(self, historic_data_json: str):
        """
        Unpacks the historic data into the appropriate memory and disk locations.

        @param historic_data_json: The historic data in JSON format.
        """
        
        self._has_historic_data = False
        self._has_previous_last_commit_hash = False

        try:
            self._historic_data = json.loads(historic_data_json)

            # Last commit hash for this branch
            self._last_commit_hash = self._historic_data[self.LAST_COMMIT_HASH_KEY]
            logger.info(f"Last commit hash '{self._last_commit_hash}' found.")

            # Last commit hash for the sequence that was run for this commit previously (if any)
            if self.HISTORIC_SEQUENCES_KEY in self._historic_data:
                if self._this_commit_hash in self._historic_data[self.HISTORIC_SEQUENCES_KEY]:
                    # 'None' is a valid value for the previously used last commit hash if there was no coverage data at that time
                    self._this_commit_hash_last_commit_hash = self._historic_data[self.HISTORIC_SEQUENCES_KEY][self._this_commit_hash]
                    self._has_previous_last_commit_hash = self._this_commit_hash_last_commit_hash is not None

                    if self._has_previous_last_commit_hash:
                        logger.info(f"Last commit hash '{self._this_commit_hash_last_commit_hash}' was used previously for the commit '{self._last_commit_hash}'.")
                    else:
                        logger.info(f"Prior sequence data found for this commit but it is empty (there was no coverage data available at that time).")
                else:
                    logger.info(f"No prior sequence data found for commit '{self._this_commit_hash}', this is the first sequence for this commit.")
            else:
                logger.info(f"No prior sequence data found for any commits.")

            # Test runs for the previous sequence associated with the last commit hash
            previous_test_runs = self._historic_data.get(self.PREVIOUS_TEST_RUNS_KEY, None)
            if previous_test_runs:
                logger.info(f"Previous test run data for a sequence of '{len(previous_test_runs)}' test targets found.")
            else:
                self._historic_data[self.PREVIOUS_TEST_RUNS_KEY] = {}
                logger.info("No previous test run data found.")

            # Create the active workspace directory for the unpacked historic data files so they are accessible by the runtime
            self._active_workspace.mkdir(exist_ok=True, parents=True)

            # Coverage file
            logger.info(f"Writing coverage data to '{self._unpacked_coverage_data_file}'.")
            with open(self._unpacked_coverage_data_file, "w", newline='\n') as coverage_data:
                coverage_data.write(self._historic_data[self.COVERAGE_DATA_KEY])

            # Previous test runs file
            logger.info(f"Writing previous test runs data to '{self._previous_test_run_data_file}'.")
            with open(self._previous_test_run_data_file, "w", newline='\n') as previous_test_runs_data:
                previous_test_runs_json = json.dumps(self._historic_data[self.PREVIOUS_TEST_RUNS_KEY])
                previous_test_runs_data.write(previous_test_runs_json)

            self._has_historic_data = True
        except json.JSONDecodeError:
            logger.error("The historic data does not contain valid JSON.")
        except KeyError as e:
            logger.error(f"The historic data does not contain the key {str(e)}.")
        except EnvironmentError as e:
            logger.error(f"There was a problem the coverage data file '{self._unpacked_coverage_data_file}': '{e}'.")

    def _pack_historic_data(self, test_runs: list):
        """
        Packs the current historic data into a JSON file for serializing.

        @param test_runs: The test runs for the sequence that just completed.

        @return: The packed historic data in JSON format.
        """

        try:
            # Attempt to read the existing coverage data
            if self._unpacked_coverage_data_file.is_file():
                if not self._historic_data:
                    self._historic_data = {}

                # Last commit hash for this branch
                self._historic_data[self.LAST_COMMIT_HASH_KEY] = self._this_commit_hash

                # Last commit hash for this commit
                if not self.HISTORIC_SEQUENCES_KEY in self._historic_data:
                    self._historic_data[self.HISTORIC_SEQUENCES_KEY] = {}
                self._historic_data[self.HISTORIC_SEQUENCES_KEY][self._this_commit_hash] = self._last_commit_hash

                # Test runs for this completed sequence
                self._historic_data[self.PREVIOUS_TEST_RUNS_KEY] = test_runs

                # Coverage data for this branch
                with open(self._unpacked_coverage_data_file, "r") as coverage_data:
                    self._historic_data[self.COVERAGE_DATA_KEY] = coverage_data.read()
                    return json.dumps(self._historic_data)
            else:
                logger.info(f"No coverage data exists at location '{self._unpacked_coverage_data_file}'.")
        except EnvironmentError as e:
            logger.error(f"There was a problem the coverage data file '{self._unpacked_coverage_data_file}': '{e}'.")
        except TypeError:
            logger.error("The historic data could not be serialized to valid JSON.")
        
        return None

    @abstractmethod
    def _store_historic_data(self, historic_data_json: str):
        """
        Stores the historic data in the designated persistent storage location.

        @param historic_data_json: The historic data (in JSON format) to be stored in persistent storage.
        """
        pass

    def update_and_store_historic_data(self, test_runs: list):
        """
        Updates the historic data and stores it in the designated persistent storage location.

        @param test_runs: The test runs for the sequence that just completed.
        """

        historic_data_json = self._pack_historic_data(test_runs)
        if historic_data_json:
            logger.info(f"Attempting to store historic data with new last commit hash '{self._this_commit_hash}'...")
            self._store_historic_data(historic_data_json)
            logger.info("The historic data was successfully stored.")

        else:
            logger.info("The historic data could not be successfully stored.")
    
    def store_artifacts(self, runtime_artifact_dir, runtime_coverage_dir):
        """
        Store the runtime artifacts and runtime coverage artifacts stored in the specified directories.

        @param runtime_artifact_dir: The directory containing the runtime artifacts to store.
        @param runtime_coverage_dir: The directory contianing the runtime coverage artifacts to store.
        """
        self._store_runtime_artifacts(runtime_artifact_dir)
        self._store_coverage_artifacts(runtime_coverage_dir)

    @abstractmethod
    def _store_runtime_artifacts(self, runtime_artifact_dir):
        """
        Store the runtime artifacts in the designated persistent storage location.

        @param runtime_artifact_dir: The directory containing the runtime artifacts to store.
        """
        pass

    @abstractmethod
    def _store_coverage_artifacts(self, runtime_coverage_dir):
        """
        Store the coverage artifacts in the designated persistent storage location.

        @param runtime_coverage_dir: The directory contianing the runtime coverage artifacts to store.
        """
        pass
    
    @property
    def has_historic_data(self):
        """
        Flag denoting that persistent storage was able to find relevant historic data for the requested branch.
        """
        return self._has_historic_data

    @property
    def last_commit_hash(self):
        """
        Hash of the last commit we processed, ingested from our historic data.
        """
        return self._last_commit_hash

    @property
    def is_last_commit_hash_equal_to_this_commit_hash(self):
        """
        Is the current commit that we are running TIAF on the same as the last commit we have in our historic data.
        This means that this is a repeat sequence.
        """
        return self._last_commit_hash == self._this_commit_hash

    @property
    def this_commit_last_commit_hash(self):
        """
        Hash of this commit. Is none if this commit hash was not found in our historic data.
        """
        return self._this_commit_hash_last_commit_hash

    @property
    def has_previous_last_commit_hash(self):
        """
        If the hash of the last commit was found in our historic data, then this flag will be set.
        """
        return self._has_previous_last_commit_hash

