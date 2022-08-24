#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from enum import Enum
from tiaf_logger import get_logger
from abc import ABC, abstractmethod
logger = get_logger(__file__)


class StorageQueryTool(ABC):

    class Action(Enum):
        CREATE = "create"
        READ = "read"
        UPDATE = "update"
        DELETE = "delete"
        QUERY = None

    class FileType(Enum):
        JSON = "json"
        ZIP = "zip"

    def __init__(self, **kwargs):
        """
        Initialise storage query tool with search parameters and flags  denoting whether to access or delete the resource.

        @param kwargs: kwargs containing parsed arguments.
        """
        self._root_directory = kwargs.get('search_in')
        self._action = self.Action(kwargs.get('action'))
        self._file_in = kwargs.get('file_in')
        self._file_out = kwargs.get('file_out')
        self._full_address = kwargs.get('full_address')
        self._file_type = self.FileType(kwargs.get('file_type'))

        if self._action == self.Action.CREATE or self._action == self.Action.UPDATE:
            if self._file_type == self.FileType.JSON:
                self._file = self._get_json_file(self._file_in)
            if self._file_type == self.FileType.ZIP:
                self._file = self._get_zip_file(self._file_in)

    def _get_zip_file(self, file_address: str):
        """
        Get zip file from the specified file address.

        @param file_address: Address to read zip from
        """
        try:
            with open(file_address, "rb") as zip_file:
                return zip_file.read()
        except FileNotFoundError as e:
            logger.error(
                f"File not found at '{file_address}'. Exception:'{e}'.")

    def _get_json_file(self, file_address: str):
        """
        Get historic json file from specified file address.

        @param file_address: Address to read json from.
        """
        try:
            with open(file_address, "r") as historic_data_raw:
                return historic_data_raw.read()
        except FileNotFoundError as e:
            logger.error(
                f"File not found at '{file_address}'. Exception:'{e}'.")

    @abstractmethod
    def _display(self):
        """
        Executes the search based on the search parameters initialised beforehand, in either the file directory or in the s3 bucket.
        """
        pass

    @abstractmethod
    def _delete(self, file: str):
        """
        Deletes the specified file.

        @param file: The file to be deleted.
        """
        pass

    @abstractmethod
    def _access(self, file: str):
        """
        Accesses the specified file.

        @param file: The file to be accessed.
        """
        pass

    @abstractmethod
    def _put(self, file: str, location: str):
        """
        Put the specified file in the specified location.

        @param file: File in json format to store.
        @param location: Location to store the file.
        """
        pass

    @abstractmethod
    def _update(self, file: str, location: str):
        """
        Replace the file in the specified location with the provided filed if it exists.

        @param file: File in json format to store.
        @param location: Location to store the file.
        """
        pass

    @property
    def has_full_address(self):
        return self._full_address
