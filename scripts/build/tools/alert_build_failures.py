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

import os
from incremental_build_util import get_iam_role_credentials
try:
    import requests
except ImportError:
    import pip
    pip.main(['install', 'requests', '--ignore-installed', '-q'])
    import requests
try:
    from requests_aws4auth import AWS4Auth
except ImportError:
    import pip
    pip.main(['install', 'requests_aws4auth', '--ignore-installed', '-q'])
    from requests_aws4auth import AWS4Auth

IAM_ROLE_NAME = 'ec2-jenkins-node'
TEAM = 'lumberyard-build'
CHIME_ROOM_WEB_HOOK = "https://hooks.chime.aws/incomingwebhooks/2a6018c9-3bf5-4e03-851c-32ba82c7c4e2?token=YWhTVXZsWVJ8MXxaLTg5RkVZNlA5Q1NiMVdfQndGeS1TSHNnYW5VREVha3pjX1pUUEd5b1JF"


def find_curr_oncalls_for_team(team):
    host = "who-is-oncall-pdx.corp.amazon.com"
    service_name = "who-is-oncall"
    aws_region = "us-west-2"
    headers = {"content-type": "application/json", "host": host}

    credentials = get_iam_role_credentials(IAM_ROLE_NAME)
    try:
        aws_access_key_id = credentials['AccessKeyId']
        aws_secret_access_key = credentials['SecretAccessKey']
        aws_session_token = credentials['Token']
    except Exception as e:
        print(f'ERROR: Cannot get AWS credentials.\n{e}')
        return ['All']

    auth = AWS4Auth(aws_access_key_id, aws_secret_access_key, aws_region, service_name, session_token=aws_session_token)
    r = requests.get(f"https://who-is-oncall-pdx.corp.amazon.com/teams/{team}", headers=headers, auth=auth, verify=False)
    if r.ok:
        res = r.json()
        try:
            return res['currOncalls']
        except KeyError:
            return ['All']
    return ['All']


def send_alert_to_chime_room(web_hook, content):
    data = '{"Content":"' + content + '"}'
    headers = {'Content-Type': 'application/json'}
    requests.post(web_hook, headers=headers, data=data)


def create_content():
    content = ''
    oncalls = find_curr_oncalls_for_team(TEAM)
    for oncall in oncalls:
        content += f'@{oncall} '
    job_name = os.environ['JOB_NAME']
    build_url = os.environ['BUILD_URL']
    content += fr'\nJob {job_name} failed\nBuild URL: {build_url}\n'
    return content


send_alert_to_chime_room(CHIME_ROOM_WEB_HOOK, create_content())

