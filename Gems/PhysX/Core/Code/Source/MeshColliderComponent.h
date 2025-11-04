/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PhysX/MeshColliderComponentBus.h>
#include <Source/BaseColliderComponent.h>

namespace PhysX
{
    class MeshColliderComponent
        : public BaseColliderComponent
        , public MeshColliderComponentRequestsBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_COMPONENT(MeshColliderComponent, "{F3C7996A-F9B8-4AFD-B2A1-6DE971EFDA11}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        MeshColliderComponent() = default;
        ~MeshColliderComponent() override = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // MeshColliderComponentRequestsBus
        AZ::Data::Asset<Pipeline::MeshAsset> GetMeshAsset() const override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;

    protected:
        void UpdateMeshAsset();

        Physics::ColliderConfiguration* m_colliderConfiguration = nullptr;
        Physics::PhysicsAssetShapeConfiguration* m_shapeConfiguration = nullptr;
    };
} // namespace PhysX
