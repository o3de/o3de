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

"""
This script will be used in the Sandobx Jenkins job PACKAGE_COPY_S3
PACKAGE_COPY_S3 is a downstream job of nightly packaging job, it copies the nightly packages from Infra S3 bucket to Lionbridge S3 bucket based on the INCLUDE_FILTER passed from packaging job
"""
import os
import re
import json
import requests
from requests.auth import HTTPBasicAuth
import boto3
from util import error, warn


# Write EMAIL_TEMPLATE to a file and inject it into the email sent to Lionbridge
EMAIL_TEMPLATE = '''Packages are uploaded to S3 bucket {}
Package List:
{}


Changelists:
{}
'''


def get_jenkins_env(key):
    try:
        return os.environ[key]
    except KeyError:
        print('Error: Jenkins parameters {} is not set.'.format(key))
    return None


JENKINS_USERNAME = get_jenkins_env('JENKINS_USERNAME')
JENKINS_API_TOKEN = get_jenkins_env('JENKINS_API_TOKEN')
JENKINS_URL = get_jenkins_env('JENKINS_URL')
WORKSPACE = get_jenkins_env('WORKSPACE')
S3_TARGET = get_jenkins_env('S3_TARGET')
INCLUDE_FILTER = get_jenkins_env('INCLUDE_FILTER')
EMAIL_TEMPLATE_FILE = get_jenkins_env('EMAIL_TEMPLATE_FILE')
if None in [JENKINS_USERNAME, JENKINS_API_TOKEN, JENKINS_URL, WORKSPACE, S3_TARGET, INCLUDE_FILTER, EMAIL_TEMPLATE_FILE]:
    error('Please make sure all Jenkins parameters are set correctly.')


def parse_include_filter(include_filter):
    try:
        res = re.search('^(\w*)-*lumberyard-(\d+)\.(\d+)-(\d+)-(\w+).*\*(\d+)\.\*', include_filter)
        branch = res.group(1)
        major_version = int(res.group(2))
        minor_version = int(res.group(3))
        changelist_number = res.group(4)
        platform = res.group(5)
        build_number = res.group(6)
        return branch, major_version, minor_version, changelist_number, platform, build_number
    except (AttributeError, IndexError):
        error('Unable to parse INCLUDE_FILTER, please make sure the INCLUDE_FILTER is set correctly')


# Get the changelists that trigger the build
def get_changelists(job_name, build_number):
    changelists = []
    headers = {'Content-type': 'application/json', 'Accept': 'application/json'}
    try:
        res = requests.get('{}/job/{}/{}/api/json'.format(JENKINS_URL, job_name, build_number),
                           auth=HTTPBasicAuth(JENKINS_USERNAME, JENKINS_API_TOKEN), headers=headers, verify=False)
        res = json.loads(res.content)
        changelists = res.get('changeSet').get('items')
        return changelists
    except:
        warn('Error: Failed to get changes from build {} in job {}'.format(build_number, job_name))
        return []


def get_packaging_job_name(branch, major_version, minor_version, platform):
    if branch == '':
        branch = 'ML' if major_version + minor_version == 0 else 'v{}_{}'.format(major_version, minor_version)
    job_name = 'PKG_{}_{}'.format(branch, platform.capitalize())
    return job_name


# Get package names by looking up S3 bucket
def get_package_names(branch, major_version, minor_version, include_filter, build_number):
    package_names = []
    prefix = include_filter[:include_filter.find('*')]
    pattern = '.*{}.*{}..*'.format(prefix, build_number)
    if branch == '':
        bucket_name = 'ly-packages-mainline' if major_version + minor_version == 0 else 'ly-packages-release-candidate'
        folder = 'lumberyard-packages'
    else:
        bucket_name = 'ly-packages-feature-branches'
        folder = 'lumberyard-packages/{}'.format(branch)
    s3 = boto3.resource('s3')
    bucket = s3.Bucket(bucket_name)
    for obj in bucket.objects.filter(Prefix='{}/{}'.format(folder, prefix)):
        package_name = obj.key
        if re.match(pattern, package_name):
            package_names.append(package_name.replace('{}/'.format(folder), ''))
    return package_names


if __name__ == "__main__":
    branch, major_version, minor_version, changelist_number, platform, build_number = parse_include_filter(INCLUDE_FILTER)
    packaging_job_name = get_packaging_job_name(branch, major_version, minor_version, platform)
    changelists = get_changelists(packaging_job_name, build_number)
    package_names = get_package_names(branch, major_version, minor_version, INCLUDE_FILTER, build_number)
    with open(os.path.join(WORKSPACE, EMAIL_TEMPLATE_FILE), 'w+') as output:
        if len(package_names) > 0:
            package_list_str = '\n'.join(package_names)
            changelists_str = ''
            for item in changelists:
                changelists_str += '---------------------------------------------------------------------------------------------\n'
                try:
                    changelists_str += 'CL{} by {} on {}\n{}\n'.format(item['changeNumber'], item['author']['fullName'], item['changeTime'], item['msg'].encode('utf-8', 'ignore'))
                except KeyError:
                    error('Internal error, check the output of Jenkins API.')
            output.write(EMAIL_TEMPLATE.format(S3_TARGET, package_list_str, changelists_str))


