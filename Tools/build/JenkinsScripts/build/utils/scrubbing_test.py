#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import requests
from requests.auth import HTTPBasicAuth
from P4 import P4, P4Exception
from util import *
from zipfile import ZipFile
from download_from_s3 import s3_download_file, get_client
from botocore.exceptions import ClientError
from upload_to_s3 import s3_upload_file
import os
import shutil
import json
import urllib

PACKAGE_NAME_REGEX = r'^lumberyard-\d\.\d-\d+-\w+-\d+\.(zip|tgz)$'
P4_USER = 'lybuilder'
BUCKET = 'ly-scrubbing-test'

# Scrubber will fail if any of these files is missing, copy these files to scrubbing workspace before the test
SCRUBBING_REQUIRED_FILES = [
]

try:
    JENKINS_USERNAME = os.environ['JENKINS_USERNAME']
    JENKINS_API_TOKEN = os.environ['JENKINS_API_TOKEN']
    JENKINS_SERVER = os.environ['JENKINS_URL']
    P4_PORT = os.environ['ENV_P4_PORT']
    JOB_NAME = os.environ['JOB_NAME']
    BUILD_NUMBER = int(os.environ['BUILD_NUMBER'])
    WORKSPACE = os.environ['WORKSPACE']
    SCRUBBING_WORKSPACE = os.environ['SCRUBBING_WORKSPACE']
except KeyError:
    error('This script has to run on Jenkins')


class File:
    def __init__(self, path, action):
        self.path = path
        self.action = action


# Get the changelist numbers that trigger the build
def get_changelist_numbers():
    changelist_numbers = []
    changeset = []
    headers = {'Content-type': 'application/json', 'Accept': 'application/json'}
    try:
        res = requests.get('{}/job/{}/{}/api/json'.format(JENKINS_SERVER, JOB_NAME, BUILD_NUMBER),
                           auth=HTTPBasicAuth(JENKINS_USERNAME, JENKINS_API_TOKEN), headers=headers, verify=False)
        res = json.loads(res.content)
        changeset = res.get('changeSet').get('items')
    except:
        print 'Error: Failed to get changes from build {} in job {}'.format(BUILD_NUMBER, JOB_NAME)
    for item in changeset:
        changelist_numbers.append(item.get('changeNumber'))
    return changelist_numbers


# Get file list and actions that trigger the Jenkins job
def get_files():
    p4 = P4()
    p4.port = P4_PORT
    p4.user = P4_USER
    p4.connect()

    files = []
    changelist_numbers = get_changelist_numbers()
    for changelist_number in changelist_numbers:
        cmd = ['describe', '-s', changelist_number]
        try:
            res = p4.run(cmd)[0]
            file_list = res.get('depotFile')
            actions = res.get('action')
            for action, file_path in zip(actions, file_list):
                # P4 returns file paths that are url encoded
                file_path = urllib.unquote(file_path).decode("utf8")
                # Ignore files which are not in dev
                p = file_path.find('dev')
                if p != -1:
                    files.append(File(file_path[p:], action))
        except P4Exception:
            error('Internal error, please contact Build System')
    return files


def copy_file(src, dst, overwrite=False):
    if os.path.exists(dst) and not overwrite:
        return
    print 'Copying file from {} to {}'.format(src, dst)
    dest_file_dir = os.path.dirname(dst)
    if not os.path.exists(dest_file_dir):
        os.makedirs(dest_file_dir)
    shutil.copyfile(src, dst)


# Run scrubbing scripts
def scrub():
    print 'Perform the Code Scrubbing'
    scrubber_path = os.path.join(WORKSPACE, 'dev/Tools/build/JenkinsScripts/distribution/scrubbing/scrub_all.py')
    scrub_params = ["-p", "-d", "-o"]
    # Scrub code
    args = ['python', scrubber_path, '-p', '-d', '-o', os.path.join(SCRUBBING_WORKSPACE, 'dev/Code'), os.path.join(SCRUBBING_WORKSPACE, 'dev')]
    return_code = safe_execute_system_call(args)
    if return_code != 0:
        print 'ERROR: Code scrubbing failed.'
        return False
    print 'Code scrubbing complete successfully.'
    return True


