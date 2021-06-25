#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/Request/AWSGameLiftCreateSessionOnQueueRequest.h
    Include/Request/AWSGameLiftCreateSessionRequest.h
    Include/Request/AWSGameLiftJoinSessionRequest.h
    Include/Request/AWSGameLiftSearchSessionsRequest.h
    Include/Request/IAWSGameLiftRequests.h
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
    Source/AWSGameLiftClientManager.cpp
    Source/AWSGameLiftClientManager.h
    Source/AWSGameLiftClientSystemComponent.cpp
    Source/AWSGameLiftClientSystemComponent.h
    Source/Request/AWSGameLiftCreateSessionOnQueueRequest.cpp
    Source/Request/AWSGameLiftCreateSessionRequest.cpp
    Source/Request/AWSGameLiftJoinSessionRequest.cpp
    Source/Request/AWSGameLiftSearchSessionsRequest.cpp
)
