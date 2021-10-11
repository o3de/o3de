"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Matchmaking rule formatted as a JSON string.
# Comments are not allowed in JSON, but most elements support a description field.
# For instructions on designing Matchmaking rule sets, please check:
# https://docs.aws.amazon.com/gamelift/latest/flexmatchguide/match-design-ruleset.html
RULE_SET_BODY = '{"ruleLanguageVersion":"1.0","teams":[{"name":"Players","maxPlayers":4,"minPlayers":2}]}'

# A flag that determines whether a match that was created with this configuration
# must be accepted by the matched players.
ACCEPTANCE_REQUIRED = False

# The maximum duration, in seconds, that a matchmaking ticket can remain in process before timing out.
# Requests that fail due to timing out can be resubmitted as needed.
REQUEST_TIMEOUT_SECONDS = 300

# The number of player slots in a match to keep open for future players.
# This parameter is not used if FlexMatchMode is set to STANDALONE.
ADDITIONAL_PLAYER_COUNT = 2

# The method used to backfill game sessions that are created with this matchmaking configuration.
# Specify MANUAL when your game manages backfill requests manually or does not use the match backfill feature.
# Specify AUTOMATIC to have GameLift create a StartMatchBackfill request whenever a game session has one or more
# open slots.
BACKFILL_MODE = 'AUTOMATIC'
