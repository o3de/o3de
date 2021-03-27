"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import pytest
pytest.importorskip("boto3")
import boto3
import botocore.exceptions
import logging
import os

import ly_test_tools.environment.file_system as file_system

logger = logging.getLogger(__name__)


class KeyExistsError(Exception):
    pass


class KeyDoesNotExistError(Exception):
    pass


class S3Utils(object):
    """
    Stores a boto3 S3 client to use for AWS S3 functionalities.
    """
    DEFAULT_REGION = 'us-west-2'

    def __init__(self, boto3_session=None):
        # type: (boto3.Session) -> None
        """
        The boto3 session can be set during init, or a default one will be created.
        :param boto3_session: A boto3 session
        """
        if boto3_session:
            self._session = boto3_session
        else:
            logger.info("No session provided, using default profile for s3 resource")
            self._session = boto3.session.Session()
        self._s3_resource = self._session.resource('s3')

    def upload_to_bucket(self, bucket_name, file_path, overwrite=False):
        """
        Uploads a given file to the given S3 bucket.
        :param bucket_name: Name of the S3 bucket where the file should be uploaded.
        :param file_path: Path to the target file.
        :param overwrite: Overwrite the key if it exists.
        """
        if not self.bucket_exists_in_s3(bucket_name):
            self._s3_resource.create_bucket(Bucket=bucket_name)

        s3_bucket = self._s3_resource.Bucket(bucket_name)

        file_key = os.path.basename(file_path)
        if not overwrite and self.key_exists_in_bucket(bucket_name, file_key):
            raise KeyExistsError("Key '{}' already exists in S3 bucket {}".format(file_key, bucket_name))

        s3_bucket.upload_file(file_path, file_key)
        logger.info("Uploading {} to S3 bucket {}".format(file_key, bucket_name))

    def download_from_bucket(self, bucket_name, file_key, destination_dir, file_name=None):
        """
        Download the given key from the given S3 bucket to the given destination. Logs an error if there is not enough \
        space available for the download.
        :param bucket_name: Name of the S3 bucket containing the desired file.
        :param file_key: Name of the file stored in S3.
        :param destination_dir: Directory where the file should be downloaded to.
        :param file_name: The name of the file you want to save it as. Defaults to the file_key.
        """
        self.bucket_exists_in_s3(bucket_name)

        if not self.key_exists_in_bucket(bucket_name, file_key):
            raise KeyDoesNotExistError("Key '{}' does not exist in S3 bucket {}".format(file_key, bucket_name))

        obj_summary = self._s3_resource.ObjectSummary(bucket_name, file_key)
        required_space = obj_summary.size

        file_system.check_free_space(destination_dir, required_space, "Insufficient space available for download:")

        if not os.path.exists(destination_dir):
            os.makedirs(destination_dir)

        if file_name is None:
            file_name = file_key
        destination_path = os.path.join(destination_dir, file_name)
        self._s3_resource.Object(bucket_name, file_key).download_file(destination_path)
        logger.info("Downloading {} to {}".format(file_key, destination_path))

    def bucket_exists_in_s3(self, bucket_name):
        """
        Verifies that the S3 bucket exists.
        :param bucket_name: Name of the S3 bucket that may or may not exist.
        :return: True if the bucket exists. False otherwise.
        """
        bucket_exists = True

        try:
            self._s3_resource.meta.client.head_bucket(Bucket=bucket_name)
        except botocore.exceptions.ClientError as err:
            if err.response['Error']['Code'] == '404':
                bucket_exists = False

        return bucket_exists

    def key_exists_in_bucket(self, bucket_name, file_key):
        """
        Verifies that the given key does not already exist in the given S3 bucket.
        :param bucket_name: Name of the S3 bucket that may or may not contain the file key.
        :param file_key: Name of the file key in question.
        :return: True if the key exists. False otherwise.
        """
        key_exists = True
        obj_summary = self._s3_resource.ObjectSummary(bucket_name, file_key)

        # Attempting to access any member of ObjectSummary for a nonexistent key will throw an exception
        # There is no built-in way to check key existence otherwise
        try:
            obj_summary.size
        except botocore.exceptions.ClientError as err:
            if err.response['Error']['Code'] == '404':
                key_exists = False

        return key_exists
