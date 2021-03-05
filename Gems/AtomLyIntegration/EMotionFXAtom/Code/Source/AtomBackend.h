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

namespace AZ
{
    namespace Render
    {
        class AtomBackend
            : public EMotionFX::Integration::RenderBackend
        {
        public:
            AZ_RTTI(EMotionFX::Integration::AtomBackend, "{05961B40-B0B3-459A-8FB1-742778CC7BF7}", EMotionFX::Integration::RenderBackend);
            AZ_CLASS_ALLOCATOR_DECL;

            EMotionFX::Integration::RenderActor * CreateActor(EMotionFX::Integration::ActorAsset * asset) override;

            EMotionFX::Integration::RenderActorInstance* CreateActorInstance(AZ::EntityId entityId,
                const EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<EMotionFX::Integration::ActorAsset>& asset,
                const EMotionFX::Integration::ActorAsset::MaterialList& materialPerLOD,
                EMotionFX::Integration::SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform) override;
        };
    } // namespace Render
} // namespace AZ
