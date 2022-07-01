#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from tiaf_logger import get_logger
from storage_query_tool import StorageQueryTool
import boto3
logger = get_logger(__file__)


class LocalStorageQueryTool(StorageQueryTool):

    def __init__(self, **kwargs):
        """
        Initialise storage query tool with search parameters and access/delete parameters.
        """
        super().__init__(**kwargs)
        self._search_in = kwargs.get('search_in')
        self._search()

        if self._access_flag:
            self._access()

        if self._delete_flag:
            self._delete()

    def _search(self):
        """
        Executes the search based on the search parameters initialised beforehand, in either the file directory or in the s3 bucket.
        """

        pass

    def _write_tree(self):
        """
        Displays file tree to user in console output.
        """
        pass

    def _delete(self, file: str):
        """
        Deletes the specified file.

        @param file: The file to be deleted.
        """
        pass

    def _access(self, file: str):
        """
        Accesses the specified file.

        @param file: The file to be accessed.
        """
        pass
