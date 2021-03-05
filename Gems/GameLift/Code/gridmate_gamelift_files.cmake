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
    Include/GameLift/GameLiftBus.h
    Include/GameLift/Session/GameLiftClientService.h
    Include/GameLift/Session/GameLiftClientServiceBus.h
    Include/GameLift/Session/GameLiftClientServiceEventsBus.h
    Include/GameLift/Session/GameLiftClientSession.h
    Include/GameLift/Session/GameLiftSearch.h
    Include/GameLift/Session/GameLiftServerService.h
    Include/GameLift/Session/GameLiftServerServiceBus.h
    Include/GameLift/Session/GameLiftServerServiceEventsBus.h
    Include/GameLift/Session/GameLiftServerSession.h
    Include/GameLift/Session/GameLiftSessionDefs.h
    Include/GameLift/Session/GameLiftSessionRequest.h
    Include/GameLift/Session/GameLiftGameSessionPlacementRequest.h
    Include/GameLift/Session/GameLiftRequestInterface.h
    Include/GameLift/Session/GameLiftMatchmaking.h
    Include/GameLift/Session/GameLiftServerSDKWrapper.h
    Source/Session/GameLiftClientService.cpp
    Source/Session/GameLiftClientSession.cpp
    Source/Session/GameLiftSearch.cpp
    Source/Session/GameLiftServerService.cpp
    Source/Session/GameLiftServerSession.cpp
    Source/Session/GameLiftSessionRequest.cpp
    Source/Session/GameLiftGameSessionPlacementRequest.cpp
    Source/Session/GameLiftMatchmaking.cpp
    Source/Session/GameLiftServerSDKWrapper.cpp
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    Source/Session/GameLiftClientSession.cpp
)
