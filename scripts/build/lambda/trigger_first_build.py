#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import boto3
import logging
import json
import os
import requests

from botocore.exceptions import ClientError
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry
from requests.exceptions import RetryError

log = logging.getLogger(__name__)
log.setLevel(logging.INFO)

JENKINS_CREDENTIAL_STORE = os.environ['JENKINS_CREDENTIAL_STORE']
JENKINS_ENDPOINT = os.environ['JENKINS_ENDPOINT']
REGION = os.environ['REGION']


def get_secret(secret_name):
    secrets = boto3.client(service_name='secretsmanager', region_name=REGION)
    try:
        response = secrets.get_secret_value(SecretId=secret_name)
        return json.loads(response['SecretString'])
    except ClientError as e:
        log.error(f'Unable to get secret value: {e}')
        return None


def lambda_handler(event, context):
    event_record = event['Records'][0]
    log.info(f'Received Event: {event_record}')

    ref_prefix = "refs/heads/"
    branch_name = event_record['codecommit']['references'][0]['ref'][len(ref_prefix):]
    repo_name = event_record['eventSourceARN'].split(":")[-1]

    credentials = get_secret(JENKINS_CREDENTIAL_STORE)

    if credentials:
        retries = 3
        backoff = 30
        status_list = [404]  # Retry if the branch doesn't exist yet and provide time for Jenkins to discover it.
        method_list = ['POST']
        retry_config = Retry(total=retries, backoff_factor=backoff, status_forcelist=status_list, allowed_methods=method_list)

        session = requests.Session()
        session.mount('https://', HTTPAdapter(max_retries=retry_config))

        encoded_branch_name = requests.utils.quote(branch_name, safe='')
        build_path = f'/job/{repo_name}/job/{encoded_branch_name}/build'
        jenkins_url = requests.compat.urljoin(JENKINS_ENDPOINT, build_path)

        try:
            response = session.post(jenkins_url, auth=(credentials['username'], credentials['apitoken']))
            if response.status_code == 201:
                log.info(f'Successfully triggered build on {repo_name}/{branch_name}')
            elif response.status_code == 400:
                log.info(f'Initial build already started for {repo_name}/{branch_name}. Parameters are already available.')
            else:
                log.error(f'Failed to start build on {repo_name}/{branch_name}. Status code: {response.status_code}')
        except RetryError as e:
            log.error(f'Pipeline for {repo_name}/{branch_name} does not exist in Jenkins: {e}')
