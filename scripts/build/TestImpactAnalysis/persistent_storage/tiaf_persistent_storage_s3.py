#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import shutil
import tempfile
import boto3
import botocore.exceptions
import zlib
import pathlib
from io import BytesIO
from persistent_storage import PersistentStorage
from tiaf_logger import get_logger

logger = get_logger(__file__)

# Implementation of s3 bucket persistent storage


class PersistentStorageS3(PersistentStorage):

    META_KEY = "meta"
    BUILD_CONFIG_KEY = "build_config"

    def __init__(self, config: dict, suites_string: str, commit: str, s3_bucket: str, root_dir: str, branch: str, active_workspace: str, unpacked_coverage_data_file_path: str, previous_test_run_data_file_path: str, temp_workspace: str):
        """
        Initializes the persistent storage with the specified s3 bucket.

        @param config:        The runtime config file to obtain the data file paths from.
        @param suites_string: The concatenated test suites string for which the historic data will be obtained for.
        @param commit:        The commit hash for this build.
        @param s3_bucket:     The s3 bucket to use for storing nd retrieving historic data.
        @param root_dir:      The root directory to use for the historic data object.
        @branch branch:       The branch to retrieve the historic data for.
        """

        super().__init__(config, suites_string, commit, active_workspace, unpacked_coverage_data_file_path, previous_test_run_data_file_path, temp_workspace)

        self.s3_bucket = s3_bucket
        self.root_dir = root_dir
        self.branch = branch
        self._s3 = boto3.client('s3')
        self._retrieve_historic_data(config)

    def _store_historic_data(self, historic_data_json: str):
        """
        Stores then historical data in specified s3 bucket at the location <branch>/<build_config>/historical_data.json.zip.

        @param historic_data_json: The historic data (in JSON format) to be stored in persistent storage.
        """

        try:
            data = BytesIO(zlib.compress(bytes(historic_data_json, "UTF-8")))
            logger.info(
                f"Uploading historic data to location '{self._historic_data_key}'...")
            self._s3.upload_fileobj(data, Bucket=self.s3_bucket, Key=self._historic_data_key, ExtraArgs={
                                    'ACL': 'bucket-owner-full-control'})
            logger.info("Upload complete.")
        except botocore.exceptions.BotoCoreError as e:
            logger.error(f"There was a problem with the s3 bucket: {e}")
        except botocore.exceptions.ClientError as e:
            logger.error(f"There was a problem with the s3 client: {e}")

    def _retrieve_historic_data(self, config: dict):
        """
        Retrieves historic data from s3 storage if it exists, and stores it locally on disk

        @param config: The runtime config file to obtain the data file paths from.
        """
        try:
            # We store the historic data as compressed JSON
            object_extension = "json.zip"

            # historic_data.json.zip is the file containing the coverage and meta-data of the last TIAF sequence run
            historic_data_file = f"historic_data.{object_extension}"

            # The location of the data is in the form <root_dir>/<branch>/<config>/<suites_string> so the build config of each branch gets its own historic data
            self._historic_data_dir = f"{self.root_dir}/{self.branch}/{config[self.COMMON_CONFIG_KEY][self.META_KEY][self.BUILD_CONFIG_KEY]}/{self._suites_string}"
            self._historic_data_key = f"{self._historic_data_dir}/{historic_data_file}"

            logger.info(
                f"Attempting to retrieve historic data for branch '{self.branch}' at location '{self._historic_data_key}' on bucket '{self.s3_bucket}'...")
            object = self._s3.get_object(
                Bucket=self.s3_bucket, Key=self._historic_data_key)

            logger.info(f"Historic data found for branch '{self.branch}'.")

            # Archive the existing object with the name of the existing last commit hash
            #archive_key = f"{self._historic_data_dir}/archive/{self._last_commit_hash}.{object_extension}"
            #logger.info(f"Archiving existing historic data to '{archive_key}' in bucket '{self._bucket.name}'...")
            #self._bucket.copy({"Bucket": self._bucket.name, "Key": self._historic_data_key}, archive_key)
            #logger.info(f"Archiving complete.")

            # Decode the historic data object into raw bytes and unpack into memory/disk
            decoded_data = self._decode(object)
            self._unpack_historic_data(decoded_data)
            return
        except KeyError as e:
            raise SystemError(f"The config does not contain the key {str(e)}.")
        except botocore.exceptions.BotoCoreError as e:
            raise SystemError(f"There was a problem with the s3 bucket: {e}")
        except botocore.exceptions.ClientError as e:
            if(e.response['Error']['Code'] == 'NoSuchKey'):
                logger.info(
                    "Error, no historic data found for this branch with key.")
                return
            raise SystemError(f"There was a problem with the s3 client: {e}")

    def _decode(self, s3_object):
        """
        Method to decompress and decode the json object and return it to the user

        @param s3_object:  The s3 object to download, decompress and decode
        """
        logger.info(f"Attempting to decode historic data object...")
        file_stream = s3_object['Body']
        decoded_data = zlib.decompress(file_stream.read()).decode('UTF-8')
        logger.info(f"Decoding complete.")

        # Decompress and unpack the zipped historic data JSON
        return decoded_data

    def _store_runtime_artifacts(self, runtime_artifact_dir: str):
        """
        Store the runtime artifacts in the designated persistent storage location.

        @param runtime_artifact_dir: The directory containing the runtime artifacts to store.
        """
        source_directory = pathlib.Path(runtime_artifact_dir)
        self._zip_and_upload_directory(source_directory, self.RUNTIME_ARTIFACT_DIRECTORY)

    def _store_coverage_artifacts(self, runtime_coverage_dir: str):
        """
        Store the coverage artifacts in the designated persistent storage location.

        @param runtime_coverage_dir: The directory containing the runtime coverage artifacts to store.
        """
        source_directory = pathlib.Path(runtime_coverage_dir)
        self._zip_and_upload_directory(source_directory, self.RUNTIME_COVERAGE_DIRECTORY)

    def _zip_and_upload_directory(self, source_directory: pathlib.Path, object_name: str):
        """
        Zip the directory indicated in source_directory and store it in the s3_bucket as a folder in the historic_data_dir.
        
        @param source_directory: Path to the directory to zip.
        @param target_directory: Name for the uploaded object.
        """
        artifact_key = f"{self._historic_data_dir}/{object_name}.zip"
        compressed_file_path = pathlib.Path(self._temp_workspace).joinpath("compressed_artifacts.zip")
        try:
            compressed_directory = shutil.make_archive(compressed_file_path.with_suffix(""), 'zip', source_directory)
            self._s3.upload_file(compressed_directory, Bucket=self.s3_bucket, Key=artifact_key,ExtraArgs={
                                'ACL': 'bucket-owner-full-control'})
        except botocore.exceptions.BotoCoreError as e:
            logger.error(f"There was a problem with the s3 bucket: {e}")
        except botocore.exceptions.ClientError as e:
            logger.error(f"There was a problem with the s3 client: {e}")
        except OSError as e:
            logger.warn(e)
        finally:
            if compressed_file_path.is_file():
                os.unlink(compressed_file_path)
