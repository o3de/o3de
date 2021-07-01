#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import boto3
import time
import logging

TIMEOUT = 300
log = logging.getLogger(__name__)
log.setLevel(logging.INFO)


def delete_ebs_volumes(repository_name, branch_name):
    """
    Delete all EBS volumes that are tagged with repository_name and branch_name
    :param repository_name: Full repository name.
    :param branch_name: Branch name that is deleted.
    :return: Number of EBS volumes that are deleted successfully, number of EBS volumes that are not deleted.
    """
    success = 0
    failure = 0
    ec2_client = boto3.resource('ec2')
    response = ec2_client.volumes.filter(Filters=[
        {
            'Name': 'tag:RepositoryName',
            'Values': [repository_name]
        },
        {
            'Name': 'tag:BranchName',
            'Values': [branch_name]
        }
    ])
    log.info(f'Deleting EBS volumes for remote-branch {branch_name} in repository {repository_name}.')
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
            success += 1
        except Exception as e:
            log.error(f'Failed to delete volume {volume.volume_id}.')
            log.error(e)
            failure += 1
    return success, failure


def lambda_handler(event, context):
    repository_name = event['repository_name']
    branch_name = event['branch_name']
    (success, failure) = delete_ebs_volumes(repository_name, branch_name)
    return {
        'success': success,
        'failure': failure
    }
