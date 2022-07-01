#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from tiaf_logger import get_logger

from abc import ABC, abstractmethod
logger = get_logger(__file__)


class StorageQueryTool(ABC):

    def __init__(self, **kwargs):
        """
        Initialise storage query tool with search parameters and flags  denoting whether to access or delete the resource
        @param kwargs: kwargs containing parsed arguments
        """
        self._root_directory = kwargs.get('root-directory')
        self._branch = kwargs.get('branch')
        self._build = kwargs.get('build')
        self._suite = kwargs.get('suite')
        self._access_flag = kwargs.get('access')
        self._delete_flag = kwargs.get('delete')
        self._update_flag = kwargs.get('update')
        self._create_flag = kwargs.get('create')

        if self._update_flag or self._create_flag:
            self._file_in = kwargs.get('file_in')
            self._file = self._get_file(self._file_in)

    def _get_file(self, file_address: str):
        """
        Get historic json file from specified file address
        @param file_address: Address to read json from.
        """
        try:
            with open(file_address, "r") as historic_data_raw:
                return historic_data_raw.read()
        except FileNotFoundError as e:
            logger.error(f"File not found at '{file_address}'. Exception:'{e}'.")
        
    @abstractmethod
    def _search(self):
        """
        Executes the search based on the search parameters initialised beforehand, in either the file directory or in the s3 bucket.
        """
        pass

    @abstractmethod
    def _write_tree(self):
        """
        Displays file tree to user in console output
        """
        pass

    @abstractmethod
    def _delete(self, file: str):
        """
        Deletes the specified file

        @param file: The file to be deleted
        """
        pass

    @abstractmethod
    def _access(self, file: str):
        """
        Accesses the specified file

        @param file: The file to be accessed
        """
        pass

