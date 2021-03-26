########################################################################################
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
#
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#
########################################################################################
import ast
import boto3
from botocore.exceptions import ClientError
from datetime import datetime
import logging
import os
import shutil
import sys
import time
import traceback
import urllib2
import uuid

KINESIS_STREAM_NAME = 'lumberyard-metrics-stream'
KINESIS_MAX_RECORD_SIZE = 1048576  # 1 MB
S3_BACKUP_BUCKET = 'infrastructure-build-metrics-backup'
IAM_ROLE_NAME = 'ec2-jenkins-node'
LOG_FILE_NAME = 'kinesis_upload.log'

MAX_RECORD_SIZE = KINESIS_MAX_RECORD_SIZE - 4  # to account for version header
MAX_RETRIES = 5
RETRY_EXCEPTIONS = ('ProvisionedThroughputExceededException',
                    'ThrottlingException')

# truncate the log file, eventually we need to send the logs to cloudwatch logs
with open(LOG_FILE_NAME, 'w'):
    pass

logger = logging.getLogger('KinesisUploader')

fileHdlr = logging.FileHandler(LOG_FILE_NAME)
# uncomment this line and the two below to have logs go to stdout for debugging purposes
#streamHdlr = logging.StreamHandler(sys.stdout)

formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
fileHdlr.setFormatter(formatter)
#streamHdlr.setFormatter(formatter)

logger.addHandler(fileHdlr)
#logger.addHandler(streamHdlr)
logger.setLevel(logging.DEBUG)

def backup_file_to_s3(s3_client, bucket_name, file_location, s3_file_name):
    try:
        s3_client.meta.client.upload_file(file_location, bucket_name, s3_file_name)
        os.remove(file_location)
    except:
        logger.error('Failed to upload backup file to S3. This is non-fatal!')
        # logger.error(traceback.print_exc())


def get_iam_role_credentials(role_name):
    security_metadata = None
    try:
        response = urllib2.urlopen(
            'http://169.254.169.254/latest/meta-data/iam/security-credentials/{0}'.format(role_name)).read()
        security_metadata = ast.literal_eval(response)
    except:
        logger.error('Unable to get iam role credentials')
        logger.error(traceback.print_exc())

    return security_metadata


def splitFileByRecord(stream, maxSize):
    version = 1

    # currently using random GUID for partition key, but in the future we may want to partition by some build id
    # or by build host
    partition_key = uuid.uuid4()

    entry_size = 0
    put_entries = []

    current_entry = ''
    for line in stream:
        line_size_in_bytes = len(line.encode('utf-8'))
        entry_size = entry_size + line_size_in_bytes

        if (entry_size > MAX_RECORD_SIZE):
            put_entries.append({
                'Data': str(version) + '\n' + str(current_entry),
                'PartitionKey': str(partition_key)
            })

            current_entry = line
            entry_size = line_size_in_bytes
        else:
            current_entry = current_entry + line

    if current_entry:
        put_entries.append({
            'Data': str(version) + '\n' + str(current_entry),
            'PartitionKey': str(partition_key)
        })

    return put_entries


def main():
    credentials = get_iam_role_credentials(IAM_ROLE_NAME)

    aws_access_key_id = None
    aws_secret_access_key = None
    aws_session_token = None

    if credentials is not None:
        keys = ['AccessKeyId', 'SecretAccessKey', 'Token']
        for key in keys:
            if key not in credentials:
                logger.error('Unable to find {0} in get_iam_role_credentials response {1}'.format(key, credentials))
                return

        aws_access_key_id = credentials['AccessKeyId']
        aws_secret_access_key = credentials['SecretAccessKey']
        aws_session_token = credentials['Token']

    kinesis_client = boto3.client('kinesis', region_name='us-west-2', aws_access_key_id=aws_access_key_id,
                                  aws_secret_access_key=aws_secret_access_key, aws_session_token=aws_session_token)
    s3_client = boto3.resource('s3', aws_access_key_id=aws_access_key_id, aws_secret_access_key=aws_secret_access_key,
                               aws_session_token=aws_session_token)

    file_location = sys.argv[1]
    filename = os.path.basename(file_location)
    backup_file_location = file_location + '.bak'

    try:
        if os.path.isfile(backup_file_location):
            logger.info('Found pre-existing backup file.  Uploading to S3.')
            backup_file_to_s3(s3_client, S3_BACKUP_BUCKET, backup_file_location,
                              '{0}.{1}'.format(filename, datetime.now().isoformat()))

        if os.path.isfile(file_location):
            shutil.copyfile(file_location, backup_file_location)
            backup_file_to_s3(s3_client, S3_BACKUP_BUCKET, backup_file_location,
                              '{0}.{1}'.format(filename, datetime.now().isoformat()))

            logger.info('Opening metrics file {0}'.format(file_location))
            with open(file_location, 'r+') as f:
                records = splitFileByRecord(f, MAX_RECORD_SIZE)
                i = 0
                retries = 0
                while i < len(records):
                    record = records[i]
                    try:
                        logger.info('Uploading {0} bytes of metrics to Kinesis...'.format(len(record)))
                        kinesis_client.put_record(StreamName=KINESIS_STREAM_NAME,
                                                  Data=record['Data'],
                                                  PartitionKey=record['PartitionKey'])
                        retries = 0
                    except ClientError as ex:
                        if ex.response['Error']['Code'] not in RETRY_EXCEPTIONS:
                            raise

                        sleep_time = 2 ** retries
                        logger.warn('Request throttled by Kinesis, '
                                     'sleeping and retrying in {0} seconds'.format(2 ** retries))
                        time.sleep(sleep_time)
                        retries += 1
                        i -= 1

                    i += 1

                f.truncate(0)
    except:
        logger.error(traceback.print_exc())

if __name__ == '__main__':
    main()