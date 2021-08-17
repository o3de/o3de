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

    WORKSPACE_KEY = "workspace"
    HISTORIC_SEQUENCES_KEY = "historic_sequences"
    ACTIVE_KEY = "active"
    ROOT_KEY = "root"
    RELATIVE_PATHS_KEY = "relative_paths"
    TEST_IMPACT_DATA_FILE_KEY = "test_impact_data_file"
    LAST_COMMIT_HASH_KEY = "last_commit_hash"
    COVERAGE_DATA_KEY = "coverage_data"

    def __init__(self, config: dict, suite: str, commit: str):
        """
        Initializes the persistent storage into a state for which there is no historic data available.

        @param config: The runtime configuration to obtain the data file paths from.
        @param suite:  The test suite for which the historic data will be obtained for.
        @param commit: The commit hash for this build.
        """

        # Work on the assumption that there is no historic meta-data (a valid state to be in, should none exist)
        self._suite = suite
        self._last_commit_hash = None
        self._has_historic_data = False
        self._has_previous_last_commit_hash = False
        self._this_commit_hash = commit
        self._this_commit_hash_last_commit_hash = None
        self._historic_data = None
        logger.info(f"Attempting to access persistent storage for the commit '{self._this_commit_hash}' for suite '{self._suite}'")

        try:
            # The runtime expects the coverage data to be in the location specified in the config file (unless overridden with 
            # the --datafile command line argument, which the TIAF scripts do not do)
            self._active_workspace = pathlib.Path(config[self.WORKSPACE_KEY][self.ACTIVE_KEY][self.ROOT_KEY])
            self._active_workspace = self._active_workspace.joinpath(pathlib.Path(self._suite))
            unpacked_coverage_data_file = config[self.WORKSPACE_KEY][self.ACTIVE_KEY][self.RELATIVE_PATHS_KEY][self.TEST_IMPACT_DATA_FILE_KEY]
        except KeyError as e:
            raise SystemError(f"The config does not contain the key {str(e)}.")

        self._unpacked_coverage_data_file = self._active_workspace.joinpath(unpacked_coverage_data_file)
        
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
                        logger.info(f"Last commit hash '{self._this_commit_hash_last_commit_hash}' was used previously for this commit.")
                    else:
                        logger.info(f"Prior sequence data found for this commit but it is empty (there was no coverage data available at that time).")
                else:
                    logger.info(f"No prior sequence data found for commit '{self._this_commit_hash}', this is the first sequence for this commit.")
            else:
                logger.info(f"No prior sequence data found for any commits.")

            # Create the active workspace directory for the unpacked historic data files so they are accessible by the runtime
            self._active_workspace.mkdir(exist_ok=True)

            # Coverage file
            logger.info(f"Writing coverage data to '{self._unpacked_coverage_data_file}'.")
            with open(self._unpacked_coverage_data_file, "w", newline='\n') as coverage_data:
                coverage_data.write(self._historic_data[self.COVERAGE_DATA_KEY])

            self._has_historic_data = True
        except json.JSONDecodeError:
            logger.error("The historic data does not contain valid JSON.")
        except KeyError as e:
            logger.error(f"The historic data does not contain the key {str(e)}.")
        except EnvironmentError as e:
            logger.error(f"There was a problem the coverage data file '{self._unpacked_coverage_data_file}': '{e}'.")

    def _pack_historic_data(self):
        """
        Packs the current historic data into a JSON file for serializing.

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

    def update_and_store_historic_data(self):
        """
        Updates the historic data and stores it in the designated persistent storage location.
        """

        historic_data_json = self._pack_historic_data()
        if historic_data_json:
            logger.info(f"Attempting to store historic data with new last commit hash '{self._this_commit_hash}'...")
            self._store_historic_data(historic_data_json)
            logger.info("The historic data was successfully stored.")

        else:
            logger.info("The historic data could not be successfully stored.")

    @property
    def has_historic_data(self):
        return self._has_historic_data

    @property
    def last_commit_hash(self):
        return self._last_commit_hash

    @property
    def is_repeat_sequence(self):
        return self._last_commit_hash == self._this_commit_hash

    @property
    def this_commit_last_commit_hash(self):
        return self._this_commit_hash_last_commit_hash

    @property
    def can_rerun_sequence(self):
        return self._has_previous_last_commit_hash