"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import boto3
import botocore.exceptions
import logging
import os

import test_tools.shared.file_system as file_system

logger = logging.getLogger(__name__)
s3 = boto3.resource('s3')


class BucketNotExists(Exception):
    pass


class KeyExistsError(Exception):
    pass


class KeyDoesNotExistError(Exception):
    pass


def create_folder_in_bucket(bucket_name, folder_key):
    """
    Given bucket name and folder key will create specified folder if it doesn't exist
    :param bucket_name: name of the bucket where folder will be created
    :param folder_key: key in s3 where folder will be created (i.e. specifying full path to folder in s3)
    :return: True if folder was successfully created, False otherwise, will raise BucketNotExists exception if
                bucket doesn't exist
    """
    if not bucket_exists_in_s3(bucket_name):
        raise BucketNotExists("Bucket {} does not exist.".format(bucket_name))

    if key_exists_in_bucket(bucket_name, '{}/'.format(folder_key)):
        logger.error("Key {} already exists in bucket {}".format(folder_key, bucket_name))
        return False

    s3_bucket = s3.Bucket(bucket_name)
    logger.info("Creating {} folder in a {} bucket".format(folder_key, bucket_name))
    s3_bucket.put_object(Bucket=bucket_name, Key=(folder_key+'/'))
    return True


def upload_to_bucket(bucket_name, file_path, file_key=None, overwrite=False):
    """
    Uploads a given file to the given S3 bucket.
    :param bucket_name: Name of the S3 bucket where the file should be uploaded.
    :param file_path: Full Path to the target file on hard drive.
    :param file_key: Needed path to file on s3 (including file name).
    :param overwrite: Overwrite the key if it exists.
    """
    if not bucket_exists_in_s3(bucket_name):
        s3.create_bucket(Bucket=bucket_name)

    s3_bucket = s3.Bucket(bucket_name)

    if file_key is None:
        file_key = os.path.basename(file_path)

    if not overwrite and key_exists_in_bucket(bucket_name, file_key):
        raise KeyExistsError("Key '{}' already exists in S3 bucket {}".format(file_key, bucket_name))

    s3_bucket.upload_file(file_path, file_key)
    logger.info("Uploading {} to S3 bucket {}".format(file_key, bucket_name))


def download_from_bucket(bucket_name, file_key, destination_dir, file_name=None):
    """
    Download the given key from the given S3 bucket to the given destination. Logs an error if there is not enough \
    space available for the download.
    :param bucket_name: Name of the S3 bucket containing the desired file.
    :param file_key: Name of the file stored in S3.
    :param destination_dir: Directory where the file should be downloaded to.
    :param file_name: The name of the file you want to save it as. Defaults to the file_key.
    """
    bucket_exists_in_s3(bucket_name)

    if not key_exists_in_bucket(bucket_name, file_key):
        raise KeyDoesNotExistError("Key '{}' does not exist in S3 bucket {}".format(file_key, bucket_name))

    obj_summary = s3.ObjectSummary(bucket_name, file_key)
    required_space = obj_summary.size
    disk_name = os.path.splitdrive(destination_dir)[0]

    file_system.check_free_space(disk_name, required_space, "Insufficient space available for download:")

    if not os.path.exists(destination_dir):
        os.makedirs(destination_dir)

    if file_name is None:
        file_name = file_key
    destination_path = os.path.join(destination_dir, file_name)
    s3.Object(bucket_name, file_key).download_file(destination_path)
    logger.info("Downloading {} to {}".format(file_key, destination_path))


def bucket_exists_in_s3(bucket_name):
    """
    Verifies that the S3 bucket exists.
    :param bucket_name: Name of the S3 bucket that may or may not exist.
    :return: True if the bucket exists. False otherwise.
    """
    bucket_exists = True

    try:
        s3.meta.client.head_bucket(Bucket=bucket_name)
    except botocore.exceptions.ClientError as err:
        if err.response['Error']['Code'] == '404':
            bucket_exists = False

    return bucket_exists


def key_exists_in_bucket(bucket_name, file_key):
    """
    Verifies that the given key does not already exist in the given S3 bucket.
    :param bucket_name: Name of the S3 bucket that may or may not contain the file key.
    :param file_key: Name of the file key in question.
    :return: True if the key exists. False otherwise.
    """
    key_exists = True
    obj_summary = s3.ObjectSummary(bucket_name, file_key)

    # Attempting to access any member of ObjectSummary for a nonexistent key will throw an exception
    # There is no built-in way to check key existence otherwise
    try:
        obj_summary.size
    except botocore.exceptions.ClientError as err:
        if err.response['Error']['Code'] == '404':
            key_exists = False

    return key_exists

