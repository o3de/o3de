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

#include <Integration/Rendering/RenderBackend.h>

namespace EMotionFX
{
    namespace Integration
    {
        class CryRenderBackend
            : public RenderBackend
        {
        public:
            AZ_RTTI(EMotionFX::Integration::CryRenderBackend, "{CC4AF6B1-D5D2-4EAA-8198-DED4F875D1F4}", EMotionFX::Integration::RenderBackend);
            AZ_CLASS_ALLOCATOR_DECL;

            RenderActor * CreateActor(ActorAsset * asset) override;

            RenderActorInstance* CreateActorInstance(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const ActorAsset::MaterialList& materialPerLOD,
                SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform) override;
        };
    } // namespace Integration
} // namespace EMotionFX
