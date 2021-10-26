/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <EMotionFX/Source/ActorInstance.h>

#include <Integration/ActorComponentBus.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Rendering/RenderFlag.h>

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
            virtual void DebugDraw(const EMotionFX::ActorRenderFlagBitset& renderFlags) = 0;

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
