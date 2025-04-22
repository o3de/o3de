/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <Integration/Rendering/RenderActorInstance.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(RenderActorInstance, EMotionFXAllocator);

        RenderActorInstance::RenderActorInstance(const AZ::Data::Asset<ActorAsset>& actorAsset,
            ActorInstance* actorInstance, AZ::EntityId entityId)
            : m_actorAsset(actorAsset)
            , m_actorInstance(actorInstance)
            , m_entityId(entityId)
        {
        }

        SkinningMethod RenderActorInstance::GetSkinningMethod() const
        {
            return m_skinningMethod;
        }

        void RenderActorInstance::SetSkinningMethod(SkinningMethod skinningMethod)
        {
            m_skinningMethod = skinningMethod;
        }

        const AZ::Aabb& RenderActorInstance::GetWorldAABB() const
        {
            return m_worldAABB;
        }

        const AZ::Aabb& RenderActorInstance::GetLocalAABB() const
        {
            return m_localAABB;
        }

        bool RenderActorInstance::IsVisible() const
        {
            return m_isVisible;
        }

        void RenderActorInstance::SetIsVisible(bool isVisible)
        {
            m_isVisible = isVisible;
        }

        bool RenderActorInstance::IsInCameraFrustum() const
        {
            return true;
        }

        Actor* RenderActorInstance::GetActor() const
        {
            ActorAsset* actorAsset = m_actorAsset.Get();
            if (actorAsset)
            {
                return actorAsset->GetActor();
            }

            return nullptr;
        }
    } // namespace Integration
} // namespace EMotionFX
