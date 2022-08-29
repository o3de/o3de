"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_gamelift as gamelift
)

from .flexmatch import matchmaking
from .game_session_queue import game_session_queue


class GameLiftStack(core.Stack):
    """
    The AWS GameLift stack

    Defines GameLift resources to use in project
    """
    def __init__(self, scope: core.Construct, id_: str,
                 stack_name: str, fleet_configurations: dict,
                 create_game_session_queue: bool,
                 flex_match: bool,
                 **kwargs) -> None:
        super().__init__(scope, id_, **kwargs)

        self._stack_name = stack_name

        fleet_ids = []
        queue_destinations = []
        for index in range(len(fleet_configurations)):
            fleet_configuration = fleet_configurations[index]
            # Create a new GameLift fleet using the configuration
            fleet_ids.append(self._create_fleet(fleet_configuration, index).attr_fleet_id)
            destination_arn = core.Fn.sub(
                body='arn:${AWS::Partition}:gamelift:${AWS::Region}::fleet/${FleetId}',
                variables={
                    'FleetId': fleet_ids[index],
                }
            )

            if fleet_configuration.get('alias_configuration'):
                # Create an alias for the fleet if the alias configuration is provided
                alias = self._create_alias(fleet_configuration['alias_configuration'], fleet_ids[index])
                destination_arn = core.Fn.sub(
                    body='arn:${AWS::Partition}:gamelift:${AWS::Region}::alias/${AliasId}',
                    variables={
                        'AliasId': alias.attr_alias_id,
                    }
                )

            queue_destinations.append(destination_arn)

        # Export the GameLift fleet ids as a stack output
        core.CfnOutput(
            self,
            id='GameLiftFleets',
            description='List of GameLift fleet ids',
            export_name=f'{self._stack_name}:GameLiftFleets',
            value=','.join(fleet_ids)
        )

        game_session_queue_arns = []
        if flex_match or create_game_session_queue:
            queue = game_session_queue.GameSessionQueueResources(self, queue_destinations)
            game_session_queue_arns.append(queue.game_session_queue_arn)

        if flex_match:
            matchmaking.MatchmakingResoures(self, game_session_queue_arns)

    def _create_fleet(self, fleet_configuration: dict, identifier: int) -> gamelift.CfnFleet:
        """
        Create an Amazon GameLift fleet to host game servers.
        :param fleet_configuration: Configuration of the fleet.
        :param identifier: Unique identifier of the fleet which will be included in the resource id.
        :return: Generated GameLift fleet.
        """
        fleet = gamelift.CfnFleet(
            self,
            id=f'{self._stack_name}-GameLiftFleet{identifier}',
            build_id=self._get_gamelift_build_id(fleet_configuration.get('build_configuration', {}), identifier),
            certificate_configuration=gamelift.CfnFleet.CertificateConfigurationProperty(
                certificate_type=fleet_configuration['certificate_configuration'].get('certificate_type')
            ) if fleet_configuration.get('certificate_configuration') else None,
            description=fleet_configuration.get('description'),
            ec2_inbound_permissions=[
                gamelift.CfnFleet.IpPermissionProperty(
                    **inbound_permission
                ) for inbound_permission in fleet_configuration.get('ec2_inbound_permissions', [])
            ],
            ec2_instance_type=fleet_configuration.get('ec2_instance_type'),
            fleet_type=fleet_configuration.get('fleet_type'),
            name=f'{self._stack_name}-GameLiftFleet{identifier}',
            new_game_session_protection_policy=fleet_configuration.get('new_game_session_protection_policy'),
            resource_creation_limit_policy=gamelift.CfnFleet.ResourceCreationLimitPolicyProperty(
                **fleet_configuration['resource_creation_limit_policy']
            ) if fleet_configuration.get('resource_creation_limit_policy') else None,
            runtime_configuration=gamelift.CfnFleet.RuntimeConfigurationProperty(
                game_session_activation_timeout_seconds=fleet_configuration['runtime_configuration'].get(
                    'game_session_activation_timeout_seconds'),
                max_concurrent_game_session_activations=fleet_configuration['runtime_configuration'].get(
                    'max_concurrent_game_session_activations'),
                server_processes=[
                    gamelift.CfnFleet.ServerProcessProperty(
                        **server_process
                    ) for server_process in fleet_configuration['runtime_configuration'].get('server_processes', [])
                ]
            ) if fleet_configuration.get('runtime_configuration') else None,
        )

        return fleet

    def _get_gamelift_build_id(self, build_configuration: dict, identifier: int) -> str:
        """
        Create a GameLift build using the storage location if the build doesn't exist and return the build id.
        :param build_configuration: Configuration of the GameLift build.
        :param identifier: Unique identifier of the build which will be included in the resource id.
        :return: Build id.
        """
        if build_configuration.get('build_id'):
            # GameLift build already exists
            return build_configuration['build_id']
        elif build_configuration.get('storage_location'):
            # Create the GameLift build using the storage location information.
            build = gamelift.CfnBuild(
                self,
                id=f'{self._stack_name}-GameLiftBuild{identifier}',
                name=f'{self._stack_name}-GameLiftBuild{identifier}',
                operating_system=build_configuration.get('operating_system'),
                storage_location=gamelift.CfnBuild.S3LocationProperty(
                    **build_configuration['storage_location']
                )
            )
            return build.ref

        return ''

    def _create_alias(self, alias_configuration: dict, fleet_id: str) -> gamelift.CfnAlias:
        """
        Create an alias for an Amazon GameLift fleet destination.
        :param alias_configuration: Configuration of the alias
        :param fleet_id: Fleet id that the alias points to.
        :return: Generated GameLift fleet alias.
        """
        alias = gamelift.CfnAlias(
            self,
            id=f'{self._stack_name}-GameLiftAlias',
            name=alias_configuration.get('name'),
            routing_strategy=gamelift.CfnAlias.RoutingStrategyProperty(
                **alias_configuration.get('routing_strategy', {}),
                fleet_id=fleet_id
            )
        )

        return alias
