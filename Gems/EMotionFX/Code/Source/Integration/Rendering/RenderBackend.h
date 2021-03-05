/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/Rendering/RenderActorInstance.h>

namespace EMotionFX
{
    namespace Integration
    {
        class RenderActor;

        class RenderBackend
        {
        public:
            AZ_RTTI(EMotionFX::Integration::RenderBackend, "{999AC1A7-0FBA-4F36-81B8-939FC80F1042}")
            AZ_CLASS_ALLOCATOR_DECL

            RenderBackend() = default;
            virtual ~RenderBackend() = default;

            virtual RenderActor* CreateActor(ActorAsset* asset) = 0;

            virtual RenderActorInstance* CreateActorInstance(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const ActorAsset::MaterialList& materialPerLOD,
                SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform) = 0;
        };
    } // namespace Integration
} // namespace EMotionFX
