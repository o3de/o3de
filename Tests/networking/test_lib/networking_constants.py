"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import os

DEFAULT_REGION = 'us-west-2'
DEFAULT_PROFILE = 'LYQA-Gamelift'
LINUX_TAR_BUCKET = 'ly-gamelift-linux-packages'
DEFAULT_LINUX_WORKSPACE = os.path.join('/home', 'ubuntu', 'workspace')
LINUX_GAMELIFT_WORKFLOW_CONFIG_FILENAME = 'gamelift_workflow_config.ini'
LINUX_GAMELIFT_WORKFLOW_CONFIG_SECTION = 'LinuxGameLiftTest'
LINUX_GAMELIFT_WORKFLOW_CONFIG_TARFILE_KEY = 'TAR_FILENAME'
LINUX_GAMELIFT_WORKFLOW_CONFIG_FLEET_KEY = 'FLEET_ID'
WINDOWS_GAMELIFT_FLEET = {
    'fleet_name': 'WindowsGameliftFleet',
    'ec2_instance_type': 'c4.large',
    'runtime_configuration': {
        'ServerProcesses': [
                {
                    'LaunchPath': None,  # Override this value with os.path.join("C:", "game", <BIN.DEDICATED_DIR>,
                    # "MultiplayerSampleLauncher_Server.exe")
                    'Parameters': '+sv_port 33435 +gamelift_start_server',
                    'ConcurrentExecutions': 1,
                },
{
                    'LaunchPath': None,
                    'Parameters': '+sv_port 33436 +gamelift_start_server',
                    'ConcurrentExecutions': 1,
                },
                {
                    'LaunchPath': None,
                    'Parameters': '+sv_port 33437 +gamelift_start_server',
                    'ConcurrentExecutions': 1,
                }
            ]
    },
    'ec2_inbound_permissions': [  # Ports & IP's for clients to connect
        {
            'FromPort': 33435,
            'ToPort': 33437,
            'IpRange': '0.0.0.0/0',  # Override this value with client IP
            'Protocol': 'UDP',
        }
    ]
}
WINDOWS_CUSTOMBACKFILL_GAMELIFT_FLEET = {
    'fleet_name': 'WindowsGameliftFleetCustomBackfill',
    'ec2_instance_type': 'c4.large',
    'runtime_configuration': {
        'ServerProcesses': [
                {
                    'LaunchPath': None,  # Override this value with os.path.join("C:", "game", <BIN.DEDICATED_DIR>,
                    # "MultiplayerSampleLauncher_Server.exe")
                    'Parameters': '+sv_port 33435 +gamelift_start_server +gamelift_flexmatch_enable 1 '
                                  '+gamelift_flexmatch_onplayerremoved_enable 1 '
                                  '+gamelift_flexmatch_minimumplayersessioncount 2',
                    'ConcurrentExecutions': 1,
                }
            ]
    },
    'ec2_inbound_permissions': [  # Ports & IP's for clients to connect
        {
            'FromPort': 33435,
            'ToPort': 33435,
            'IpRange': '0.0.0.0/0',  # Override this value with client IP
            'Protocol': 'UDP',
        }
    ]
}
LINUX_GAMELIFT_FLEET = {
    'fleet_name': 'LinuxGameliftFleet',
    'ec2_instance_type': 'c5.large',
    'runtime_configuration': {
        "GameSessionActivationTimeoutSeconds": 300,
        "MaxConcurrentGameSessionActivations": 3,
        "ServerProcesses": [
            {
                "LaunchPath": "/local/game/./MultiplayerSampleLauncher_Server",
                "Parameters": "+sv_port 33435 +map multiplayersample +gamelift_start_server true",
                "ConcurrentExecutions": 1
            }
        ]
    },
    'ec2_inbound_permissions': [  # Ports & IP's for clients to connect
        {
            'FromPort': 33435,
            'ToPort': 33437,
            'IpRange': '0.0.0.0/0',  # Override this value with client IP
            'Protocol': 'UDP',
        }
    ]
}
