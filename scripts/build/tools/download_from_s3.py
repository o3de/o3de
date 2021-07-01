#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

'''
Usage:
Use EC2 role to download files to %WORKSPACE% folder from bucket bucket_name:
python download_from_s3.py --base_dir %WORKSPACE% --files_to_download "file1,file2" --bucket bucket_name

Use profile to download files to %WORKSPACE% folder from bucket bucket_name:
python download_from_s3.py --base_dir %WORKSPACE% --profile profile --files_to_download "file1,file2" --bucket bucket_name
'''


import os
import json
import boto3
from optparse import OptionParser
from util import error


def parse_args():
    parser = OptionParser()
    parser.add_option("--base_dir", dest="base_dir", default=os.getcwd(), help="Base directory to download files, If not given, then current directory is used.")
    parser.add_option("--files_to_download", dest="files_to_download", default=None, help="Files to download, separated by comma.")
    parser.add_option("--profile", dest="profile", default=None, help="The name of a profile to use. If not given, then the default profile is used.")
    parser.add_option("--bucket", dest="bucket", default=None, help="S3 bucket the files are downloaded from.")
    parser.add_option("--key_prefix", dest="key_prefix", default='', help="Object key prefix.")
    '''
    ExtraArgs used to call s3.download_file(), should be in json format. extra_args key must be one of: ACL, CacheControl, ContentDisposition, ContentEncoding, ContentLanguage, ContentType, Expires,
    GrantFullControl, GrantRead, GrantReadACP, GrantWriteACP, Metadata, RequestPayer, ServerSideEncryption, StorageClass,
    SSECustomerAlgorithm, SSECustomerKey, SSECustomerKeyMD5, SSEKMSKeyId, WebsiteRedirectLocation
    '''
    parser.add_option("--extra_args", dest="extra_args", default=None, help="Additional parameters used to download file.")
    parser.add_option("--max_retry", dest="max_retry", default=1, help="Maximum retry times to download file.")
    (options, args) = parser.parse_args()
    if not os.path.isdir(options.base_dir):
        error('{} is not a valid directory'.format(options.base_dir))
    if not options.files_to_download:
        error('Use --files_to_download to specify files to download, separated by comma.')
    if not options.bucket:
        error('Use --bucket to specify bucket that the files are downloaded from.')
    return options


def get_client(service_name, profile_name=None):
    session = boto3.session.Session(profile_name=profile_name)
    client = session.client(service_name)
    return client


def s3_download_file(client, base_dir, file, bucket, key_prefix=None, extra_args=None, max_retry=1):
    print('Downloading file {} from bucket {}.'.format(file, bucket))
    key = file if key_prefix is None else '{}/{}'.format(key_prefix, file)
    for x in range(max_retry):
        try:
            client.download_file(
                bucket, key, os.path.join(base_dir, file),
                ExtraArgs=extra_args
            )
            print('Download succeeded')
            return True
        except:
            print('Retrying download...')
    print('Download failed')
    return False


def download_files(base_dir, files_to_download, bucket, key_prefix=None, profile=None, extra_args=None, max_retry=1):
    client = get_client('s3', profile)
    files_to_download = files_to_download.split(',')
    extra_args = json.loads(extra_args) if extra_args else None

    print('Downloading {} files from bucket {}.'.format(len(files_to_download), bucket))
    failure = []
    success = []
    for file in files_to_download:
        if not s3_download_file(client, base_dir, file, bucket, key_prefix, extra_args, max_retry):
            failure.append(file)
        else:
            success.append(file)
    print('{} files are downloaded successfully:'.format(len(success)))
    print('\n'.join(success))
    print('{} files failed to download:'.format(len(failure)))
    print('\n'.join(failure))
    # Exit with error code 1 if any file is failed to download
    if len(failure) > 0:
        return False
    return True


if __name__ == "__main__":
    options = parse_args()
    download_files(options.base_dir, options.files_to_download, options.bucket, options.key_prefix, options.profile, options.extra_args, options.max_retry)
