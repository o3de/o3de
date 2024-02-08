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
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <Source/Pipeline/PrimitiveShapeFitter/PrimitiveShapeFitter.h>

namespace AZ
{
    class ReflectContext;
    class SerializeContext;

    namespace SceneAPI::Containers
    {
        class SceneGraph;
    }
}

namespace PhysX
{
    namespace Pipeline
    {
        class MeshGroup;

        enum class MeshExportMethod
            : AZ::u8
        {
            TriMesh,
            Convex,
            Primitive,
        };

        class TriangleMeshAssetParams
        {
        friend MeshGroup;

        public:
            AZ_TYPE_INFO(TriangleMeshAssetParams, "{4E13C91E-F700-42DC-8669-895359D903E6}");

            TriangleMeshAssetParams();

            static void Reflect(AZ::ReflectContext* context);

            bool GetMergeMeshes() const;
            void SetMergeMeshes(bool mergeMeshes);
            bool GetWeldVertices() const;
            void SetWeldVertices(bool weldVertices);
            bool GetDisableCleanMesh() const;
            bool GetForce32BitIndices() const;
            bool GetSuppressTriangleMeshRemapTable() const;
            bool GetBuildTriangleAdjacencies() const;
            float GetMeshWeldTolerance() const;
            void SetMeshWeldTolerance(float weldTolerance);
            AZ::u32 GetNumTrisPerLeaf() const;

        private:
            bool m_mergeMeshes = false;
            bool m_weldVertices = false;
            bool m_disableCleanMesh = false;
            bool m_force32BitIndices = false;
            bool m_suppressTriangleMeshRemapTable = false;
            bool m_buildTriangleAdjacencies = false;
            float m_meshWeldTolerance = 0.0f;
            AZ::u32 m_numTrisPerLeaf = 0;
        };

        class ConvexAssetParams
        {
        friend MeshGroup;

        public:
            AZ_TYPE_INFO(ConvexAssetParams, "{C14B4312-0643-4CFD-BD1B-0B1F6C0CE8F4}");

            ConvexAssetParams();

            static void Reflect(AZ::ReflectContext* context);

            float GetAreaTestEpsilon() const;
            float GetPlaneTolerance() const;
            bool GetUse16bitIndices() const;
            bool GetCheckZeroAreaTriangles() const;
            bool GetQuantizeInput() const;
            bool GetUsePlaneShifting() const;
            bool GetShiftVertices() const;
            bool GetBuildGpuData() const;
            AZ::u32 GetGaussMapLimit() const;

        private:
            float m_areaTestEpsilon = 0.0f;
            float m_planeTolerance = 0.0f;
            bool m_use16bitIndices = false;
            bool m_checkZeroAreaTriangles = false;
            bool m_quantizeInput = false;
            bool m_usePlaneShifting = false;
            bool m_shiftVertices = false;
            bool m_buildGpuData = false;
            AZ::u32 m_gaussMapLimit = 0;
        };

        class PrimitiveAssetParams
        {
        friend MeshGroup;

        public:
            AZ_TYPE_INFO(PrimitiveAssetParams, "{55DDE8EE-CEDF-4085-B7CF-B874CC7A5F74}");

            PrimitiveAssetParams() = default;

            static void Reflect(AZ::ReflectContext* context);

            PrimitiveShapeTarget GetPrimitiveShapeTarget() const;
            float GetVolumeTermCoefficient() const;

        private:
            PrimitiveShapeTarget m_primitiveShapeTarget = PrimitiveShapeTarget::BestFit;
            float m_volumeTermCoefficient = 0.0f;
        };

        class ConvexDecompositionParams
        {
        friend MeshGroup;

        public:
            AZ_TYPE_INFO(ConvexDecompositionParams, "{E076A8BC-5409-4125-B2B7-35500AF33BC2}");

            ConvexDecompositionParams() = default;

            static void Reflect(AZ::ReflectContext* context);

