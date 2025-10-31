#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import zlib
import json
from io import BytesIO
import boto3
import botocore.exceptions
from tiaf_logger import get_logger
from storage_query_tool import StorageQueryTool

logger = get_logger(__file__)


class S3StorageQueryTool(StorageQueryTool):
    """
    S3 oriented subclass of the Storage Query Tool, with Create, Read, Update and Delete functionality for dealing with historic.json.zip files in buckets

    """

    def __init__(self, **kwargs):
        """
        Initialise storage query tool with search parameters and access/delete parameters.
        """
        super().__init__(**kwargs)
        self._bucket_name = kwargs['s3_bucket']

        try:

            self._s3_client = boto3.client('s3')
            self._s3 = boto3.resource('s3')
            if self.has_full_address:
                if self._action == self.Action.CREATE:
                    logger.info('create')
                    self._put(bucket_name=self._bucket_name,
                              file=self._file, storage_location=self._full_address)
                elif self._action == self.Action.READ:
                    logger.info('read')
                    self._access(bucket_name=self._bucket_name,
                                 file=self._full_address, destination=self._file_out)
                elif self._action == self.Action.UPDATE:
                    logger.info('update')
                    self._update(bucket_name=self._bucket_name,
                                 file=self._file, storage_location=self._full_address)
                elif self._action == self.Action.DELETE:
                    logger.info("update")
                    self._delete(bucket_name=self._bucket_name,
                                 file=self._full_address)
                elif self._action == self.Action.QUERY:
                    self._display()
            else:
                self._display()
        except botocore.exceptions.BotoCoreError as e:
            raise SystemError(f"There was a problem with the s3 bucket: {e}")
        except botocore.exceptions.ClientError as e:
            raise SystemError(f"There was a problem with the s3 client: {e}")

    def _display(self):
        """
        Executes the search based on the search parameters initialised beforehand, in either the file directory or in the s3 bucket.
        """

        def filter_by(filter_by, bucket_objs):
            prefix_string = filter_by + "/"
            result = bucket_objs.filter(Prefix=prefix_string)
            return result

        bucket_objs = self._s3.Bucket(self._bucket_name).objects.all()

        if self.has_full_address:
            self._check_object_exists(self._bucket_name, self._full_address)

        else:
            if self._root_directory:
                bucket_objs = filter_by(self._root_directory, bucket_objs)
            for obj in bucket_objs:
                logger.info(obj.key)

    def _check_object_exists(self, bucket: str, key: str):
        """
        Method that returns whether an object exists in the specified bucket. It also writes into the console whether the file exists.

        @param bucket: name of the bucket to check.
        @param key: key of the object to look for.
        """
        try:
            self._s3.Object(bucket, key).load()
            logger.info(f"File found at {key} in bucket: {bucket}")
            return True
        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] == "404":
                logger.info("File not found!")
            return False

    def _delete(self, bucket_name: str, file: str):
        """
        Deletes the specified file.

        @param bucket_name: Bucket to delete file from.
        @param file: The file to be deleted.
        """
        if self._check_object_exists(bucket_name, file):
            logger.info(
                f"Deleting file in bucket: {bucket_name} with key {file}")
            self._s3.Object(bucket_name, file).delete()

    def _access(self, bucket_name: str, file: str, destination: str):
        """
        Accesses the specified file.

        @param bucket_name: Bucket to access file in.
        @param file: The file to be accessed.
        @param destination: Path to where file should be saved on local machine.
        """
        if self._check_object_exists(bucket_name, file):
            try:
                logger.info(
                    f"Downloading file in bucket: {bucket_name} with key {file}")
                file_stream = self._s3_client.get_object(
                    Bucket=bucket_name, Key=file)['Body']
            except botocore.exceptions.ClientError as e:
                logger.error(f"Failed to download file, error: {e}")
                raise e
            if self._file_type == self.FileType.JSON:
                self._save_as_json_file(file_stream, destination)
            elif self._file_type == self.FileType.ZIP:
                self._save_as_zip_file(file_stream, destination)
            else:
                raise SystemError(
                    "File type not specified or otherwise not passed through to SQT")

    def _save_as_zip_file(self, file_stream, destination: str):
        """
        Saves the provided file_stream as a zip file named zippedartifacts.zip in the destination folder.

        @param file_stream: File stream retrieved from S3 object, to be written out as a zip file.
        @param destination: Directory to write zip file to.
        """
        try:
            with open(f"{destination}zippedartifacts.zip", "wb") as output_file:
                output_file.writelines(file_stream.iter_lines())
        except OSError as e:
            logger.error("Error occurred when writing artifacts to file.")

    def _save_as_json_file(self, file_stream, destination: str):
        """
        Saves the provided file_stream as a json file named historic_data.json in the destination folder.

        @param file_stream: File stream retrieved from S3 object, to be written out as a json file.
        @param destination: Directory to write json file to.
        """
        try:
            decoded_data = zlib.decompress(
                file_stream.read()).decode('UTF-8')
            logger.info(f"Decoding complete.")
            json_obj = json.loads(decoded_data)
            with open(f"{destination}historic_data.json", "w", encoding="UTF-8") as raw_output_file:
                json.dump(json_obj, raw_output_file,
                          ensure_ascii=False, indent=4)
            logger.info(
                f"Data sucessfully saved in: {destination}historic_data.json")
        except json.JSONDecodeError:
            logger.error("The historic data does not contain valid JSON.")

    def _handle_file_writing(self, bucket_name, file, storage_location: str):
        """
        Writes the provided file to the storage location in the provided bucket. Storage format is decided by this objects file_type property.

        @param bucket-name: Bucket to store data in.
        @param file: File to store in bucket.
        @param storage_location: Location to store the file.
        """
        if self._file_type == self.FileType.JSON:
            data = BytesIO(zlib.compress(bytes(file, "UTF-8")))
        if self._file_type == self.FileType.ZIP:
            data = file
        logger.info(
            f"Uploading data to: {storage_location} in bucket: {bucket_name}")
        self._s3_client.put_object(
            Bucket=bucket_name, Key=storage_location, Body=data)
        logger.info(f"Upload complete")

    def _put(self, bucket_name: str, file: str, storage_location: str):
        """
        Put the specified file in the specified location.

        @param bucket-name: Bucket to store data in.
        @param file: File to store in bucket.
        @param storage_location: Location to store the file.
        """
        if not self._check_object_exists(bucket_name, storage_location):
            self._handle_file_writing(bucket_name, file, storage_location)
        else:
            logger.info("Cancelling create, as file exists already")

    def _update(self, bucket_name: str, file: str, storage_location: str):
        """
        Replace the file in the specified location with the provided file if it exists.

        @param bucket-name: Bucket to store data in.
        @param file: File to store in bucket.
        @param storage_location: Location to store the file.
        """
        if self._check_object_exists(bucket_name, storage_location):
            self._handle_file_writing(bucket_name, file, storage_location)
        else:
            logger.info("Cancelling update, as file does not already exist")
