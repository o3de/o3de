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