            float GetConcavity() const;
            float GetAlpha() const;
            float GetBeta() const;
            float GetMinVolumePerConvexHull() const;
            AZ::u32 GetResolution() const;
            AZ::u32 GetMaxNumVerticesPerConvexHull() const;
            AZ::u32 GetPlaneDownsampling() const;
            AZ::u32 GetConvexHullDownsampling() const;
            AZ::u32 GetMaxConvexHulls() const;
            bool GetPca() const;
            AZ::u32 GetMode() const;
            bool GetProjectHullVertices() const;

        private:
            AZ::u32 m_maxConvexHulls = 1024;
            AZ::u32 m_maxNumVerticesPerConvexHull = 64;
            float m_concavity = 0.001f;
            AZ::u32 m_resolution = 100000;
            AZ::u32 m_mode = 0;
            float m_alpha = 0.05f;
            float m_beta = 0.05f;
            float m_minVolumePerConvexHull = 0.0001f;
            AZ::u32 m_planeDownsampling = 4;
            AZ::u32 m_convexHullDownsampling = 4;
            bool m_pca = 0;
            bool m_projectHullVertices = true;
        };

        class MeshGroup
            : public AZ::SceneAPI::DataTypes::ISceneNodeGroup
        {
        public:
            AZ_RTTI(MeshGroup, "{5B03C8E6-8CEE-4DA0-A7FA-CD88689DD45B}", AZ::SceneAPI::DataTypes::ISceneNodeGroup);
            AZ_CLASS_ALLOCATOR_DECL

            MeshGroup();
            ~MeshGroup() override;

            static void Reflect(AZ::ReflectContext* context);

            const AZStd::string& GetName() const override;
            void SetName(const AZStd::string& name);
            void SetName(AZStd::string&& name);
            const AZ::Uuid& GetId() const override;
            void OverrideId(const AZ::Uuid& id);
            bool GetExportAsConvex() const;
            bool GetExportAsTriMesh() const;
            bool GetExportAsPrimitive() const;
            bool GetDecomposeMeshes() const;
            const Physics::MaterialSlots& GetMaterialSlots() const;

            void SetSceneGraph(const AZ::SceneAPI::Containers::SceneGraph* graph);
            void UpdateMaterialSlots();

            AZ::SceneAPI::Containers::RuleContainer& GetRuleContainer() override;
            const AZ::SceneAPI::Containers::RuleContainer& GetRuleContainerConst() const override;

            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

            TriangleMeshAssetParams& GetTriangleMeshAssetParams();
            const TriangleMeshAssetParams& GetTriangleMeshAssetParams() const;

            ConvexAssetParams& GetConvexAssetParams();
            const ConvexAssetParams& GetConvexAssetParams() const;

            PrimitiveAssetParams& GetPrimitiveAssetParams();
            const PrimitiveAssetParams& GetPrimitiveAssetParams() const;

            ConvexDecompositionParams& GetConvexDecompositionParams();
            const ConvexDecompositionParams& GetConvexDecompositionParams() const;

        protected:

            AZ::u32 OnNodeSelectionChanged();
            AZ::u32 OnExportMethodChanged();
            AZ::u32 OnDecomposeMeshesChanged();

            bool GetDecomposeMeshesVisibility() const;

            AZ::Uuid m_id{};
            AZStd::string m_name{};
            AZ::SceneAPI::SceneData::SceneNodeSelectionList m_nodeSelectionList{};
            MeshExportMethod m_exportMethod{};
            bool m_decomposeMeshes{false};
            TriangleMeshAssetParams m_triangleMeshAssetParams{};
            ConvexAssetParams m_convexAssetParams{};
            PrimitiveAssetParams m_primitiveAssetParams{};
            ConvexDecompositionParams m_convexDecompositionParams{};
            AZ::SceneAPI::Containers::RuleContainer m_rules{};
            Physics::MaterialSlots m_physicsMaterialSlots;

            const AZ::SceneAPI::Containers::SceneGraph* m_graph = nullptr;
        };
    }
}