# Run scrubbing validator
def validate():
    # Run validator
    print 'Running validator'
    validator_platforms = ["provo", "salem", "xenia"]
    success = True
    for validator_platform in validator_platforms:
        validator_path = os.path.join(WORKSPACE, 'dev/Tools/build/JenkinsScripts/distribution/scrubbing/validator.py')
        args = ['python', validator_path, '-p', validator_platform, os.path.join(SCRUBBING_WORKSPACE, 'dev')]
        if safe_execute_system_call(args):
            success = False
    if not success:
        print 'ERROR: Scrubbing validator failed.'
        return False
    print 'Scrubbing validator complete successfully.'
    return True


def scrubbing_test():
    if os.path.exists(SCRUBBING_WORKSPACE):
        os.system('rmdir /s /q \"{}\"'.format(SCRUBBING_WORKSPACE))
    os.mkdir(SCRUBBING_WORKSPACE)

    client = get_client('s3')
    zip_name = '{}.zip'.format(JOB_NAME)
    # Check if zipfile exists in S3 bucket
    try:
        client.head_object(Bucket=BUCKET, Key=zip_name)
    except ClientError as e:
        if e.response['Error']['Code'] == '404':
            print 'No previous zipfile found in S3 bucket {}'.format(BUCKET)
        else:
            raise
    else:
        # Download the zipfile from S3 bucket if the zipfile exists
        if not s3_download_file(client, SCRUBBING_WORKSPACE, zip_name, BUCKET, max_retry=3):
            warn('Failed to download {} from S3 bucket {}'.format(zip_name, BUCKET))

    # Unzip the zipfile to SCRUBBING_WORKSPACE
    zip_path = os.path.join(SCRUBBING_WORKSPACE, zip_name)
    if os.path.exists(zip_path):
        zip_file = ZipFile(os.path.join(SCRUBBING_WORKSPACE, zip_name), 'r')
        zip_file.extractall(SCRUBBING_WORKSPACE)
        zip_file.close()

    # Copy scrubbing required files to SCRUBBING_WORKSPACE, no overwrite
    for file in SCRUBBING_REQUIRED_FILES:
        src_file = os.path.join(WORKSPACE, file)
        dst_file = os.path.join(SCRUBBING_WORKSPACE, file)
        copy_file(src_file, dst_file)

    # Get file list and actions that trigger the Jenkins job
    files = get_files()

    # Copy or delete each file in SCRUBBING_WORKSPACE
    for f in files:
        dst_file = os.path.join(SCRUBBING_WORKSPACE, f.path)
        if 'delete' in f.action:
            if os.path.exists(dst_file):
                print 'Deleting {}'.format(dst_file)
                os.remove(dst_file)
        else:
            src_file = os.path.join(WORKSPACE, f.path)
            copy_file(src_file, dst_file, overwrite=True)

    # Backup the unmodified files and run scrubber and validator
    backup_path = os.path.join(SCRUBBING_WORKSPACE, 'backup')
    scrubbing_dev = os.path.join(SCRUBBING_WORKSPACE, 'dev')
    success = True
    if os.path.exists(scrubbing_dev):
        shutil.copytree(scrubbing_dev, os.path.join(backup_path, 'dev'))
        success = scrub() and validate()

    if success:
        # Delete zipfile from S3 if validator run successfully
        try:
            print 'Deleting {} from bucket {}'.format(zip_name, BUCKET)
            client.delete_object(Bucket=BUCKET, Key=zip_name)
        except:
            warn('Failed to delete {} from bucket {}'.format(zip_name, BUCKET))
    else:
        # Upload backup files to S3 bucket
        if os.path.exists(backup_path):
            zip_path = os.path.join(SCRUBBING_WORKSPACE, JOB_NAME)
            shutil.make_archive(zip_path, 'zip', backup_path)
            if not s3_upload_file(client, SCRUBBING_WORKSPACE, zip_name, BUCKET, max_retry=3):
                error('Failed to upload {} to S3 bucket {}'.format(zip_name, BUCKET))
        exit(1)


if __name__ == "__main__":
    scrubbing_test()
