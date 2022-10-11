#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import json
from io import BytesIO
from pathlib import Path
from tiaf_logger import get_logger
from storage_query_tool import StorageQueryTool
import datetime

logger = get_logger(__file__)


class LocalStorageQueryTool(StorageQueryTool):
    """
    Local Storage oriented subclass of the Storage Query Tool, with Create, Read, Update and Delete functionality for dealing with historic.json.zip files in buckets

    """

    def __init__(self, **kwargs):
        """
        Initialise storage query tool with search parameters and access/delete parameters.
        """
        super().__init__(**kwargs)
        if self.has_full_address:
            if self._action == self.Action.CREATE:
                logger.info('Creating file')
                self._put(file=self._file,
                          storage_location=self._full_address)
            elif self._action == self.Action.READ:
                logger.info('Opening file')
                self._access(file=self._full_address)
            elif self._action == self.Action.UPDATE:
                logger.info('Updating File')
                self._update(file=self._file,
                             storage_location=self._full_address)
            elif self._action == self.Action.DELETE:
                logger.info("Deleting File")
                self._delete(file=self._full_address)
            elif self._action == self.Action.QUERY:
                self._display()
        else:
            self._display()

    def _display(self):
        """
        Executes the search based on the search parameters initialised beforehand, in either the file directory or in the s3 bucket.
        """

        if self.has_full_address:
            self._check_object_exists(self._full_address)
        else:
            if self._root_directory:
                l = Path.glob(Path(self._root_directory), "**/*.json")
                for obj in l:
                    logger.info(obj.__str__() + ", last modified at: " +
                                str(datetime.datetime.utcfromtimestamp(obj.stat().st_mtime)))
            else:
                raise SystemError(
                    "You must provide a root directory to search in or a full address when using local storage.")

    def _check_object_exists(self, key: str):
        """
        Method that returns whether an object exists in the specified location. It also writes into the console whether the file exists.

        @param key: address of the file to look for.
        """
        if Path(key).is_file():
            logger.info(f"File found at {key}")
            return True
        else:
            logger.info(f"File not found at {key}")
            return False

    def _delete(self, file: str):
        """
        Deletes the specified file.

        @param file: Path to the file to be deleted.
        """
        if self._check_object_exists(file):
            logger.info(
                f"Deleting file at address: {file}")
            Path.unlink(Path(file))

    def _access(self, file: str):
        """
        Accesses the specified file.

        @param file: Path to the file to be accessed.
        @param destination: Path to where file should be saved on local machine.
        """
        if self._check_object_exists(file):
            os.startfile(os.path.split(file)[0])

    def _put(self, file: str, storage_location: str):
        """
        Put the specified file in the specified location.

        @param file: File to store.
        @param storage_location: Location to store the file.
        """
        try:
            json_obj = json.loads(file)
            if not self._check_object_exists(str(storage_location)+"/historic_data.json"):
                with open(f"{storage_location}/historic_data.json", "w", encoding="UTF-8") as raw_output_file:
                    json.dump(json_obj, raw_output_file,
                              ensure_ascii=False, indent=4)
            else:
                logger.info("Cancelling create, as file exists already")
        except json.JSONDecodeError:
            logger.error("The historic data does not contain valid json")

    def _update(self, file: str, storage_location: str):
        """
        Replace the file in the specified location with the provided file if it exists.

        @param file: File to store.
        @param storage_location: Location to store the file.
        """
        try:
            json_obj = json.loads(file)
            if self._check_object_exists(storage_location):
                with open(f"{storage_location}", "w", encoding="UTF-8") as raw_output_file:
                    json.dump(json_obj, raw_output_file,
                              ensure_ascii=False, indent=4)
            else:
                logger.info("Cancelling update, as file does not exist")
        except json.JSONDecodeError:
            logger.error("The historic data does not contain valid json")
