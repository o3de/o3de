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

#include <EMotionFX/Source/Actor.h>
#include <AtomBackend.h>
#include <AtomActor.h>
#include <AtomActorInstance.h>

namespace AZ
{
    namespace Render
    {
        class ActorAsset;

        AZ_CLASS_ALLOCATOR_IMPL(AtomBackend, EMotionFX::Integration::EMotionFXAllocator, 0);

        EMotionFX::Integration::RenderActor* AtomBackend::CreateActor(EMotionFX::Integration::ActorAsset* asset)
        {
            return aznew AtomActor(asset);
        }

        EMotionFX::Integration::RenderActorInstance* AtomBackend::CreateActorInstance(AZ::EntityId entityId,
            const EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<EMotionFX::Integration::ActorAsset>& asset,
            [[maybe_unused]] const EMotionFX::Integration::ActorAsset::MaterialList& materialPerLOD,
            [[maybe_unused]] EMotionFX::Integration::SkinningMethod skinningMethod,
            const AZ::Transform& worldTransform)
        {
            return aznew AZ::Render::AtomActorInstance(entityId, actorInstance, asset, worldTransform, skinningMethod);
        }
    } // namespace Render
} // namespace AZ
