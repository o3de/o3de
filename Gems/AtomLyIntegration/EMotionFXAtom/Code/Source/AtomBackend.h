/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_RTTI(AtomBackend, "{05961B40-B0B3-459A-8FB1-742778CC7BF7}", EMotionFX::Integration::RenderBackend);
            AZ_CLASS_ALLOCATOR_DECL;

            EMotionFX::Integration::RenderActor * CreateActor(EMotionFX::Integration::ActorAsset * asset) override;

            EMotionFX::Integration::RenderActorInstance* CreateActorInstance(AZ::EntityId entityId,
                const EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<EMotionFX::Integration::ActorAsset>& asset,
                EMotionFX::Integration::SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform,
                bool raytracingEnabled) override;
        };
    } // namespace Render
} // namespace AZ
