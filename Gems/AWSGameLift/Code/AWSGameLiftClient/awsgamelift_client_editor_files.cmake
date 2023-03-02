#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ../AWSGameLiftCommon/Include/AWSGameLiftPlayer.h
    ../AWSGameLiftCommon/Source/AWSGameLiftPlayer.cpp
    ../AWSGameLiftCommon/Source/AWSGameLiftSessionConstants.h
    Source/Activity/AWSGameLiftActivityUtils.cpp
    Source/Activity/AWSGameLiftActivityUtils.h
    Source/Activity/AWSGameLiftAcceptMatchActivity.cpp
    Source/Activity/AWSGameLiftAcceptMatchActivity.h
    Source/Activity/AWSGameLiftCreateSessionActivity.cpp
    Source/Activity/AWSGameLiftCreateSessionActivity.h
    Source/Activity/AWSGameLiftCreateSessionOnQueueActivity.cpp
    Source/Activity/AWSGameLiftCreateSessionOnQueueActivity.h
    Source/Activity/AWSGameLiftJoinSessionActivity.cpp
    Source/Activity/AWSGameLiftJoinSessionActivity.h
    Source/Activity/AWSGameLiftLeaveSessionActivity.cpp
    Source/Activity/AWSGameLiftLeaveSessionActivity.h
    Source/Activity/AWSGameLiftSearchSessionsActivity.cpp
    Source/Activity/AWSGameLiftSearchSessionsActivity.h
    Source/Activity/AWSGameLiftStartMatchmakingActivity.cpp
    Source/Activity/AWSGameLiftStartMatchmakingActivity.h
    Source/Activity/AWSGameLiftStopMatchmakingActivity.cpp
    Source/Activity/AWSGameLiftStopMatchmakingActivity.h
    Source/AWSGameLiftClientLocalTicketTracker.cpp
    Source/AWSGameLiftClientLocalTicketTracker.h
    Source/AWSGameLiftClientManager.cpp
    Source/AWSGameLiftClientManager.h
    Source/AWSGameLiftClientEditorSystemComponent.cpp
    Source/AWSGameLiftClientEditorSystemComponent.h
    Source/AWSGameLiftClientSystemComponent.cpp
    Source/AWSGameLiftClientSystemComponent.h
    Source/Request/AWSGameLiftAcceptMatchRequest.cpp
    Source/Request/AWSGameLiftCreateSessionOnQueueRequest.cpp
    Source/Request/AWSGameLiftCreateSessionRequest.cpp
    Source/Request/AWSGameLiftJoinSessionRequest.cpp
    Source/Request/AWSGameLiftSearchSessionsRequest.cpp
    Source/Request/AWSGameLiftStartMatchmakingRequest.cpp
    Source/Request/AWSGameLiftStopMatchmakingRequest.cpp
    Source/Request/IAWSGameLiftInternalRequests.h
)
