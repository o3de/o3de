/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <AtomBackend.h>
#include <AtomActor.h>
#include <AtomActorInstance.h>

namespace AZ
{
    namespace Render
    {
        class ActorAsset;

        AZ_CLASS_ALLOCATOR_IMPL(AtomBackend, EMotionFX::Integration::EMotionFXAllocator);

        EMotionFX::Integration::RenderActor* AtomBackend::CreateActor(EMotionFX::Integration::ActorAsset* asset)
        {
            return aznew AtomActor(asset);
        }

        EMotionFX::Integration::RenderActorInstance* AtomBackend::CreateActorInstance(AZ::EntityId entityId,
            const EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<EMotionFX::Integration::ActorAsset>& asset,
            [[maybe_unused]] EMotionFX::Integration::SkinningMethod skinningMethod,
            const AZ::Transform& worldTransform,
            bool rayTracingEnabled)
        {
            return aznew AZ::Render::AtomActorInstance(entityId, actorInstance, asset, worldTransform, skinningMethod, rayTracingEnabled);
        }
    } // namespace Render
} // namespace AZ
