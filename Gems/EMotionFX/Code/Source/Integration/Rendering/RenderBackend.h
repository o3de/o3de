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
                SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform,
                bool rayTracingEnabled) = 0;
        };
    } // namespace Integration
} // namespace EMotionFX
