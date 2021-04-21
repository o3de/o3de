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
import boto3
import json
import hmac
import hashlib


def delete_volumes(repository_name, branch_name):
    """
    Trigger lambda function that deletes EBS volumes.
    :param repository_name: Full repository name.
    :param branch_name: Branch name that is deleted.
    :return: Number of EBS volumes that are deleted successfully, number of EBS volumes that are not deleted.
    """
    client = boto3.client('lambda')
    payload = {
        'repository_name': repository_name,
        'branch_name': branch_name
    }
    response = client.invoke(
        FunctionName='delete_branch_ebs',
        Payload=json.dumps(payload),
    )
    status = response['Payload'].read()
    response_json = json.loads(status.decode())
    return response_json['success'], response_json['failure']


def verify_signature(headers, payload):
    """
    Validate POST request headers and payload to only receive the expected GitHub webhook requests.
    :param headers: Headers from POST request.
    :param payload: Payload from POST request.
    :return: True if request is verified, otherwise, return False.
    """
    # secret is stored in AWS Secret Manager
    secret_name = os.environ.get('GITHUB_WEBHOOK_SECRET_NAME', '')
    client = boto3.client(service_name='secretsmanager')
    response = client.get_secret_value(SecretId=secret_name)
    secret = response['SecretString']
    # Using X-Hub-Signature-256 is recommended by https://docs.github.com/en/developers/webhooks-and-events/securing-your-webhooks
    signature = headers.get('X-Hub-Signature-256', '')
    computed_hash = hmac.new(secret.encode(), payload.encode(), hashlib.sha256).hexdigest()
    computed_signature = f'sha256={computed_hash}'
    return hmac.compare_digest(computed_signature.encode(), signature.encode())


def create_response(status, success=0, failure=0, repository_name=None, branch_name=None):
    """
    :param status: Status of EBS deletion request.
    :param success: Number of EBS volumes that are deleted successfully.
    :param failure: Number of EBS volumes that are not deleted.
    :param repository_name: Full repository name.
    :param branch_name: Branch name that is deleted.
    :return: JSON response.
    """
    response = {
        'success': {
            'statusCode': 200,
            'body': f'[SUCCESS] All {success + failure} EBS volumes are deleted for branch {branch_name} in repository {repository_name}',
            'isBase64Encoded': 'false'
        },
        'failure': {
            'statusCode': 500,
            'body': f'[FAILURE] Failed to delete {failure}/{success + failure} EBS volumes for branch {branch_name} in repository {repository_name}',
            'isBase64Encoded': 'false'
        },
        'unauthorized': {
            'statusCode': 401,
            'body': 'Unauthorized',
            'isBase64Encoded': 'false'
        }
    }
    return response[status]


def lambda_handler(event, context):
    # This function is triggered by AWS API Gateway,
    if event.get('resource', '') == '/delete-github-branch-ebs':
        headers = event['headers']
        payload = event['body']
        if headers['X-GitHub-Event'] == 'delete':
            # Validate github webhook request here since request body cannot be passed to API Gateway lambda authorizer.
            if verify_signature(headers, payload):
                # Convert payload from string type to json to get repository name and branch name
                payload = json.loads(payload)
                repository_name = payload['repository']['full_name']
                branch_name = payload['ref']
                (success, failure) = delete_volumes(repository_name, branch_name)
                if not failure:
                    return create_response('success', success, failure, repository_name, branch_name)
                else:
                    return create_response('failure', success, failure, repository_name, branch_name)
            else:
                return create_response('unauthorized')
