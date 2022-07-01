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

        bucket_objs =self._s3.Bucket(self._bucket_name).objects.all()

        if self.has_full_address:
            self._check_object_exists(self._bucket_name, self._full_address)
            
        else:
            bucket_objs = filter_by(self._root_directory, bucket_objs)
            for object in bucket_objs:
                logger.info(object.key)

    def _check_object_exists(self, bucket, key):
        try:
            self._s3.Object(bucket, key).load()
            logger.info(f"File found at {key} in bucket: {bucket}")
            return True
        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] == "404":
                logger.info("File not found!")
            return False

    def _write_tree(self):
        """
        Displays file tree to user in console output
        """
        pass

    def _delete(self, bucket_name:str, file: str):
        """
        Deletes the specified file
        @param bucket_name: Bucket to delete file from
        @param file: The file to be deleted
        """
        if self._check_object_exists(bucket_name, file):
            logger.info(f"Deleting file in bucket: {bucket_name} with key {file}")
            self._s3.Object(bucket_name, file).delete()

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

