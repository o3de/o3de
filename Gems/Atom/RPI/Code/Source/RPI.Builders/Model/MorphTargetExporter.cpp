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

#include <Model/MorphTargetExporter.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>

namespace AZ::RPI
{
    using namespace AZ::SceneAPI;

    AZStd::unordered_map<AZStd::string, MorphTargetExporter::SourceBlendShapeInfo> MorphTargetExporter::GetBlendShapeInfos(
        const Containers::Scene& scene,
        const AZStd::optional<AZStd::string>& filterMeshName) const
    {
        const Containers::SceneGraph& sceneGraph = scene.GetGraph();
        const auto contentStorage = sceneGraph.GetContentStorage();
        const auto nameStorage = sceneGraph.GetNameStorage();

        AZStd::unordered_map<AZStd::string, SourceBlendShapeInfo> result;

        const auto keyValueView = Containers::Views::MakePairView(nameStorage, contentStorage);
        const auto filteredView = Containers::Views::MakeFilterView(keyValueView, Containers::DerivedTypeFilter<DataTypes::IBlendShapeData>());
        for (const auto& [name, object] : filteredView)
        {
            const Containers::SceneGraph::NodeIndex sceneNodeIndex = sceneGraph.Find(name.GetPath());

            AZStd::set<AZ::Crc32> types;
            Events::GraphMetaInfoBus::Broadcast(&Events::GraphMetaInfo::GetVirtualTypes, types, scene, sceneNodeIndex);
            if (types.find(Events::GraphMetaInfo::GetIgnoreVirtualType()) == types.end())
            {
                const char* sceneNodePath = name.GetPath();
                const Containers::SceneGraph::NodeIndex nodeIndex = sceneGraph.Find(sceneNodePath);
                if (nodeIndex.IsValid())
                {
                    const AZStd::string meshNodeName = SourceBlendShapeInfo::GetMeshNodeName(sceneGraph, nodeIndex);
                    if (!filterMeshName.has_value() ||
                        (filterMeshName.has_value() && filterMeshName.value() == meshNodeName))
                    {
                        const AZStd::string blendShapeName = sceneGraph.GetNodeName(nodeIndex).GetName();
                        SourceBlendShapeInfo& blendShapeInfo = result[blendShapeName];
                        blendShapeInfo.m_sceneNodeIndices.push_back(nodeIndex);
                    }
                }
                else
                {
                    AZ_Warning(ModelAssetBuilderComponent::s_builderName, false, "Cannot retrieve scene graph index for blend shape node with path %s.", sceneNodePath);
                }
            }
        }

        return result;
    }

    AZStd::string MorphTargetExporter::SourceBlendShapeInfo::GetMeshNodeName(const Containers::SceneGraph& sceneGraph,
        const Containers::SceneGraph::NodeIndex& sceneNodeIndex)
    {
        const auto* blendShapeData =
            azrtti_cast<const DataTypes::IBlendShapeData*>(sceneGraph.GetNodeContent(sceneNodeIndex).get());
        AZ_Assert(blendShapeData, "Cannot get mesh node name from scene node. Node is expected to be a blend shape.");
        if (blendShapeData)
        {
            Containers::SceneGraph::NodeIndex morphMeshParentIndex = sceneGraph.GetNodeParent(sceneNodeIndex);
            return sceneGraph.GetNodeName(morphMeshParentIndex).GetName();
        }

        return {};
    }

    void MorphTargetExporter::ProduceMorphTargets(const Containers::Scene& scene,
        uint32_t vertexOffset,
        const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
        ModelAssetBuilderComponent::ProductMeshContent& productMesh,
        MorphTargetMetaAssetCreator& metaAssetCreator,
        const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter)
    {
        const Containers::SceneGraph& sceneGraph = scene.GetGraph();

        // Get the blend shapes for the given mesh
        const AZStd::string_view meshName = sourceMesh.m_name.GetStringView();
        AZStd::unordered_map<AZStd::string, SourceBlendShapeInfo> blendShapeInfos = GetBlendShapeInfos(scene, meshName);

        for (const auto& iter : blendShapeInfos)
        {
            const AZStd::string& blendShapeName = iter.first;

            for (const SceneAPI::Containers::SceneGraph::NodeIndex& sceneNodeIndex : iter.second.m_sceneNodeIndices)
            {
                const AZStd::shared_ptr<const DataTypes::IBlendShapeData>& blendShapeData =
                    azrtti_cast<const DataTypes::IBlendShapeData*>(sceneGraph.GetNodeContent(sceneNodeIndex));
                AZ_Assert(blendShapeData, "Node is expected to be a blend shape.");
                if (blendShapeData)
                {
                    const Containers::SceneGraph::NodeIndex morphMeshParentIndex = sceneGraph.GetNodeParent(sceneNodeIndex);
                    const char* meshNodeName = sceneGraph.GetNodeName(morphMeshParentIndex).GetName();

                    AZ_Assert(AZ::StringFunc::Equal(sourceMesh.m_name.GetCStr(), meshNodeName, /*bCaseSensitive=*/true),
                        "Scene graph mesh node (%s) has a different name than the product mesh (%s).",
                        meshNodeName, sourceMesh.m_name.GetCStr());

                    const DataTypes::MatrixType globalTransform = Utilities::BuildWorldTransform(sceneGraph, sceneNodeIndex);
                    BuildMorphTargetMesh(vertexOffset, sourceMesh, productMesh, metaAssetCreator, blendShapeName, blendShapeData, globalTransform, coordSysConverter);
                }
            }
        }
    }

