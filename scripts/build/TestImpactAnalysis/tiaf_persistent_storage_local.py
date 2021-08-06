#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from pathlib import Path
from pathlib import PurePath
import logging
from tiaf_persistent_storage import PersistentStorage

logger = logging.getLogger()
logging.basicConfig()

# Implementation of local persistent storage
class PersistentStorageLocal(PersistentStorage):
    def __init__(self, config: str, suite: str):
        """
        Initializes the persistent storage with any local historic data available.

        @param config: The runtime config file to obtain the data file paths from.
        @param suite:  The test suite for which the historic data will be obtained for.
        """

        self._historic_data_file = None

        super().__init__(config, suite)
        try:
            # Attempt to obtain the local persistent data location specified in the runtime config file
            self._historic_workspace = config["workspace"]["historic"]["root"]
            historic_data_file = config["workspace"]["historic"]["relative_paths"]["data"]
            
            # Attempt to unpack the local historic data file
            self._historic_data_file = PurePath(self._historic_workspace).joinpath(historic_data_file)
            if not Path.is_file(self._historic_data_file):
                logger.error(f"The historic data file '{self._historic_data_file}' is not a valid file path.")
                self._historic_data_file = None
            else:
                with open(self._historic_data_file, "r") as historic_data_raw:
                    historic_data_json = historic_data_raw.read()
                    self._unpack_historic_data(historic_data_json)

        except KeyError as e:
            raise SystemError(f"The config does not contain the key {str(e)}.")
        except EnvironmentError as e:
            raise SystemError(f"There was a problem the historic data file '{self._historic_data_file}': '{e}'.")

    def _store_historic_data(self, historic_data_json: str):
        """
        Stores then historical data in historic workspace location specified in the runtime config file.

        @param historic_data_json: The historic data (in JSON format) to be stored in persistent storage.
        """

        if self._historic_data_file is None:
            logger.error("Cannot store the historic data, the historic data file path is invalid")
            return

        try:
            Path.mkdir(self._historic_workspace, exist_ok=True)
            with open(self._historic_data_file, "w") as historic_data_file:
                historic_data_file.write(historic_data_json)
        except EnvironmentError as e:
            logger.error(f"There was a problem the historic data file '{self._historic_data_file}': '{e}'.")