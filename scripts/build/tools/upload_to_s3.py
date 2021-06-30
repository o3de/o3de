#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

'''


import os
import re
import json
import boto3
from optparse import OptionParser


def parse_args():
    parser = OptionParser()
    parser.add_option("--base_dir", dest="base_dir", default=os.getcwd(), help="Base directory to upload files, If not given, then current directory is used.")
    parser.add_option("--file_regex", dest="file_regex", default=None, help="Regular expression that used to match file names to upload.")
    parser.add_option("--profile", dest="profile", default=None, help="The name of a profile to use. If not given, then the default profile is used.")
    parser.add_option("--bucket", dest="bucket", default=None, help="S3 bucket the files are uploaded to.")
    parser.add_option("--key_prefix", dest="key_prefix", default='', help="Object key prefix.")
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


def get_files_to_upload(base_dir, regex):
    # Get all file names in base directory
    files = [x for x in os.listdir(base_dir) if os.path.isfile(os.path.join(base_dir, x))]
    # strip the surround quotes, if they exist
    try:
        regex = json.loads(regex)
    except:
        pass
    # Get all file names matching the regular expression, those file will be uploaded to S3
    files_to_upload = [x for x in files if re.match(regex, x)]
    return files_to_upload


def s3_upload_file(client, base_dir, file, bucket, key_prefix=None, extra_args=None, max_retry=1):
    print(('Uploading file {} to bucket {}.'.format(file, bucket)))
    key = file if key_prefix is None else '{}/{}'.format(key_prefix, file)
    for x in range(max_retry):
        try:
            client.upload_file(
                os.path.join(base_dir, file), bucket, key,
                ExtraArgs=extra_args
            )
            print('Upload succeeded')
            return True
        except Exception as err:
            print(('exception while uploading: {}'.format(err)))
            print('Retrying upload...')
    print('Upload failed')
    return False


if __name__ == "__main__":
    options = parse_args()
    client = get_client('s3', options.profile)
    files_to_upload = get_files_to_upload(options.base_dir, options.file_regex)
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
