/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAssetCreator.h>
#include <Model/ModelAssetBuilderComponent.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    namespace RPI
    {
        class MorphTargetExporter
        {
        public:
            AZ_RTTI(MorphTargetExporter, "{A684EBE7-03A2-4877-B6F7-83FC0029CC38}");

            void ProduceMorphTargets(
                uint32_t productMeshIndex,
                uint32_t startVertex,
                const AZStd::map<uint32_t, uint32_t>& oldToNewIndicesMap,
                const AZ::SceneAPI::Containers::Scene& scene,
                const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
                ModelAssetBuilderComponent::ProductMeshContent& productMesh,
                MorphTargetMetaAssetCreator& metaAssetCreator,
                const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter);

        private:
            struct SourceBlendShapeInfo
            {
                AZStd::vector<AZ::SceneAPI::Containers::SceneGraph::NodeIndex> m_sceneNodeIndices;
            };
            //! Retrieve all scene graph nodes per blend shape for all available blend shapes.
            AZStd::unordered_map<AZStd::string, SourceBlendShapeInfo> GetBlendShapeInfos(const AZ::SceneAPI::Containers::Scene& scene, const MeshData* meshData) const;

            //! Calculate position delta tolerance that is used to indicate whether a given vertex is part of the sparse set of morphed vertices
            //! or if it will be skipped and optimized out due to a hardly visible or no movement at all.
            float CalcPositionDeltaTolerance(const ModelAssetBuilderComponent::SourceMeshContent& mesh) const;

            static constexpr inline float s_positionDeltaTolerance = 0.0025f;

            //! Compress float value by using the full range of the storage type as a normalized value within range [minValue, maxValue].
            template <class StorageType>
            static StorageType Compress(float value, float minValue, float maxValue);

            // Extract the morph target vertex and meta data and save it into the product mesh content.
            void BuildMorphTargetMesh(
                uint32_t productMeshIndex,
                uint32_t startVertex,
                const AZStd::map<uint32_t, uint32_t>& oldToNewIndicesMap,
                const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
                ModelAssetBuilderComponent::ProductMeshContent& productMesh,
                MorphTargetMetaAssetCreator& metaAssetCreator,
                const AZStd::string& blendShapeName,
                const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IBlendShapeData>& blendShapeData,
                const AZ::SceneAPI::DataTypes::MatrixType& globalTransform,
                const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter,
                const AZStd::string& sourceSceneFilename);

            // Find a wrinkle mask for this morph target, if it exists
            Data::Asset<RPI::StreamingImageAsset> GetWrinkleMask(const AZStd::string& sourceSceneFullFilePath, const AZStd::string& blendShapeName) const;
        };
    } // namespace RPI
} // namespace AZ
