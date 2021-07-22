"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Configurations for the fleets to deploy.
# Modify the fleet configuration fields below before deploying the CDK application.
# Customers can define multiple fleets by copying the existing configuration template below and
# append the new fleet configuration to the FLEET_CONFIGURATIONS list. All the fleets in the list
# will be deployed automatically by this CDK application.
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
                # Choose from SIMPLE or TERMINAL.
                'type': 'SIMPLE'
            }
        },
        # (Required) Information about a game server build that is installed and
        # run on instances in an Amazon GameLift fleet.
        'build_configuration': {
            # (Conditional) A unique identifier for a build to be deployed on the new fleet.
            # This parameter is required unless the parameters build_path and operating_system are defined and
            # the conditional variable upload-with-support-stack is set to true
            'build_id': '<build id>',
            # (Conditional) The disk location of the local build file(.zip).
            # This parameter is required unless the parameter build_id is defined.
            'build_path': '<build path>',
            # (Conditional) The operating system that the game server binaries are built to run on.
            # This parameter is required if the parameter build_path is defined.
            # Choose from AMAZON_LINUX, AMAZON_LINUX or WINDOWS_2012.
            'operating_system': 'WINDOWS_2012'
        },
        # (Optional) Information about the use of a TLS/SSL certificate for a fleet.
        'certificate_configuration': {
            # (Required) Indicates whether a TLS/SSL certificate is generated for the fleet.
            # Choose from DISABLED or GENERATED.
            'certificate_type': 'DISABLED',
        },
        # A human-readable description of the fleet.
        'description': 'Amazon GameLift fleet to host game servers.',
        # (Optional) A range of IP addresses and port settings that allow inbound traffic to connect to
        # server processes on an Amazon GameLift server.
        # This should be the same port range as the server is configured for.
        'ec2_inbound_permissions': [
            {
                # (Required) A starting value for a range of allowed port numbers.
                # 33450 is the default server port defined by the Multiplayer Gem.
                'from_port': 33450,
                # (Required) A range of allowed IP addresses.
                'ip_range': '<ip range>',
                # (Required) The network communication protocol used by the fleet.
                'protocol': 'UDP',
                # (Required) An ending value for a range of allowed port numbers.
                'to_port': 33450
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
        # Choose from the available EC2 instance type list: https://aws.amazon.com/ec2/instance-types/
        'ec2_instance_type': 'c5.large',
        # (Optional) Indicates whether to use On-Demand or Spot instances for this fleet.
        # Choose from ON_DEMAND or SPOT
        'fleet_type': 'ON_DEMAND',
        # (Optional) A game session protection policy to apply to all game sessions hosted on instances in this fleet.
        # Choose from FullProtection or NoProtection
        'new_game_session_protection_policy': 'NoProtection',
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
                    # (Required) The location of a game build executable that contains the Init() function.
                    # Game builds are installed on instances at the root:
                    # Windows (custom game builds only): C:\game.
                    # Linux: /local/game.
                    'launch_path': 'C:\\game\\bin\\server.exe',
                    # (Optional) An optional list of parameters to pass to the server executable on launch.
                    'parameters': '--sv_port 33450 --project-path=C:\\game '
                                  '--project-cache-path=C:\\game\\assets --engine-path=C:\\game '
                                  '-bg_ConnectToAssetProcessor=0'
                }
            ]
        }
        # For additional fleet configurations, please check:
        # https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/AWS_GameLift.html
    }
]
