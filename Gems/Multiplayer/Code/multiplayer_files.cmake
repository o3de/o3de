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
    Source/Multiplayer_precompiled.cpp
    Source/Multiplayer_precompiled.h
    Include/Multiplayer/IMultiplayerGem.h
    Include/Multiplayer/MultiplayerLobbyComponent.h
    Include/Multiplayer/MultiplayerEventsComponent.h
    Include/Multiplayer/MultiplayerUtils.h
    Include/Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyLANServiceWrapper.h
    Include/Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyServiceWrapper.h
    Include/Multiplayer/GridMateServiceWrapper/GridMateLANServiceWrapper.h
    Include/Multiplayer/GridMateServiceWrapper/GridMateServiceWrapper.h
    Include/Multiplayer/BehaviorContext/GridSearchContext.h
    Include/Multiplayer/BehaviorContext/GridSystemContext.h
    Source/MultiplayerLobbyComponent.cpp
    Source/MultiplayerCVars.h
    Source/MultiplayerCVars.cpp
    Source/MultiplayerUtils.cpp
    Source/GameLiftListener.h
    Source/GameLiftListener.cpp
    Source/MultiplayerEventsComponent.cpp
    Source/MultiplayerGameLiftClient.h
    Source/MultiplayerGameLiftClient.cpp
    Source/MultiplayerLobbyServiceWrapper/MultiplayerLobbyLANServiceWrapper.cpp
    Source/MultiplayerLobbyServiceWrapper/MultiplayerLobbyServiceWrapper.cpp
    Source/GridMateServiceWrapper/GridMateLANServiceWrapper.cpp
    Source/GridMateServiceWrapper/GridMateServiceWrapper.cpp
    Source/BehaviorContext/GridSearchContext.cpp
    Source/BehaviorContext/GridSystemContext.cpp
    Source/Canvas/MultiplayerCanvasHelper.h
    Source/Canvas/MultiplayerCanvasHelper.cpp
    Source/Canvas/MultiplayerCreateServerView.h
    Source/Canvas/MultiplayerCreateServerView.cpp
    Source/Canvas/MultiplayerGameLiftFlextMatchView.h
    Source/Canvas/MultiplayerGameLiftFlextMatchView.cpp
    Source/Canvas/MultiplayerJoinServerView.h
    Source/Canvas/MultiplayerJoinServerView.cpp
    Source/Canvas/MultiplayerGameLiftLobbyCanvas.h
    Source/Canvas/MultiplayerGameLiftLobbyCanvas.cpp
    Source/Canvas/MultiplayerLANGameLobbyCanvas.h
    Source/Canvas/MultiplayerLANGameLobbyCanvas.cpp
    Source/Canvas/MultiplayerBusyAndErrorCanvas.h
    Source/Canvas/MultiplayerBusyAndErrorCanvas.cpp
    Source/Canvas/MultiplayerDedicatedHostTypeSelectionCanvas.h
    Source/Canvas/MultiplayerDedicatedHostTypeSelectionCanvas.cpp
    Source/GameLift/GameLiftMatchmakingComponent.h
    Source/GameLift/GameLiftMatchmakingComponent.cpp
)
