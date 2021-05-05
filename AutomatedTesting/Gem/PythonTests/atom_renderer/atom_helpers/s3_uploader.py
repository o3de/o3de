"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Used for uploading Atom test artifacts to s3 to debug failures.
"""

import logging
import os

import boto3
import botocore.exceptions
from botocore.config import Config


logger = logging.getLogger(__name__)
s3_config = Config(
    region_name='us-west-2',
    signature_version='s3v4',
    s3={"payload_signing_enabled": True},
    retries={'max_attempts': 10, 'mode': 'standard'})
s3 = boto3.resource('s3', config=s3_config)


class BucketDoesNotExistError(Exception):
    """Raised when a bucket_name param references an s3 bucket that doesn't exist."""
    pass


class FileKeyExistsError(Exception):
    """Raised when a referenced file key already exists."""
    pass


def _bucket_exists_in_s3(bucket_name):
    """
    Verifies that the S3 bucket exists.
    :param bucket_name: Name of the S3 bucket that may or may not exist.
    :return: True if the bucket exists. False otherwise.
    """
    bucket_exists = False

    try:
        s3.meta.client.head_bucket(Bucket=bucket_name)
        bucket_exists = True
    except botocore.exceptions.ClientError as err:
        if err.response['Error']['Code'] in ['400', '404']:
            bucket_exists = False

    return bucket_exists


def create_folder_in_bucket(bucket_name, folder_key):
    """
    Given bucket name and folder key will create specified folder if it doesn't exist
    :param bucket_name: name of the bucket where folder will be created
    :param folder_key: key in s3 where folder will be created (i.e. specifying full path to folder in s3)
    :return: s3.Object class containing the newly created folder key as part of its 'key' attribute
            or raises an BucketDoesNotExistError exception.
    """
    if not _bucket_exists_in_s3(bucket_name):
        raise BucketDoesNotExistError(f"Bucket {bucket_name} does not exist. Please supply a valid bucket_name param.")
    logger.info(f"Creating {folder_key} folder in a {bucket_name} bucket")

    return s3.Object(bucket_name=bucket_name, key=f"{folder_key}")


def upload_to_bucket(bucket_name, file_path, file_key=None):
    """
    Uploads a given file to the given S3 bucket.
    :param bucket_name: Name of the S3 bucket where the file should be uploaded.
    :param file_path: Full Path to the target file on hard drive.
    :param file_key: Needed path to file on s3 (including file name).
    """
    if not _bucket_exists_in_s3(bucket_name):
        raise BucketDoesNotExistError(
            f'S3 Bucket: {bucket_name} does not exist, please create it first or '
            f'set the bucket_must_exist param to False.')

    if file_key is None:
        file_key = os.path.basename(file_path)

    s3_bucket = s3.Bucket(name=bucket_name)
    s3_bucket.upload_file(Filename=file_path, Key=file_key)
    logger.info(f"Uploading '{file_path}' to S3 bucket '{bucket_name}' in s3 file key: '{file_key}'")


def bulk_upload_to_bucket(bucket_name, list_of_file_paths, file_key=None):
    """
    Uploads a list of given file paths to a given s3 bucket.
    :param bucket_name: Name of the S3 bucket where the file should be uploaded.
    :param list_of_file_paths: List containing full paths to each target file on the hard drive.
    :param file_key: Needed path to file on s3 (including file name).
    """
    for file_path in list_of_file_paths:
        parsed_file_path, parsed_file_extension = os.path.splitext(file_path)
        parsed_file_name = parsed_file_path.split('\\')[-1]  # The last index is the file name (including extension)
        s3_file_upload = f"{parsed_file_name}{parsed_file_extension}"
        upload_to_bucket(
            bucket_name=bucket_name,
            file_path=file_path,
            file_key=f"{file_key}/{s3_file_upload}")
