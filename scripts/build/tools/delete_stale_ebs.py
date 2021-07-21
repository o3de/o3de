#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import requests
import traceback
import boto3
import json
from datetime import datetime
from requests.auth import HTTPBasicAuth
from urllib.parse import unquote


class JenkinsAPIClient:
    def __init__(self, jenkins_base_url, jenkins_username, jenkins_api_token):
        self.jenkins_base_url = jenkins_base_url.rstrip('/')
        self.jenkins_username = jenkins_username
        self.jenkins_api_token = jenkins_api_token

    def get(self, url, retry=1):
        for i in range(retry):
            try:
                response = requests.get(url, auth=HTTPBasicAuth(self.jenkins_username, self.jenkins_api_token))
                if response.ok:
                    return response.json()
            except Exception:
                traceback.print_exc()
                print(f'WARN: Get request {url} failed, retying....')
        print(f'WARN: Get request {url} failed, see exception for more details.')

    def get_pipeline(self, pipeline_name):
        url = f'{self.jenkins_base_url}/job/{pipeline_name}/api/json'
        # Use retry because Jenkins API call sometimes may fail when Jenkins server is on high load
        return self.get(url, retry=3)

    def get_branch_job(self, pipeline_name, branch_name):
        url = f'{self.jenkins_base_url}/blue/rest/organizations/jenkins/pipelines/{pipeline_name}/branches/{branch_name}'
        # Use retry because Jenkins API call sometimes may fail when Jenkins server is on high load
        return self.get(url, retry=3)


def delete_branch_ebs_volumes(branch_name):
    """
    Make a fake branch deleted event and invoke the lambda function that we use to delete branch EBS volumes after a branch is deleted.
    """
    # Unescape branch name as it's URL encoded
    branch_name = unquote(branch_name)
    input = {
        "detail": {
            "event": "referenceDeleted",
            "repositoryName": "Lumberyard",
            "referenceName": branch_name
        }
    }
    client = boto3.client('lambda')
    # Invoke lambda function "AutoDeleteEBS-Lambda" asynchronously.
    # This lambda function can have 1000 concurrent runs.
    # we will setup a SQS/SNS queue to process the events if the event number exceeds the function capacity.
    client.invoke(
        FunctionName='AutoDeleteEBS-Lambda',
        InvocationType='Event',
        Payload=json.dumps(input),
    )


def delete_old_branch_ebs_volumes(env):
    """
    Check last run time of each branch build, if it exceeds the retention days, delete the EBS volumes that are tied to the branch.
    """
    branch_volumes_deleted = []
    jenkins_client = JenkinsAPIClient(env['JENKINS_URL'], env['JENKINS_USERNAME'], env['JENKINS_API_TOKEN'])
    today_date = datetime.today().date()
    pipeline_name = env['PIPELINE_NAME']
    pipeline_job = jenkins_client.get_pipeline(pipeline_name)
    if not pipeline_job:
        print(f'ERROR: Cannot get data of pipeline job {pipeline_name}.')
        exit(1)
    branch_jobs = pipeline_job.get('jobs', [])
    retention_days = int(env['RETENTION_DAYS'])
    for branch_job in branch_jobs:
        branch_name = branch_job['name']
        branch_job = jenkins_client.get_branch_job(pipeline_name, branch_name)
        if not branch_job:
            print(f'WARN: Cannot get data of {branch_name} job , skipping branch {pipeline_name}.')
            continue
        latest_run = branch_job.get('latestRun')
        # If the job hasn't run, then there is no EBS volumes tied to that job
        if latest_run:
            latest_run_start_time = latest_run.get('startTime')
            latest_run_datetime = datetime.strptime(latest_run_start_time, '%Y-%m-%dT%H:%M:%S.%f%z')
            # Convert startTime to local timezone to compare, because Jenkins server may use a different timezone.
            latest_run_date = latest_run_datetime.astimezone().date()
            date_diff = today_date - latest_run_date
            if date_diff.days > retention_days:
                print(f'Branch {branch_name} job hasn\'t run for over {retention_days} days, deleting the EBS volumes of this branch...')
                delete_branch_ebs_volumes(branch_name)
                branch_volumes_deleted.append(branch_name)
    print('Deleted EBS volumes for branches:')
    print('\n'.join(branch_volumes_deleted))


def get_required_env(env, keys):
    success = True
    for key in keys:
        try:
            env[key] = os.environ[key].strip()
        except KeyError:
            print(f'ERROR: {key} is not set in environment variable')
            success = False
    return success


def main():
    env = {}
    required_env_list = [
        'JENKINS_URL',
        'JENKINS_USERNAME',
        'JENKINS_API_TOKEN',
        'PIPELINE_NAME',
        'RETENTION_DAYS'
    ]
    if not get_required_env(env, required_env_list):
        print('ERROR: Required environment variable is not set, see log for more details.')

    delete_old_branch_ebs_volumes(env)


if __name__ == "__main__":
    main()
