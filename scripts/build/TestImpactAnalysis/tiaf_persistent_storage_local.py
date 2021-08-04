#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import json
from tiaf_persistent_storage import PersistentStorageNull

class PersistentStorageLocal(PersistentStorageNull):
    def __init__(self, config, suite):
        super().__init__(config, suite)
        self.__historic_workspace = config["workspace"]["historic"]["root"]
        historic_data_file = config["workspace"]["historic"]["relative_paths"]["data"]
        self.__historic_data_file = os.path.join(self.__historic_workspace, historic_data_file)
        if os.path.isfile(self.__historic_data_file):
            with open(self.__historic_data_file, "r") as historic_data_raw:
                historic_data_json = historic_data_raw.read()
                self._unpack_historic_data(historic_data_json)

    def _update_historic_data(self, historic_data_json):
        os.makedirs(self.__historic_workspace, exist_ok=True)
        with open(self.__historic_data_file, "w") as historic_data_file:
            historic_data_file.write(historic_data_json)
        