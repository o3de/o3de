"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import copy

from aws_cdk import core

from aws_gamelift.fleet_configurations import FLEET_CONFIGURATIONS
from aws_gamelift.gamelift_stack import GameLiftStack
from aws_gamelift.support_stack import SupportStack


class AWSGameLift(core.Construct):
    """
    Orchestrates setting up the AWS GameLift Stack(s).

    This construct uses the fleet configurations defined in
    aws_gamelift/fleet_configurations.py to set up the GameLift stacks.
    """
    def __init__(self,
                 scope: core.Construct,
                 id_: str,
                 project_name: str,
                 feature_name: str,
                 tags: dict,
                 env: core.Environment) -> None:
        super().__init__(scope, id_)

        stack_name = f'{project_name}-{feature_name}-{env.region}'

        fleet_configurations = copy.deepcopy(FLEET_CONFIGURATIONS)
        if self.node.try_get_context('upload-with-support-stack') == 'true':
            # Create an optional support stack for generating GameLift builds with local build files
            self._support_stack = SupportStack(
                scope,
                f'{stack_name}-Support',
                stack_name=stack_name,
                fleet_configurations=fleet_configurations,
                description='Contains resources for creating GameLift builds with local files',
                tags=tags,
                env=env
            )

        # Create the GameLift Stack
        self._feature_stack = GameLiftStack(
            scope,
            f'{stack_name}',
            stack_name=stack_name,
            fleet_configurations=fleet_configurations,
            create_game_session_queue=self.node.try_get_context('create_game_session_queue') == 'true',
            flex_match=self.node.try_get_context('flex_match') == 'true',
            description=f'Contains resources for the AWS GameLift Gem stack as part of the {project_name} project',
            tags=tags,
            env=env
        )
