#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import boto3

from io import BytesIO
from tiaf_persistent_storage import PersistentStorage
import zlib
from base64 import b64encode, b64decode

class PersistentStorageS3(PersistentStorage):
    def __init__(self, config, suite, s3_bucket, branch):
        super().__init__(config, suite)
        object_extension = "json.zip"
        historic_data_file = f"historic_data.{object_extension}"
        self._dir = f'{branch}/{config["meta"]["build_config"]}'
        self._historical_data_key = f'{self._dir}/{historic_data_file}'
        
        print(f"Attempting to retrieve historical data for branch '{branch}' at location '{self._historical_data_key}' on bucket '{s3_bucket}'...")
        self._s3 = boto3.resource("s3")
        self._bucket = self._s3.Bucket(s3_bucket)
        for object in self._bucket.objects.filter(Prefix=self._historical_data_key):
            print(f"Historical data found for branch '{branch}'.")
            response = object.get()
            file_stream = response['Body']
            historic_data_json = zlib.decompress(file_stream.read()).decode('UTF-8')
            self._unpack_historic_data(historic_data_json)
            # Backup the existing object with the name of the existing last commit hash
            archive_key = f"{self._dir}/archive/{self._last_commit_hash}.{object_extension}"
            print(f"Archiving existing historical data to {archive_key}...")
            self._bucket.copy({"Bucket": self._bucket.name, "Key": self._historical_data_key}, archive_key)
            return

    def _update_historic_data(self, historic_data_json):
        data = BytesIO(zlib.compress(bytes(historic_data_json, "UTF-8")))
        print(f"Uploading historical data to location '{self._historical_data_key}'...")
        self._bucket.upload_fileobj(data, self._historical_data_key)
        print("Upload complete.")
        