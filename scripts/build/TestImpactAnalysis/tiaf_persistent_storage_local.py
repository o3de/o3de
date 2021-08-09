#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import pathlib
import logging
from tiaf_persistent_storage import PersistentStorage

logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)

# Implementation of local persistent storage
class PersistentStorageLocal(PersistentStorage):
    def __init__(self, config: str, suite: str):
        """
        Initializes the persistent storage with any local historic data available.

        @param config: The runtime config file to obtain the data file paths from.
        @param suite:  The test suite for which the historic data will be obtained for.
        """

        super().__init__(config, suite)
        try:
            # Attempt to obtain the local persistent data location specified in the runtime config file
            self._historic_workspace = pathlib.Path(config["workspace"]["historic"]["root"])
            historic_data_file = pathlib.Path(config["workspace"]["historic"]["relative_paths"]["data"])
            
            # Attempt to unpack the local historic data file
            self._historic_data_file = self._historic_workspace.joinpath(historic_data_file)
            if self._historic_data_file.is_file():
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

        try:
            self._historic_workspace.mkdir(exist_ok=True)
            with open(self._historic_data_file, "w") as historic_data_file:
                historic_data_file.write(historic_data_json)
        except EnvironmentError as e:
            logger.error(f"There was a problem the historic data file '{self._historic_data_file}': '{e}'.")