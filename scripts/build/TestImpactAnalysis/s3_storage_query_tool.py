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
import botocore.exceptions
logger = get_logger(__file__)


class S3StorageQueryTool(StorageQueryTool):

    def __init__(self, **kwargs):
        """
        Initialise storage query tool with search parameters and access/delete parameters
        """
        super().__init__(**kwargs)
        self._bucket_name = kwargs['bucket_name']
        
        try:

            self._s3 = boto3.resource("s3")
            self._bucket = self._s3.Bucket(self._bucket_name)

            if self._read:
                self._access()
            elif self._create_flag:
                self._put(self._file, self._location)
            elif self._update_flag:
                self._update(self._file, self._location)
            elif self._delete_flag:
                self._delete("")
            else:
                self._display()
        except botocore.exceptions.BotoCoreError as e:
            raise SystemError(f"There was a problem with the s3 bucket: {e}")
        except botocore.exceptions.ClientError as e:
            raise SystemError(f"There was a problem with the s3 client: {e}")
        except Exception as e:
            logger.info(e)            
        


    def _display(self):
        """
        Executes the search based on the search parameters initialised beforehand, in either the file directory or in the s3 bucket.
        """

        def filter_by(filter_by, bucket_objs):
            prefix_string = filter_by + "/"
            result = bucket_objs.filter(Prefix=prefix_string)
            return result

        bucket_objs = self._bucket.objects.all()

        if self._root_directory:
            bucket_objs = filter_by(self._root_directory, bucket_objs)
        elif self._branch and self._build and self._suite:
            bucket_objs = filter_by(self._root_directory+"/"+self._branch, bucket_objs)

        for object in bucket_objs:
            logger.info(object.key)

    

    def _write_tree(self):
        """
        Displays file tree to user in console output
        """
        pass

    def _delete(self, file: str):
        """
        Deletes the specified file

        @param file: The file to be deleted
        """
        pass

    def _access(self, file: str):
        """
        Accesses the specified file

        @param file: The file to be accessed
        """
        pass

    def _put(self, file: str, location: str):
        """
        Put the specified file in the specified location
        @param file: File in json format to store
        @param location: Location to store the file
        """
        pass

    def _update(self, file: str, location: str):
        """
        Replace the file in the specified location with the provided filed if it exists
        @param file: File in json format to store
        @param location: Location to store the file
        """
        pass

