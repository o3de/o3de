#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import json
from abc import ABC, abstractmethod

class PersistentStorageNull(ABC):
    def __init__(self, config, suite):
        self._active_workspace = config["workspace"]["active"]["root"]
        unpacked_coverage_data_file = config["workspace"]["active"]["relative_paths"]["test_impact_data_files"][suite]
        self._unpacked_coverage_data_file = os.path.join(self._active_workspace, unpacked_coverage_data_file)
        self._last_commit_hash = None
        self._has_historic_data = None

    def _unpack_historic_data(self, historic_data_json):
        historic_data = json.loads(historic_data_json)
        self._last_commit_hash = historic_data["last_commit_hash"]
        os.makedirs(self._active_workspace, exist_ok=True)
        f = open(self._unpacked_coverage_data_file, "w", newline='\n')
        f.write(historic_data["coverage_data"])
        f.close()
        self._has_historic_data = True

    def _pack_historic_data(self, last_commit_hash):
        f = open(self._unpacked_coverage_data_file, "r")
        coverage_data = f.read()
        historic_data = {"last_commit_hash": last_commit_hash, "coverage_data": coverage_data}
        return json.dumps(historic_data)

    @abstractmethod
    def _update_historic_data(self, historic_data_json):
        pass

    #def return_historic_data(self):


    def update_historic_data(self, last_commit_hash):
        historical_data_json = self._pack_historic_data(last_commit_hash)
        self._update_historic_data(historical_data_json)

    @property
    def has_historic_data(self):
        return self._has_historic_data

    @property
    def last_commit_hash(self):
        return self._last_commit_hash