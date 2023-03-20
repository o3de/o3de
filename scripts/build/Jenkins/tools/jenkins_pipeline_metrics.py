#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import json
import requests
import traceback
import csv
import sys
from datetime import datetime, timezone
from requests.auth import HTTPBasicAuth

cur_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.abspath(f'{cur_dir}/../../../util'))
import util

class JenkinsAPIClient:
    def __init__(self, jenkins_base_url, jenkins_username, jenkins_api_token):
        self.jenkins_base_url = jenkins_base_url.rstrip('/')
        self.jenkins_username = jenkins_username
        self.jenkins_api_token = jenkins_api_token
        self.blueocean_api_path = '/blue/rest/organizations/jenkins/pipelines'

    def get_request(self, url, retry=1):
        for i in range(retry):
            try:
                response = requests.get(url, auth=HTTPBasicAuth(self.jenkins_username, self.jenkins_api_token))
                if response.ok:
                    return response.json()
            except Exception:
                traceback.print_exc()
                print(f'WARN: Get request {url} failed, retying....')
        util.error(f'Get request {url} failed, see exception for more details.')

    def get_builds(self, pipeline_name, branch_name=''):
        url = self.jenkins_base_url + self.blueocean_api_path + f'/{pipeline_name}/{branch_name}/runs'
        return self.get_request(url, retry=3)

    def get_stages(self, build_number, pipeline_name, branch_name=''):
        url = self.jenkins_base_url + self.blueocean_api_path + f'/{pipeline_name}/{branch_name}/runs/{build_number}/nodes'
        return self.get_request(url, retry=3)


def generate_build_metrics_csv(env, target_date):
    output = []
    jenkins_client = JenkinsAPIClient(env['JENKINS_URL'], env['JENKINS_USERNAME'], env['JENKINS_API_TOKEN'])
    pipeline_name = env['PIPELINE_NAME']
    builds = jenkins_client.get_builds(pipeline_name)
    days_to_collect = env['DAYS_TO_COLLECT']
    for build in builds:
        build_time = datetime.strptime(build['startTime'], '%Y-%m-%dT%H:%M:%S.%f%z')
        # Convert startTime to local timezone to compare, because Jenkins server may use a different timezone.
        build_date = build_time.astimezone().date()
        date_diff = target_date - build_date
        # Only collect build result of past {days_to_collect} days.
        if date_diff.days > int(days_to_collect):
            break
        stages = jenkins_client.get_stages(build['id'], pipeline_name)
        stage_dict = {}
        parallel_stages = []
        # Build stage_dict and find all parallel stages
        for stage in stages:
            stage_dict[stage['id']] = stage
            if stage['type'] == 'PARALLEL':
                parallel_stages.append(stage)
        # Calculate build metrics grouped by parallel stage
        def stage_duration_sum(stage):
            duration_sum = stage['durationInMillis']
            for edge in stage['edges']:
                downstream_stage = stage_dict[edge['id']]
                duration_sum += stage_duration_sum(downstream_stage)
            return duration_sum
        for parallel_stage in parallel_stages:
            try:
                build_info = {
                    'job_name': parallel_stage['displayName'],
                    'view_name': env['BRANCH_NAME']
                }
                # UTC datetime is required by BI team
                build_info['build_time'] = datetime.fromtimestamp(build_time.timestamp(), timezone.utc)
                build_info['build_number'] = build['id']
                build_info['job_duration'] = stage_duration_sum(parallel_stage) / 1000 / 60
                build_info['status'] = parallel_stage['result']
                build_info['clean_build'] = 'True'
                print(build_info)
                output.append(build_info)
            except Exception:
                traceback.print_exc()

    if output:
        with open('spectra_build_metrics.csv', 'w', newline='') as csvfile:
            fieldnames = list(output[0].keys())
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(output)


def generate_build_metrics_manifest(csv_s3_location):
    data = {
                "entries": [
                    {
                        "url": csv_s3_location
                    }
                ]
    }
    with open('spectra_build_metrics.manifest', 'w') as manifest:
        json.dump(data, manifest)


def upload_files_to_s3(env, formatted_date):
    csv_s3_prefix = f"{env['CSV_PREFIX'].rstrip('/')}/{formatted_date}"
    manifest_s3_prefix = f"{env['MANIFEST_PREFIX'].rstrip('/')}/{formatted_date}"
    upload_to_s3_script_path = os.path.join(cur_dir, 'upload_to_s3.py')
    engine_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))
    if sys.platform == 'win32':
        python = os.path.join(engine_root, 'python', 'python.cmd')
    else:
        python = os.path.join(engine_root, 'python', 'python.sh')
    upload_csv_cmd = [python, upload_to_s3_script_path, '--base_dir', cur_dir, '--file_regex', env['CSV_REGEX'], '--bucket', env['BUCKET'], '--key_prefix', csv_s3_prefix]
    util.execute_system_call(upload_csv_cmd)
    upload_manifest_cmd = [python, upload_to_s3_script_path, '--base_dir', cur_dir, '--file_regex', env['MANIFEST_REGEX'], '--bucket', env['BUCKET'], '--key_prefix', manifest_s3_prefix]
    util.execute_system_call(upload_manifest_cmd)


def get_required_env(env, keys):
    success = True
    for key in keys:
        try:
            env[key] = os.environ[key].strip()
        except KeyError:
            util.error(f'{key} is not set in environment variable')
            success = False
    return success


def main():
    env = {}
    required_env_list = ['JENKINS_URL', 'PIPELINE_NAME', 'BRANCH_NAME', 'JENKINS_USERNAME', 'JENKINS_API_TOKEN', 'BUCKET', 'CSV_REGEX', 'CSV_PREFIX', 'MANIFEST_REGEX', 'MANIFEST_PREFIX', 'DAYS_TO_COLLECT']
    if not get_required_env(env, required_env_list):
        util.error('Required environment variable is not set, see log for more details.')

    target_date = datetime.today().date()
    formatted_date = f'{target_date.year}/{target_date:%m}/{target_date:%d}'
    csv_s3_location = f"s3://{env['BUCKET']}/{env['CSV_PREFIX'].rstrip('/')}/{formatted_date}/spectra_build_metrics.csv"

    generate_build_metrics_csv(env, target_date)
    generate_build_metrics_manifest(csv_s3_location)
    upload_files_to_s3(env, formatted_date)


if __name__ == "__main__":
    main()
