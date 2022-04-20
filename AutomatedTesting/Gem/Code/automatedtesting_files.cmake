#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/AutomatedTesting/AutomatedTestingBus.h
    Source/AutoGen/NetworkTestPlayerComponent.AutoComponent.xml
    Source/AutoGen/NetworkTestLevelEntityComponent.AutoComponent.xml
    Source/AutoGen/SimpleScriptPlayerComponent.AutoComponent.xml
    Source/AutoGen/NetworkPlayerSpawnerComponent.AutoComponent.xml
    Source/Components/NetworkPlayerSpawnerComponent.cpp
    Source/Components/NetworkPlayerSpawnerComponent.h
    Source/Spawners/IPlayerSpawner.h
    Source/Spawners/RoundRobinSpawner.h
    Source/Spawners/RoundRobinSpawner.cpp
    Source/AutomatedTestingModule.cpp
    Source/AutomatedTestingSystemComponent.cpp
    Source/AutomatedTestingSystemComponent.h
)
