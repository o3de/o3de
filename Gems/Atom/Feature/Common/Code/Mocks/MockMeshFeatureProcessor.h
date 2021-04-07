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

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockMeshFeatureProcessor : public AZ::Render::MeshFeatureProcessorInterface
    {
    public:
        MOCK_METHOD3(
            AcquireMesh,
            MeshHandle(const AZ::Data::Asset<AZ::RPI::ModelAsset>&, const AZ::Render::MaterialAssignmentMap&, bool));
        MOCK_METHOD3(
            AcquireMesh,
            MeshHandle(
                const AZ::Data::Asset<AZ::RPI::ModelAsset>&, const AZStd::intrusive_ptr<AZ ::RPI::Material>&, bool));
        MOCK_METHOD1(ReleaseMesh, bool(MeshHandle&));
        MOCK_METHOD1(CloneMesh, MeshHandle(const MeshHandle&));
        MOCK_CONST_METHOD1(GetModel, AZStd::intrusive_ptr<AZ::RPI::Model>(const MeshHandle&));
        MOCK_CONST_METHOD1(GetMaterialAssignmentMap, const AZ::Render::MaterialAssignmentMap&(const MeshHandle&));
        MOCK_METHOD2(ConnectModelChangeEventHandler, void(const MeshHandle&, ModelChangedEvent::Handler&));
        MOCK_METHOD2(SetTransform, void(const MeshHandle&, const AZ::Transform&));
        MOCK_METHOD2(SetExcludeFromReflectionCubeMaps, void(const MeshHandle&, bool));
        MOCK_METHOD2(SetMaterialAssignmentMap, void(const MeshHandle&, const AZ::Data::Instance<AZ::RPI::Material>&));
        MOCK_METHOD2(SetMaterialAssignmentMap, void(const MeshHandle&, const AZ::Render::MaterialAssignmentMap&));
        MOCK_METHOD1(GetTransform, AZ::Transform (const MeshHandle&));
        MOCK_METHOD2(SetSortKey, void (const MeshHandle&, AZ::RHI::DrawItemSortKey));
        MOCK_METHOD1(GetSortKey, AZ::RHI::DrawItemSortKey(const MeshHandle&));
        MOCK_METHOD2(SetLodOverride, void(const MeshHandle&, AZ::RPI::Cullable::LodOverride));
        MOCK_METHOD1(GetLodOverride, AZ::RPI::Cullable::LodOverride(const MeshHandle&));
        MOCK_METHOD4(AcquireMesh, MeshHandle (const AZ::Data::Asset<AZ::RPI::ModelAsset>&, const AZ::Render::MaterialAssignmentMap&, bool, bool));
        MOCK_METHOD4(AcquireMesh, MeshHandle (const AZ::Data::Asset<AZ::RPI::ModelAsset>&, const AZ::Data::Instance<AZ::RPI::Material>&, bool, bool));
        MOCK_METHOD2(SetRayTracingEnabled, void (const MeshHandle&, bool));
        MOCK_METHOD2(SetVisible, void (const MeshHandle&, bool));
        MOCK_METHOD2(SetUseForwardPassIblSpecular, void (const MeshHandle&, bool));
    };
} // namespace UnitTest
