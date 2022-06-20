#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

'''
Usage:
Use EC2 role to upload all .zip and .MD5 files in %WORKSPACE% folder to bucket ly-packages-mainline:
python upload_to_s3.py --base_dir %WORKSPACE% --file_regex "(.*zip$|.*MD5$)" --bucket ly-packages-mainline

Use profile to upload all .zip and .MD5 files in %WORKSPACE% folder to bucket ly-packages-mainline:
python upload_to_s3.py --base_dir %WORKSPACE% --profile profile --file_regex "(.*zip$|.*MD5$)" --bucket ly-packages-mainline

Another example usage for uploading all .png and .ppm files inside base_dir and only subdirectories within base_dir:
python upload_to_s3.py --base_dir %WORKSPACE%/path/to/files --file_regex "(.*png$|.*ppm$)" --bucket screenshot-test-bucket --search_subdirectories True --key_prefix Test

'''


import os
import re
import json
import time
import boto3
import pathlib
from optparse import OptionParser


def parse_args():
    parser = OptionParser()
    parser.add_option("--base_dir", dest="base_dir", default=os.getcwd(), help="Base directory to upload files, If not given, then current directory is used.")
    parser.add_option("--file_regex", dest="file_regex", default=None, help="Regular expression that used to match file names to upload.")
    parser.add_option("--profile", dest="profile", default=None, help="The name of a profile to use. If not given, then the default profile is used.")
    parser.add_option("--bucket", dest="bucket", default=None, help="S3 bucket the files are uploaded to.")
    parser.add_option("--key_prefix", dest="key_prefix", default='', help="Object key prefix.")
    parser.add_option("--search_subdirectories", dest="search_subdirectories", action='store_true',
                      help="Toggle for searching for files in subdirectories beneath base_dir, defaults to False")
    '''
    ExtraArgs used to call s3.upload_file(), should be in json format. extra_args key must be one of: ACL, CacheControl, ContentDisposition, ContentEncoding, ContentLanguage, ContentType, Expires,
    GrantFullControl, GrantRead, GrantReadACP, GrantWriteACP, Metadata, RequestPayer, ServerSideEncryption, StorageClass,
    SSECustomerAlgorithm, SSECustomerKey, SSECustomerKeyMD5, SSEKMSKeyId, WebsiteRedirectLocation
    '''
    parser.add_option("--extra_args", dest="extra_args", default=None, help="Additional parameters used to upload file.")
    parser.add_option("--max_retry", dest="max_retry", default=1, help="Maximum retry times to upload file.")
    (options, args) = parser.parse_args()
    if not os.path.isdir(options.base_dir):
        error('{} is not a valid directory'.format(options.base_dir))
    if not options.file_regex:
        error('Use --file_regex to specify regular expression that used to match file names to upload.')
    if not options.bucket:
        error('Use --bucket to specify bucket that the files are uploaded to.')
    return options


def error(message):
    print(f'Error: {message}')
    exit(1)


def get_client(service_name, profile_name):
    session = boto3.session.Session(profile_name=profile_name)
    client = session.client(service_name)
    return client


def get_files_to_upload(base_dir, regex, search_subdirectories):
    """
    Uses a regex expression pattern to return a list of file paths for files to upload to the s3 bucket.
    :param base_dir: path for the base directory, if using search_subdirectories=True ensure this is the parent.
    :param regex: pattern to use for regex searching, ex. "(.*zip$|.*MD5$)"
    :param search_subdirectories: boolean False for only getting files in base_dir, True to get all files in base_dir
        and any subdirectory inside base_dir, defaults to False from the parse_args() function.
    :return: a list of string file paths for files to upload to the s3 bucket matching the regex expression.
    """
    # Get all file names in base directory
    files = [os.path.join(base_dir, x) for x in os.listdir(base_dir) if os.path.isfile(os.path.join(base_dir, x))]
    if search_subdirectories:  # Get all file names in base directory and any subdirectories.
        for subdirectory in os.walk(base_dir):
            # Example output for subdirectory:
            # ('C:\path\to\base_dir\', ['Subfolder1', 'Subfolder2'], ['file1', 'file2'])
            subdirectory_file_path = subdirectory[0]
            subdirectory_files = subdirectory[2]
            if subdirectory_files:
                subdirectory_file_paths = _build_file_paths(subdirectory_file_path, subdirectory_files)
                files.extend(subdirectory_file_paths)

    try:
        regex = json.loads(regex)  # strip the surround quotes, if they exist
    except:
        print(f'WARNING: failed to call json.loads() for regex: "{regex}"')
        pass
    # Get all file names matching the regular expression, those file will be uploaded to S3
    regex_files_to_upload = [x for x in files if re.match(regex, x)]

    return regex_files_to_upload


def s3_upload_file(client, base_dir, file, bucket, key_prefix=None, extra_args=None, max_retry=1):
    try:
        # replicate the local folder structure relative to search root in the bucket path
        s3_file_path = pathlib.Path(file).relative_to(base_dir).as_posix()
    except ValueError as err:
        print(f'Unexpected file error: {err}')
        return False

    key = s3_file_path if key_prefix is None else f'{key_prefix}/{s3_file_path}'
    error_message = None

    for x in range(max_retry):
        try:
            client.upload_file(file, bucket, key, ExtraArgs=extra_args)
            return True
        except Exception as err:
            time.sleep(0.1)  # Sleep for 100 milliseconds between retries.
            error_message = err

    print(f'Upload failed - Exception while uploading: {error_message}')
    return False


def _build_file_paths(path_to_files, files_in_path):
    """
    Given a path containing files, returns a list of strings representing complete paths to each file.
    :param path_to_files: path to the location storing the files to create string paths for
    :param files_in_path: list of files that are inside the path_to_files path string
    :return: list of fully parsed file path strings from path_to_files path.
    """
    parsed_file_paths = []

    for file_in_path in files_in_path:
        complete_file_path = os.path.join(path_to_files, file_in_path)
        if os.path.isfile(complete_file_path):
            parsed_file_paths.append(complete_file_path)

    return parsed_file_paths


if __name__ == "__main__":
    options = parse_args()
    client = get_client('s3', options.profile)
    files_to_upload = get_files_to_upload(options.base_dir, options.file_regex, options.search_subdirectories)
    extra_args = json.loads(options.extra_args) if options.extra_args else None

    print(('Uploading {} files to bucket {}.'.format(len(files_to_upload), options.bucket)))
    failure = []
    success = []
    for file in files_to_upload:
        if not s3_upload_file(client, options.base_dir, file, options.bucket, options.key_prefix, extra_args, 2):
            failure.append(file)
        else:
            success.append(file)
    print('Upload finished.')
    print(('{} files are uploaded successfully:'.format(len(success))))
    print(('\n'.join(success)))
    if len(failure) > 0:
        print(('{} files failed to upload:'.format(len(failure))))
        print(('\n'.join(failure)))
        # Exit with error code 1 if any file is failed to upload
        exit(1)
