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
        void GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const override;
        Physics::MaterialId GetMaterialId() const override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        void SetMaterialAsset(const AZ::Data::AssetId& id) override;
        void SetMaterialId(const Physics::MaterialId& id) override;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;

    protected:
        void UpdateMeshAsset();

        Physics::ColliderConfiguration* m_colliderConfiguration = nullptr;
        Physics::PhysicsAssetShapeConfiguration* m_shapeConfiguration = nullptr;
    };
} // namespace PhysX