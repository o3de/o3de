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
#include <Integration/Rendering/RenderActorInstance.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(RenderActorInstance, EMotionFXAllocator, 0);

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

        void RenderActorInstance::SetOnMaterialChangedCallback(MaterialChangedFunction callback)
        {
            m_onMaterialChangedCallback = callback;
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
