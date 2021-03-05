"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Downloads packages and unzips them.
"""
import argparse
import boto3
import os
import zipfile


def download_and_unzip_packages(bucket_name, package_key, folder_path, destination_path):
    """
    Downloads a given package from a S3 and unzips it.
    :param bucket_name: S3 bucket
    :param package_key: Key for the package
    :param folder_path: Folder path to the package
    :param destination_path: Where to download the package to
    :return:
    """
    # Make sure the directory exists
    if not os.path.isdir(destination_path):
        os.makedirs(destination_path)

    # Download the package
    s3 = boto3.resource('s3')
    print('Downloading package: {0} from bucket {1} to {2}'.format(package_key, bucket_name, destination_path))
    s3.Bucket(bucket_name).download_file(folder_path + package_key, os.path.join(destination_path, package_key))

    # Unzip the package
    with zipfile.ZipFile(os.path.join(destination_path, package_key), 'r') as zip_ref:
        print('Unzipping package: {0} to {1}'.format(package_key, destination_path))
        zip_ref.extractall(destination_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bucket_name', required=True, help='Bucket that holds the package.')
    parser.add_argument('-p', '--package_key', required=True, help='Desired package\'s key.')
    parser.add_argument('-d', '--destination_path', required=True, help='Destination for the contents of the packages.')
    parser.add_argument('-f', '--folder_path', help='Folder that contains the package, must include /.')

    args = parser.parse_args()
    download_and_unzip_packages(args.bucket_name, args.package_key, args.folder_path, args.destination_path)


if __name__ == "__main__":
    main()