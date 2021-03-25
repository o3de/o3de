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

            void ProduceMorphTargets(const AZ::SceneAPI::Containers::Scene& scene,
                uint32_t vertexOffset,
                const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
                ModelAssetBuilderComponent::ProductMeshContent& productMesh,
                MorphTargetMetaAssetCreator& metaAssetCreator,
                const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter);

        private:
            struct SourceBlendShapeInfo
            {
                AZStd::vector<AZ::SceneAPI::Containers::SceneGraph::NodeIndex> m_sceneNodeIndices;

                static AZStd::string GetMeshNodeName(const AZ::SceneAPI::Containers::SceneGraph& sceneGraph,
                    const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& sceneNodeIndex);
            };
            //! Retrieve all scene graph nodes per blend shape for all available blend shapes.
            AZStd::unordered_map<AZStd::string, SourceBlendShapeInfo> GetBlendShapeInfos(const AZ::SceneAPI::Containers::Scene& scene, const AZStd::optional<AZStd::string>& filterMeshName = AZStd::nullopt) const;

            //! Calculate position delta tolerance that is used to indicate whether a given vertex is part of the sparse set of morphed vertices
            //! or if it will be skipped and optimized out due to a hardly visible or no movement at all.
            float CalcPositionDeltaTolerance(const ModelAssetBuilderComponent::SourceMeshContent& mesh) const;

            static constexpr inline float s_positionDeltaTolerance = 0.0025f;

            //! Compress float value by using the full range of the storage type as a normalized value within range [minValue, maxValue].
            template <class StorageType>
            static StorageType Compress(float value, float minValue, float maxValue);

            // Extract the morph target vertex and meta data and save it into the product mesh content.
            void BuildMorphTargetMesh(uint32_t vertexOffset,
                const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
                ModelAssetBuilderComponent::ProductMeshContent& productMesh,
                MorphTargetMetaAssetCreator& metaAssetCreator,
                const AZStd::string& blendShapeName,
                const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IBlendShapeData>& blendShapeData,
                const AZ::SceneAPI::DataTypes::MatrixType& globalTransform,
                const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter);
        };
    } // namespace RPI
} // namespace AZ
