"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import typing

from aws_cdk import (
    core,
    aws_gamelift as gamelift
)


class GameSessionQueueResources:
    """
    Create a game session queue which fulfills game session placement requests using the fleets.
    For more information about Gamelift game session queues, please check
    https://docs.aws.amazon.com/gamelift/latest/developerguide/queues-intro.html
    """
    def __init__(self, stack: core.Stack, destinations: typing.List):
        self._game_session_queue = gamelift.CfnGameSessionQueue(
            scope=stack,
            id=f'{stack.stack_name}-GameLiftQueue',
            name=f'{stack.stack_name}-GameLiftQueue',
            destinations=[
                gamelift.CfnGameSessionQueue.DestinationProperty(
                    destination_arn=resource_arn
                ) for resource_arn in destinations
            ]
        )

        # Export the game session queue name as a stack output
        core.CfnOutput(
            scope=stack,
            id='GameSessionQueue',
            description='Name of the game session queue',
            export_name=f'{stack.stack_name}:GameSessionQueue',
            value=self._game_session_queue.name)

    @property
    def game_session_queue_arn(self) -> str:
        return self._game_session_queue.attr_arn