    float MorphTargetExporter::CalcPositionDeltaTolerance(const ModelAssetBuilderComponent::SourceMeshContent& mesh) const
    {
        AZ::Aabb meshAabb = AZ::Aabb::CreateNull();

        const size_t numVertices = mesh.m_meshData->GetVertexCount();
        for (size_t i = 0; i < numVertices; ++i)
        {
            meshAabb.AddPoint(mesh.m_meshData->GetPosition(i));
        }
        const float radius = (meshAabb.GetMax() - meshAabb.GetMin()).GetLength() * 0.5f;

        float tolerance = radius * s_positionDeltaTolerance; // TODO: Value needs further consideration but is proven to work for EMotion FX.
        if (tolerance < AZ::Constants::FloatEpsilon)
        {
            tolerance = AZ::Constants::FloatEpsilon;
        }

        return tolerance;
    }

    template <class StorageType>
    StorageType MorphTargetExporter::Compress(float value, float minValue, float maxValue)
    {
        // Number of steps within the specified range
        constexpr uint32_t CONVERT_VALUE = (1 << (sizeof(StorageType) << 3)) - 1;

        const float f = static_cast<float>(CONVERT_VALUE) / (maxValue - minValue);
        return static_cast<StorageType>((value - minValue) * f);
    };

    void MorphTargetExporter::BuildMorphTargetMesh(uint32_t vertexOffset,
        const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
        ModelAssetBuilderComponent::ProductMeshContent& productMesh,
        MorphTargetMetaAssetCreator& metaAssetCreator,
        const AZStd::string& blendShapeName,
        const AZStd::shared_ptr<const DataTypes::IBlendShapeData>& blendShapeData,
        const DataTypes::MatrixType& globalTransform,
        const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter)
    {
        const float tolerance = CalcPositionDeltaTolerance(sourceMesh);
        AZ::Aabb deltaPositionAabb = AZ::Aabb::CreateNull();

        const uint32_t numFaces = blendShapeData->GetFaceCount();

        ModelAssetBuilderComponent::ProductMeshContent::MorphTargetVertexData& morphTargetVertexData = productMesh.m_morphTargetVertexData;
        AZStd::vector<uint32_t>& vertexIndices = morphTargetVertexData.m_vertexIndices;
        AZStd::vector<float>& uncompressedPositionDeltas = morphTargetVertexData.m_uncompressedPositionDeltas;
        AZStd::vector<uint16_t>& positionDeltas = morphTargetVertexData.m_positionDeltas;
        AZStd::vector<uint8_t>& normalDeltas = morphTargetVertexData.m_normalDeltas;

        MorphTargetMetaAsset::MorphTarget metaData;
        metaData.m_meshNodeName = sourceMesh.m_name.GetStringView();
        metaData.m_morphTargetName = blendShapeName;

        // Determine the vertex index range for the morph target.
        const uint32_t numVertices = blendShapeData->GetVertexCount();
        AZ_Assert(blendShapeData->GetVertexCount() == sourceMesh.m_meshData->GetVertexCount(),
            "Blend shape (%s) contains more/less vertices (%d) than the neutral mesh (%d).",
            blendShapeName.c_str(), numVertices, sourceMesh.m_meshData->GetVertexCount());
        metaData.m_startIndex = aznumeric_caster<uint32_t>(vertexIndices.size());


        // Multiply normal by inverse transpose to avoid incorrect values produced by non-uniformly scaled transforms.
        DataTypes::MatrixType globalTransformN = globalTransform.GetInverseFull().GetTranspose();
        globalTransformN.SetTranslation(AZ::Vector3::CreateZero());

        uint32_t numMorphedVertices = 0;
        for (uint32_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
        {
            AZ::Vector3 targetPosition = blendShapeData->GetPosition(vertexIndex);
            targetPosition = globalTransform * targetPosition;
            targetPosition = coordSysConverter.ConvertVector3(targetPosition);

            AZ::Vector3 neutralPosition = sourceMesh.m_meshData->GetPosition(vertexIndex);
            neutralPosition = globalTransform * neutralPosition;
            neutralPosition = coordSysConverter.ConvertVector3(neutralPosition);

            // Check if the vertex positions are different.
            if (!targetPosition.IsClose(neutralPosition, tolerance))
            {
                vertexIndices.emplace_back(vertexIndex + vertexOffset);
                numMorphedVertices++;

                const AZ::Vector3 deltaPosition = targetPosition - neutralPosition;
                deltaPositionAabb.AddPoint(deltaPosition);
                uncompressedPositionDeltas.emplace_back(deltaPosition.GetX());
                uncompressedPositionDeltas.emplace_back(deltaPosition.GetY());
                uncompressedPositionDeltas.emplace_back(deltaPosition.GetZ());

                // Normal
                {
                    AZ::Vector3 neutralNormal = sourceMesh.m_meshData->GetNormal(vertexIndex);
                    neutralNormal = globalTransformN * neutralNormal;
                    neutralNormal = coordSysConverter.ConvertVector3(neutralNormal);

                    AZ::Vector3 targetNormal = blendShapeData->GetNormal(vertexIndex);
                    targetNormal = globalTransformN * targetNormal;
                    targetNormal = coordSysConverter.ConvertVector3(targetNormal);
                    targetNormal.NormalizeSafe();

                    const AZ::Vector3 deltaNormal = targetNormal - neutralNormal;

                    normalDeltas.emplace_back(Compress<uint8_t>(deltaNormal.GetX(), -2.0f, 2.0f));
                    normalDeltas.emplace_back(Compress<uint8_t>(deltaNormal.GetY(), -2.0f, 2.0f));
                    normalDeltas.emplace_back(Compress<uint8_t>(deltaNormal.GetZ(), -2.0f, 2.0f));
                }
            }
        }

        metaData.m_numVertices = numMorphedVertices;
        AZ_Assert((vertexIndices.size() - metaData.m_startIndex) == numMorphedVertices, "Vertex index range (%d) in morph target meta data does not match number of morphed vertices (%d).",
            vertexIndices.size() - metaData.m_startIndex, numMorphedVertices);

        const float morphedVerticesRatio = numMorphedVertices / static_cast<float>(numVertices);
        AZ_Printf(ModelAssetBuilderComponent::s_builderName, "'%s' morphs %.1f%% of the vertices.", blendShapeName.c_str(), morphedVerticesRatio * 100.0f);

        // Calculate the minimum and maximum position compression values.
        {
            const AZ::Vector3& boxMin = deltaPositionAabb.GetMin();
            const AZ::Vector3& boxMax = deltaPositionAabb.GetMax();
            auto [minValue, maxValue] = AZStd::minmax({boxMin.GetX(), boxMin.GetY(), boxMin.GetZ(),
                boxMax.GetX(), boxMax.GetY(), boxMax.GetZ()});

            // Make sure the diff between min and max isn't too small.
            if (maxValue - minValue < 1.0f)
            {
                minValue -= 0.5f; // TODO: Value needs further consideration but is proven to work for EMotion FX.
                maxValue += 0.5f;
            }

            metaData.m_minPositionDelta = minValue;
            metaData.m_maxPositionDelta = maxValue;
        }

        metaAssetCreator.AddMorphTarget(metaData);

        // Compress the position deltas. (Only the newly added ones from this morph target)
        for (size_t i = metaData.m_startIndex * 3; i < uncompressedPositionDeltas.size(); i++)
        {
            positionDeltas.emplace_back(Compress<uint16_t>(uncompressedPositionDeltas[i], metaData.m_minPositionDelta, metaData.m_maxPositionDelta));
        }
        AZ_Assert(uncompressedPositionDeltas.size() == positionDeltas.size(), "Number of uncompressed (%d) and compressed position delta components (%d) do not match.",
            uncompressedPositionDeltas.size(), positionDeltas.size());

        AZ_Assert(positionDeltas.size() == normalDeltas.size(), "Number of position deltas needs to match the number of normal deltas.");
        AZ_Assert((positionDeltas.size() / 3) == vertexIndices.size(), "Position delta component count needs to be three times the number of vertex indices.");
    }
} // namespace AZ::RPI
