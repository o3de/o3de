#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import pathlib
import logging
from abc import ABC, abstractmethod

logger = logging.getLogger()
logging.basicConfig()

# Abstraction for the persistent storage required by TIAF to store and retrieve the branch coverage data and other meta-data
class PersistentStorage(ABC):
    def __init__(self, config: dict, suite: str):
        """
        Initializes the persistent storage into a state for which there is no historic data available.

        @param config: The runtime configuration to obtain the data file paths from.
        @param suite:  The test suite for which the historic data will be obtained for.
        """

        # Work on the assumption that there is no historic meta-data (a valid state to be in, should none exist)
        self._last_commit_hash = None
        self._has_historic_data = False
        self._unpacked_coverage_data_file = None

        try:
            # The runtime expects the coverage data to be in the location specified in the config file (unless overridden with 
            # the --datafile command line argument, which the TIAF scripts do not do)
            self._active_workspace = config["workspace"]["active"]["root"]
            unpacked_coverage_data_file = config["workspace"]["active"]["relative_paths"]["test_impact_data_files"][suite]
        except KeyError as e:
            raise SystemError(f"The config does not contain the key {str(e)}.")

        self._unpacked_coverage_data_file = pathlib.PurePath(self._active_workspace).joinpath(unpacked_coverage_data_file)
        if not pathlib.Path.is_file(self._unpacked_coverage_data_file):
            logging.error(f"The coverage data file '{self._unpacked_coverage_data_file}' is not a valid file path.")
            self._unpacked_coverage_data_file = None
        
    def _unpack_historic_data(self, historic_data_json: str):
        """
        Unpacks the historic data into the appropriate memory and disk locations.

        @param historic_data_json: The historic data in JSON format.
        """
        
        self._has_historic_data = False
        if self._unpacked_coverage_data_file is None:
            logger.error("Cannot unpack historic data, the unpacked coverage data file path is invalid")
            return

        try:
            historic_data = json.loads(historic_data_json)
            self._last_commit_hash = historic_data["last_commit_hash"]

            # Create the active workspace directory where the coverage data file will be placed and unpack the coverage data so 
            # it is accessible by the runtime
            pathlib.Path.mkdir(self._active_workspace, exist_ok=True)
            with open(self._unpacked_coverage_data_file, "w", newline='\n') as coverage_data:
                coverage_data.write(historic_data["coverage_data"])

            self._has_historic_data = True
        except json.JSONDecodeError:
            logger.error("The historic data does not contain valid JSON.")
        except KeyError as e:
            logger.error(f"The historic data does not contain the key {str(e)}.")
        except EnvironmentError as e:
            logger.error(f"There was a problem the coverage data file '{self._unpacked_coverage_data_file}': '{e}'.")

    def _pack_historic_data(self, last_commit_hash: str):
        """
        Packs the current historic data into a JSON file for serializing.

        @param last_commit_hash: The commit hash to associate the coverage data (and any other meta data) with.
        @return:                 The packed historic data in JSON format.
        """

        if self._unpacked_coverage_data_file is None:
            logger.error("Cannot pack historic data, the unpacked coverage data file path is invalid")
            return

        try:
            # Attempt to read the existing coverage data
            if pathlib.Path.is_file(self._unpacked_coverage_data_file):
                with open(self._unpacked_coverage_data_file, "r") as coverage_data:
                    historic_data = {"last_commit_hash": last_commit_hash, "coverage_data": coverage_data.read()}
                    return json.dumps(historic_data)
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

    def update_and_store_historic_data(self, last_commit_hash: str):
        """
        Updates the historic data and stores it in the designated persistent storage location.

        @param last_commit_hash: The commit hash to associate the coverage data (and any other meta data) with.
        """

        historic_data_json = self._pack_historic_data(last_commit_hash)
        if historic_data_json is not None:
            self._store_historic_data(historic_data_json)
        else:
            logger.info("The historic data could not be successfully stored.")

    @property
    def has_historic_data(self):
        return self._has_historic_data

    @property
    def last_commit_hash(self):
        return self._last_commit_hash