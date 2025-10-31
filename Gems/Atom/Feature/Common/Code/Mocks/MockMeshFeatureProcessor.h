/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockMeshFeatureProcessor : public AZ::Render::MeshFeatureProcessorInterface
    {
    public:
        MOCK_CONST_METHOD1(GetObjectId, AZ::Render::TransformServiceFeatureProcessorInterface::ObjectId(const MeshHandle&));
        MOCK_METHOD1(ReleaseMesh, bool(MeshHandle&));
        MOCK_METHOD1(CloneMesh, MeshHandle(const MeshHandle&));
        MOCK_CONST_METHOD1(GetModel, AZStd::intrusive_ptr<AZ::RPI::Model>(const MeshHandle&));
        MOCK_CONST_METHOD1(GetModelAsset, AZ::Data::Asset<AZ::RPI::ModelAsset>(const MeshHandle&));
        MOCK_CONST_METHOD1(GetDrawPackets, const AZ::RPI::MeshDrawPacketLods&(const MeshHandle&));
        MOCK_CONST_METHOD1(GetObjectSrgs, const AZStd::vector<AZStd::intrusive_ptr<AZ::RPI::ShaderResourceGroup>>&(const MeshHandle&));
        MOCK_CONST_METHOD1(QueueObjectSrgForCompile, void(const MeshHandle&));
        MOCK_CONST_METHOD1(GetCustomMaterials, const AZ::Render::CustomMaterialMap&(const MeshHandle&));
        MOCK_METHOD2(ConnectModelChangeEventHandler, void(const MeshHandle&, ModelChangedEvent::Handler&));
        MOCK_METHOD3(SetTransform, void(const MeshHandle&, const AZ::Transform&, const AZ::Vector3&));
        MOCK_METHOD2(SetIsAlwaysDynamic, void(const MeshHandle&, bool));
        MOCK_CONST_METHOD1(GetIsAlwaysDynamic, bool(const MeshHandle&));
        MOCK_METHOD2(SetCustomMaterials, void(const MeshHandle&, const AZ::Data::Instance<AZ::RPI::Material>&));
        MOCK_METHOD2(SetCustomMaterials, void(const MeshHandle&, const AZ::Render::CustomMaterialMap&));
        MOCK_METHOD1(GetTransform, AZ::Transform(const MeshHandle&));
        MOCK_METHOD1(GetNonUniformScale, AZ::Vector3(const MeshHandle&));
        MOCK_METHOD2(SetLocalAabb, void(const MeshHandle&, const AZ::Aabb&));
        MOCK_CONST_METHOD1(GetLocalAabb, AZ::Aabb(const MeshHandle&));
        MOCK_METHOD2(SetSortKey, void (const MeshHandle&, AZ::RHI::DrawItemSortKey));
        MOCK_CONST_METHOD1(GetSortKey, AZ::RHI::DrawItemSortKey(const MeshHandle&));
        MOCK_METHOD2(SetMeshLodConfiguration, void(const MeshHandle&, const AZ::RPI::Cullable::LodConfiguration&));
        MOCK_CONST_METHOD1(GetMeshLodConfiguration, AZ::RPI::Cullable::LodConfiguration(const MeshHandle&));
        MOCK_METHOD1(AcquireMesh, MeshHandle (const AZ::Render::MeshHandleDescriptor&));
        MOCK_METHOD2(SetRayTracingEnabled, void (const MeshHandle&, bool));
        MOCK_CONST_METHOD1(GetRayTracingEnabled, bool(const MeshHandle&));
        MOCK_METHOD2(SetExcludeFromReflectionCubeMaps, void(const MeshHandle&, bool));
        MOCK_CONST_METHOD1(GetExcludeFromReflectionCubeMaps, bool(const MeshHandle&));
        MOCK_METHOD2(SetVisible, void (const MeshHandle&, bool));
        MOCK_CONST_METHOD1(GetVisible, bool(const MeshHandle&));
        MOCK_METHOD2(SetUseForwardPassIblSpecular, void(const MeshHandle&, bool));
        MOCK_METHOD1(SetRayTracingDirty, void(const MeshHandle&));
    };
} // namespace UnitTest
