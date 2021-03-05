"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Downloads the latest package from a S3 and unzips it to a desired location.
"""
import argparse
import boto3
import os
import re
import zipfile


def download_and_unzip_package(bucket_name, package_regex, build_number_regex, folder_path, destination_path):
    """
    Downloads a given package from a S3 and unzips it.
    :param bucket_name: S3 bucket
    :param package_regex: Regex to find the desired package
    :param build_number_regex: Regex to find the build number from the package name
    :param folder_path: Folder path to the package
    :param destination_path: Where to download the package to
    :return:
    """
    # Make sure the directory exists
    if not os.path.isdir(destination_path):
        os.makedirs(destination_path)

    # Sorting function for latest package
    def get_build_number(file_name_to_parse):
        return re.search(build_number_regex, file_name_to_parse).group(0)[:-4]  # [:-4] removes the .zip extension

    s3 = boto3.resource('s3')
    bucket = s3.Bucket(bucket_name)
    largest_build_number = -1
    latest_file = 'No file found!'
    # Find the latest package
    print 'Reading files from bucket...'
    for bucket_file in bucket.objects.filter(Prefix=folder_path):
        file_name = bucket_file.key
        if re.search(package_regex, file_name) and get_build_number(file_name) > largest_build_number:
            largest_build_number = get_build_number(file_name)
            latest_file = file_name

    package_name = latest_file.split('/')[-1]

    # Download the package
    print('Downloading package: {0} from bucket {1} to {2}'.format(latest_file, bucket_name, destination_path))
    s3.Bucket(bucket_name).download_file(latest_file, os.path.join(destination_path, package_name))

    # Unzip the package
    with zipfile.ZipFile(os.path.join(destination_path, package_name), 'r') as zip_ref:
        print('Unzipping package: {0} to {1}'.format(package_name, destination_path))
        zip_ref.extractall(destination_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bucket_name', required=True, help='Bucket that holds the package.')
    parser.add_argument('-p', '--package_regex', required=True,
                        help='Regex to identify a package.  Such as: lumberyard-0.0-[\d]{6,7}-pc-[\d]{4}.zip\s to find '
                             'the main pc package.')
    parser.add_argument('-n', '--build_number_regex', required=True,
                        help='Regex to identify the build number. Such as [\d]{4,5}.zip$ to find the build number from '
                             'the name of the main pc package')
    parser.add_argument('-d', '--destination_path', required=True, help='Destination for the contents of the packages.')
    parser.add_argument('-f', '--folder_path', help='Folder that contains the package, must include /.')

    args = parser.parse_args()
    download_and_unzip_package(args.bucket_name, args.package_regex, args.build_number_regex, args.folder_path,
                               args.destination_path)


if __name__ == "__main__":
    main()