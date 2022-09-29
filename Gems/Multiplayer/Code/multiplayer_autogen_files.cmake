#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ${LY_ROOT_FOLDER}/Code/Framework/AzNetworking/AzNetworking/AutoGen/AutoPackets_Header.jinja
    ${LY_ROOT_FOLDER}/Code/Framework/AzNetworking/AzNetworking/AutoGen/AutoPackets_Inline.jinja
    ${LY_ROOT_FOLDER}/Code/Framework/AzNetworking/AzNetworking/AutoGen/AutoPackets_Source.jinja
    ${LY_ROOT_FOLDER}/Code/Framework/AzNetworking/AzNetworking/AutoGen/AutoPacketDispatcher_Header.jinja
    ${LY_ROOT_FOLDER}/Code/Framework/AzNetworking/AzNetworking/AutoGen/AutoPacketDispatcher_Inline.jinja
    Include/Multiplayer/AutoGen/AutoComponentTypes_Header.jinja
    Include/Multiplayer/AutoGen/AutoComponentTypes_Source.jinja
    Include/Multiplayer/AutoGen/AutoComponent_Common.jinja
    Include/Multiplayer/AutoGen/AutoComponent_Header.jinja
    Include/Multiplayer/AutoGen/AutoComponent_Source.jinja
    Source/AutoGen/LocalPredictionPlayerInputComponent.AutoComponent.xml
    Source/AutoGen/Multiplayer.AutoPackets.xml
    Source/AutoGen/MultiplayerEditor.AutoPackets.xml
    Source/AutoGen/NetworkCharacterComponent.AutoComponent.xml
    Source/AutoGen/NetworkHitVolumesComponent.AutoComponent.xml
    Source/AutoGen/NetworkRigidBodyComponent.AutoComponent.xml
    Source/AutoGen/NetworkTransformComponent.AutoComponent.xml
    Source/AutoGen/NetworkHierarchyChildComponent.AutoComponent.xml
    Source/AutoGen/NetworkHierarchyRootComponent.AutoComponent.xml
)
