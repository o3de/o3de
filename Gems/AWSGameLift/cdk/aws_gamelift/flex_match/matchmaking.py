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

from . import flex_match_configurations

BACKFILL_MODE_MANUAL = 'MANUAL'
FLEX_MATCH_MODE_STANDALONE = 'STANDALONE'

class Matchmaking:
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
            rule_set_body=flex_match_configurations.RULE_SET_BODY
        )

        # AUTOMATIC FlextMatch mode is not available when there's no game session queue.
        flex_match_mode = FLEX_MATCH_MODE_STANDALONE if len(game_session_queue_arns) == 0 \
            else flex_match_configurations.FLEX_MATCH_MODE
        # Automatic backfill is not available when FlexMatchMode is set to STANDALONE.
        backfill_mode = BACKFILL_MODE_MANUAL if flex_match_mode == FLEX_MATCH_MODE_STANDALONE \
            else flex_match_configurations.BACKFILL_MODE

        matchmaking_configuration = gamelift.CfnMatchmakingConfiguration(
            scope=stack,
            id='MatchmakingConfiguration',
            acceptance_required=flex_match_configurations.ACCEPTANCE_REQUIRED,
            name=f'{stack.stack_name}-MatchmakingConfiguration',
            request_timeout_seconds=flex_match_configurations.REQUEST_TIMEOUT_SECONDS,
            rule_set_name=rule_set.name,
            additional_player_count=flex_match_configurations.ADDITIONAL_PLAYER_COUNT,
            backfill_mode=backfill_mode,
            flex_match_mode=flex_match_mode,
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
