#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import boto3
import botocore.exceptions

from io import BytesIO
from tiaf_persistent_storage import PersistentStorage
import zlib

# Implementation of s3 bucket persistent storage
class PersistentStorageS3(PersistentStorage):
    def __init__(self, config: dict, suite: str, s3_bucket: str, branch: str):
        """
        Initializes the persistent storage with the specified s3 bucket.

        @param config:    The runtime config file to obtain the data file paths from.
        @param suite:     The test suite for which the historic data will be obtained for.
        @param s3_bucket: The s3 bucket to use for storing nd retrieving historic data.
        """

        super().__init__(config, suite)

        try:
            # We store the historic data as compressed JSON
            object_extension = "json.zip"

            # historic_data.json.zip is the file containing the coverage and meta-data of the last TIAF sequence run
            historic_data_file = f"historic_data.{object_extension}"

            # The location of the data is in the form <branch>/<config> so the build config of each branch gets its own historic data
            self._dir = f'{branch}/{config["meta"]["build_config"]}'
            self._historic_data_key = f'{self._dir}/{historic_data_file}'
            
            print(f"Attempting to retrieve historic data for branch '{branch}' at location '{self._historic_data_key}' on bucket '{s3_bucket}'...")
            self._s3 = boto3.resource("s3")
            self._bucket = self._s3.Bucket(s3_bucket)

            # There is only one historic_data.json.zip in the specified location
            for object in self._bucket.objects.filter(Prefix=self._historic_data_key):
                print(f"Historic data found for branch '{branch}'.")

                # Archive the existing object with the name of the existing last commit hash
                archive_key = f"{self._dir}/archive/{self._last_commit_hash}.{object_extension}"
                print(f"Archiving existing historic data to {archive_key}...")
                self._bucket.copy({"Bucket": self._bucket.name, "Key": self._historic_data_key}, archive_key)

                # Decode the historic data object into raw bytes
                response = object.get()
                file_stream = response['Body']

                # Decompress and unpack the zipped historic data JSON
                historic_data_json = zlib.decompress(file_stream.read()).decode('UTF-8')
                self._unpack_historic_data(historic_data_json)

                return
        except KeyError as e:
            print(f"The config does not contain the key {str(e)}.")
        except botocore.exceptions.BotoCoreError as e:
            print(f"There was a problem with the s3 bucket: {e}")

    def _store_historic_data(self, historic_data_json: str):
        """
        Stores then historical data in specified s3 bucket at the location <branch>/<build_config>/historical_data.json.zip.

        @param historic_data_json: The historic data (in JSON format) to be stored in persistent storage.
        """

        try:
            data = BytesIO(zlib.compress(bytes(historic_data_json, "UTF-8")))
            print(f"Uploading historic data to location '{self._historic_data_key}'...")
            self._bucket.upload_fileobj(data, self._historic_data_key)
            print("Upload complete.")
        except botocore.exceptions.BotoCoreError as e:
            print(f"There was a problem with the s3 bucket: {e}")
        