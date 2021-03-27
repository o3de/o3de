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

import boto3
import time
import logging

TIMEOUT = 300


def lambda_handler(event, context):
    log = logging.getLogger(__name__)
    log.setLevel(logging.INFO)
    branch_name = event['detail']['referenceName']
    ec2_client = boto3.resource('ec2')
    response = ec2_client.volumes.filter(Filters=[
        {
            'Name': 'tag:BranchName',
            'Values': [branch_name]
        }
    ])
    log.info(f'Deleting EBS volumes for remote-branch {branch_name}.')
    for volume in response:
        if volume.attachments:
            ec2_instance_id = volume.attachments[0]['InstanceId']
            try:
                log.info(f'Detaching volume {volume.volume_id} from ec2 {ec2_instance_id}.')
                volume.detach_from_instance(Device='xvdf',
                                            Force=True,
                                            InstanceId=ec2_instance_id,
                                            VolumeId=volume.volume_id)
            except Exception as e:
                log.error(f'Failed to detach volume {volume.volume_id} from {ec2_instance_id}.')
                log.error(e)
            timeout_init = time.clock()
            while len(volume.attachments) and volume.attachments[0]['State'] != 'detached':
                time.sleep(1)
                volume.load()
                if (time.clock() - timeout_init) > TIMEOUT:
                    log.error('Timeout reached trying to detach EBS.')
        try:
            log.info(f'Deleting volume {volume.volume_id}')
            volume.delete()
        except Exception as e:
            log.error(f'Failed to delete volume {volume.volume_id}.')
            log.error(e)


lambda_handler(event, context)