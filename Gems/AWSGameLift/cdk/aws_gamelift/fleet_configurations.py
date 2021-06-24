"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Configurations for the fleets to deploy.
# Modify the fleet configuration fields below before deploying the CDK application.
# To select the right combination of hosting resources and learn how to configure them to best suit to your application,
# please check: https://docs.aws.amazon.com/gamelift/latest/developerguide/fleets-design.html
FLEET_CONFIGURATIONS = [
    {
        # (Optional) An alias for an Amazon GameLift fleet destination.
        # By using aliases instead of specific fleet IDs, customers can more easily and seamlessly switch
        # player traffic from one fleet to another by changing the alias's target location.
        'alias_configuration': {
            # (Required) A descriptive label that is associated with an alias. Alias names do not need to be unique.
            'name': '<alias name>',
            # (Conditional) A type of routing strategy for the GameLift fleet alias if exists.
            # Required if alias_configuration is provided.
            'routing_strategy': {
                # The message text to be used with a terminal routing strategy.
                # If you specify TERMINAL for the Type property, you must specify this property.
                # Required if specify TERMINAL for the Type property,
                'message': '<routing strategy message>',
                # (Required) A type of routing strategy.
                'type': 'SIMPLE | TERMINAL'
            }
        },
        # (Required) Information about a game server build that is installed and
        # run on instances in an Amazon GameLift fleet.
        'build_configuration': {
            # (Conditional) A unique identifier for a build to be deployed on the new fleet.
            # This parameter is required unless the parameters build_path and operating_system are defined and
            # the conditional variable upload-with-support-stack is set to true
            'build_id': '<build id>',
            # (Conditional) The disk location of the local build file(zip).
            # This parameter is required unless the parameter build_id is defined.
            'build_path': '<build path>',
            # (Conditional) The operating system that the game server binaries are built to run on.
            # This parameter is required if the parameter build_path is defined.
            'operating_system': 'AMAZON_LINUX | AMAZON_LINUX_2 | WINDOWS_2012'
        },
        # (Optional) Information about the use of a TLS/SSL certificate for a fleet.
        'certificate_configuration': {
            # (Required) Indicates whether a TLS/SSL certificate is generated for the fleet.
            'certificate_type': 'DISABLED | GENERATED',
        },
        # A human-readable description of the fleet.
        'description': 'Amazon GameLift fleet to host game servers.',
        # (Optional) A range of IP addresses and port settings that allow inbound traffic to connect to
        # server processes on an Amazon GameLift server.
        # This should be the same port range as the server is configured for.
        'ec2_inbound_permissions': [
            {
                # (Required) A starting value for a range of allowed port numbers.
                # 30090 is the default server port defined by the Multiplayer Gem.
                'from_port': 30090,
                # (Required) A range of allowed IP addresses.
                'ip_range': '<ip range>',
                # (Required) The network communication protocol used by the fleet.
                'protocol': 'UDP',
                # (Required) An ending value for a range of allowed port numbers.
                'to_port': 30090
            },
            {
                # Open the debug port for remote into a Windows fleet.
                'from_port': 3389,
                'ip_range': '<external ip range>',
                'protocol': 'TCP',
                'to_port': 3389
            },
            {
                # Open the debug port for remote into a Linux fleet.
                'from_port': 22,
                'ip_range': '<external ip range>',
                'protocol': 'TCP',
                'to_port': 22
            }
        ],
        # (Optional) The GameLift-supported EC2 instance type to use for all fleet instances.
        'ec2_instance_type': 'c3.2xlarge | c3.4xlarge | c3.8xlarge | c3.large | c3.xlarge | c4.2xlarge | c4.4xlarge |'
                             ' c4.8xlarge | c4.large | c4.xlarge | c5.12xlarge | c5.18xlarge | c5.24xlarge |'
                             ' c5.2xlarge | c5.4xlarge | c5.9xlarge | c5.large | c5.xlarge | c5a.12xlarge |'
                             ' c5a.16xlarge | c5a.24xlarge | c5a.2xlarge | c5a.4xlarge | c5a.8xlarge | c5a.large |'
                             ' c5a.xlarge | m3.2xlarge | m3.large | m3.medium | m3.xlarge | m4.10xlarge | m4.2xlarge |'
                             ' m4.4xlarge | m4.large | m4.xlarge | m5.12xlarge | m5.16xlarge | m5.24xlarge |'
                             ' m5.2xlarge | m5.4xlarge | m5.8xlarge | m5.large | m5.xlarge | m5a.12xlarge |'
                             ' m5a.16xlarge | m5a.24xlarge | m5a.2xlarge | m5a.4xlarge | m5a.8xlarge | m5a.large |'
                             ' m5a.xlarge | r3.2xlarge | r3.4xlarge | r3.8xlarge | r3.large | r3.xlarge | r4.16xlarge |'
                             ' r4.2xlarge | r4.4xlarge | r4.8xlarge | r4.large | r4.xlarge | r5.12xlarge |'
                             ' r5.16xlarge | r5.24xlarge | r5.2xlarge | r5.4xlarge | r5.8xlarge | r5.large |'
                             ' r5.xlarge | r5a.12xlarge | r5a.16xlarge | r5a.24xlarge | r5a.2xlarge | r5a.4xlarge |'
                             ' r5a.8xlarge | r5a.large | r5a.xlarge | t2.large | t2.medium | t2.micro | t2.small',
        # (Optional) Indicates whether to use On-Demand or Spot instances for this fleet.
        'fleet_type': 'ON_DEMAND | SPOT',
        # (Optional) A game session protection policy to apply to all game sessions hosted on instances in this fleet.
        'new_game_session_protection_policy': 'FullProtection | NoProtection',
        # (Optional) A policy that limits the number of game sessions that an individual player
        # can create on instances in this fleet within a specified span of time.
        'resource_creation_limit_policy': {
            # (Optional) The maximum number of game sessions that an individual can create during the policy period.
            # Provide any integer not less than 0.
            'new_game_sessions_per_creator': 3,
            # (Optional) The time span used in evaluating the resource creation limit policy.
            # Provide any integer not less than 0.
            'policy_period_in_minutes': 15
        },
        # (Conditional) Instructions for launching server processes on each instance in the fleet.
        # This parameter is required unless the parameters ServerLaunchPath and ServerLaunchParameters are defined.
        'runtime_configuration': {
            # (Optional) The maximum amount of time (in seconds) allowed to launch a new game session and
            # have it report ready to host players.
            # Provide an integer from 1 to 600.
            'game_session_activation_timeout_seconds': 300,
            # (Optional) The number of game sessions in status ACTIVATING to allow on an instance.
            # Provide an integer from 1 to 2147483647.
            'max_concurrent_game_session_activations': 2,
            # (Optional) A collection of server process configurations that identify what server processes
            # to run on each instance in a fleet. To set up a fleet's runtime configuration to
            # run multiple game server processes per instance, please check the following document:
            # https://docs.aws.amazon.com/gamelift/latest/developerguide/fleets-multiprocess.html
            'server_processes': [
                {
                    # (Required) The number of server processes using this configuration that
                    # run concurrently on each instance.
                    # Provide any integer not less than 1.
                    'concurrent_executions': 1,
                    # (Required) The location of a game build executable or the Realtime script file that
                    # contains the Init() function.
                    'launch_path': '(Windows) <C:\\game\\<executable or script> | '
                                   '(Linux) /local/game/MyGame/<executable or script>',
                    # (Optional) An optional list of parameters to pass to the server executable
                    # or Realtime script on launch.
                    'parameters': '<launch parameters>'
                }
            ]
        }
        # For additional fleet configurations, please check:
        # # https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/AWS_GameLift.html
    }
]
