#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Integration/AnimationBus.h
    Include/Integration/ActorComponentBus.h
    Include/Integration/AnimGraphComponentBus.h
    Include/Integration/AnimAudioComponentBus.h
    Include/Integration/EditorSimpleMotionComponentBus.h
    Include/Integration/SimpleMotionComponentBus.h
    Include/Integration/AnimGraphNetworkingBus.h
    Source/Integration/System/SystemCommon.h
    Source/Integration/Assets/AssetCommon.h
    Source/Integration/Assets/ActorAsset.h
    Source/Integration/Assets/ActorAsset.cpp
    Source/Integration/Assets/MotionAsset.cpp
    Source/Integration/Assets/MotionAsset.h
    Source/Integration/Assets/MotionSetAsset.cpp
    Source/Integration/Assets/MotionSetAsset.h
    Source/Integration/Assets/AnimGraphAsset.cpp
    Source/Integration/Assets/AnimGraphAsset.h
    Source/Integration/Components/ActorComponent.h
    Source/Integration/Components/ActorComponent.cpp
    Source/Integration/Components/AnimAudioComponent.h
    Source/Integration/Components/AnimAudioComponent.cpp
    Source/Integration/Components/AnimGraphComponent.h
    Source/Integration/Components/AnimGraphComponent.cpp
    Source/Integration/Components/SimpleMotionComponent.h
    Source/Integration/Components/SimpleMotionComponent.cpp
    Source/Integration/Components/SimpleLODComponent.h
    Source/Integration/Components/SimpleLODComponent.cpp
    Source/Integration/Rendering/RenderBackend.h
    Source/Integration/Rendering/RenderBackend.cpp
    Source/Integration/Rendering/RenderActor.h
    Source/Integration/Rendering/RenderActor.cpp
    Source/Integration/Rendering/RenderActorInstance.h
    Source/Integration/Rendering/RenderActorInstance.cpp
    Source/Integration/Rendering/RenderBackendManager.h
    Source/Integration/Rendering/RenderBackendManager.cpp
)
