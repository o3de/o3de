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

from . import flexmatch_configurations

FLEX_MATCH_MODE = 'WITH_QUEUE'


class MatchmakingResoures:
    """
    Create a matchmaking rule set and matchmaking configuration for Gamelift FlexMatch.
    For more information about Gamelift FlexMatch, please check
    https://docs.aws.amazon.com/gamelift/latest/flexmatchguide/match-intro.html
    """
    def __init__(self, stack: core.Stack, game_session_queue_arns: typing.List[str]):
        rule_set = gamelift.CfnMatchmakingRuleSet(
            scope=stack,
            id='MatchmakingRuleSet',
            name=f'{stack.stack_name}-MatchmakingRuleSet',
            rule_set_body=flexmatch_configurations.RULE_SET_BODY
        )

        matchmaking_configuration = gamelift.CfnMatchmakingConfiguration(
            scope=stack,
            id='MatchmakingConfiguration',
            acceptance_required=flexmatch_configurations.ACCEPTANCE_REQUIRED,
            name=f'{stack.stack_name}-MatchmakingConfiguration',
            request_timeout_seconds=flexmatch_configurations.REQUEST_TIMEOUT_SECONDS,
            rule_set_name=rule_set.name,
            additional_player_count=flexmatch_configurations.ADDITIONAL_PLAYER_COUNT,
            backfill_mode=flexmatch_configurations.BACKFILL_MODE,
            flex_match_mode=FLEX_MATCH_MODE,
            game_session_queue_arns=game_session_queue_arns if len(game_session_queue_arns) else None
        )
        matchmaking_configuration.node.add_dependency(rule_set)

        # Export the matchmaking rule set and configuration names as stack outputs
        core.CfnOutput(
            stack,
            id='MatchmakingRuleSetName',
            description='Name of the matchmaking rule set',
            export_name=f'{stack.stack_name}:MatchmakingRuleSet',
            value=rule_set.name)
        core.CfnOutput(
            stack,
            id='MatchmakingConfigurationName',
            description='Name of the matchmaking configuration',
            export_name=f'{stack.stack_name}:MatchmakingConfiguration',
            value=matchmaking_configuration.name)
