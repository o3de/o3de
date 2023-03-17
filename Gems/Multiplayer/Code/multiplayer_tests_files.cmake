#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Multiplayer/AutoGen/AutoComponentTypes_Header.jinja
    Include/Multiplayer/AutoGen/AutoComponentTypes_Source.jinja
    Include/Multiplayer/AutoGen/AutoComponent_Common.jinja
    Include/Multiplayer/AutoGen/AutoComponent_Header.jinja
    Include/Multiplayer/AutoGen/AutoComponent_Source.jinja
    Tests/AutoGen/TestMultiplayerComponent.AutoComponent.xml
    Tests/ClientHierarchyTests.cpp
    Tests/ServerHierarchyBenchmarks.cpp
    Tests/CommonHierarchySetup.h
    Tests/CommonNetworkEntitySetup.h
    Tests/CommonBenchmarkSetup.h
    Tests/IMultiplayerConnectionMock.h
    Tests/IMultiplayerSpawnerMock.h
    Tests/Main.cpp
    Tests/MockInterfaces.h
    Tests/LocalPredictionPlayerInputTests.cpp
    Tests/MultiplayerComponentTests.cpp
    Tests/MultiplayerSystemTests.cpp
    Tests/NetworkCharacterTests.cpp
    Tests/NetworkEntityTests.cpp
    Tests/NetworkInputTests.cpp
    Tests/NetworkRigidBodyTests.cpp
    Tests/NetworkTransformTests.cpp
    Tests/RewindableContainerTests.cpp
    Tests/RewindableObjectTests.cpp
    Tests/ServerHierarchyTests.cpp
    Tests/SimplePlayerSpawnerTests.cpp
    Tests/TestMultiplayerComponent.h
    Tests/TestMultiplayerComponent.cpp

    Tests/LocalRpcTests.cpp
    Tests/AutoGen/RpcUnitTesterComponent.AutoComponent.xml
    Tests/RpcUnitTesterComponent.h
    Tests/RpcUnitTesterComponent.cpp
)
