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

#include <AzCore/Math/Aabb.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <EMotionFX/Source/ActorInstance.h>

#include <Integration/ActorComponentBus.h>
#include <Integration/Assets/ActorAsset.h>

namespace EMotionFX
{
    namespace Integration
    {
        class RenderActorInstance
        {
        public:
            AZ_RTTI(EMotionFX::Integration::RenderActorInstance, "{7F5FA3A7-BE62-4384-9C99-72305404C0BF}")
            AZ_CLASS_ALLOCATOR_DECL

            RenderActorInstance(const AZ::Data::Asset<ActorAsset>& actorAsset,
                ActorInstance* actorInstance,
                AZ::EntityId entityId);
            virtual ~RenderActorInstance() = default;

            virtual void OnTick(float timeDelta) = 0;

            struct DebugOptions
            {
                bool m_drawAABB = false;
                bool m_drawSkeleton = false;
                bool m_drawRootTransform = false;
                AZ::Transform m_rootWorldTransform = AZ::Transform::CreateIdentity();
                bool m_emfxDebugDraw = false;
            };
            virtual void DebugDraw(const DebugOptions& debugOptions) = 0;

            SkinningMethod GetSkinningMethod() const;
            virtual void SetSkinningMethod(SkinningMethod skinningMethod);

            virtual void UpdateBounds() = 0;
            const AZ::Aabb& GetWorldAABB() const;
            const AZ::Aabb& GetLocalAABB() const;

            bool IsVisible() const;
            virtual void SetIsVisible(bool isVisible);
            virtual bool IsInCameraFrustum() const;

            virtual void SetMaterials(const ActorAsset::MaterialList& materialsPerLOD) = 0;
            typedef AZStd::function<void(const AZStd::string& materialName)> MaterialChangedFunction;
            void SetOnMaterialChangedCallback(MaterialChangedFunction callback);

            Actor* GetActor() const;

        protected:
            AZ::Data::Asset<ActorAsset> m_actorAsset;
            ActorInstance* m_actorInstance = nullptr;
            const AZ::EntityId m_entityId;

            AZ::Aabb m_localAABB = AZ::Aabb::CreateNull();
            AZ::Aabb m_worldAABB = AZ::Aabb::CreateNull();

            bool m_isVisible = true;
            SkinningMethod m_skinningMethod = SkinningMethod::DualQuat;
            MaterialChangedFunction m_onMaterialChangedCallback;
        };
    } // namespace Integration
} // namespace EMotionFX
