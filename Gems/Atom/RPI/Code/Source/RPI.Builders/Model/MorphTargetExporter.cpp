/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Model/MorphTargetExporter.h>
#include <Atom/RPI.Reflect/Model/MorphTargetDelta.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AZ::RPI
{
    using namespace AZ::SceneAPI;

    AZStd::unordered_map<AZStd::string, MorphTargetExporter::SourceBlendShapeInfo> MorphTargetExporter::GetBlendShapeInfos(
        const Containers::Scene& scene,
        const MeshData* meshData) const
    {
        const Containers::SceneGraph& sceneGraph = scene.GetGraph();

        const auto foundBaseMeshIter = AZStd::find_if(sceneGraph.GetContentStorage().cbegin(), sceneGraph.GetContentStorage().cend(), [meshData](const auto& nodeData)
        {
            return nodeData.get() == meshData;
        });
        if (foundBaseMeshIter == sceneGraph.GetContentStorage().cend())
        {
            return {};
        }

        const auto baseMeshNodeIndex = sceneGraph.ConvertToNodeIndex(foundBaseMeshIter);

        const auto childBlendShapeDatas = Containers::MakeDerivedFilterView<DataTypes::IBlendShapeData>(
            Containers::Views::MakeSceneGraphChildView(sceneGraph, baseMeshNodeIndex, sceneGraph.GetContentStorage().cbegin(), true)
        );

        AZStd::unordered_map<AZStd::string, SourceBlendShapeInfo> result;
        for (auto it = childBlendShapeDatas.cbegin(); it != childBlendShapeDatas.cend(); ++it)
        {
            const Containers::SceneGraph::NodeIndex blendShapeNodeIndex = sceneGraph.ConvertToNodeIndex(it.GetBaseIterator().GetBaseIterator().GetHierarchyIterator());

            AZStd::set<AZ::Crc32> types;
            Events::GraphMetaInfoBus::Broadcast(&Events::GraphMetaInfo::GetVirtualTypes, types, scene, blendShapeNodeIndex);
            if (!types.contains(Events::GraphMetaInfo::GetIgnoreVirtualType()))
            {
                const AZStd::string blendShapeName{sceneGraph.GetNodeName(blendShapeNodeIndex).GetName(), sceneGraph.GetNodeName(blendShapeNodeIndex).GetNameLength()};
                result[blendShapeName].m_sceneNodeIndices.emplace_back(blendShapeNodeIndex);
            }
        }

        return result;
    }

    void MorphTargetExporter::ProduceMorphTargets(const Containers::Scene& scene,
        uint32_t vertexOffset,
        const ModelAssetBuilderComponent::SourceMeshContent& sourceMesh,
        ModelAssetBuilderComponent::ProductMeshContent& productMesh,
        MorphTargetMetaAssetCreator& metaAssetCreator,
        const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter)
    {
        const Containers::SceneGraph& sceneGraph = scene.GetGraph();

        const auto baseMeshIt = AZStd::find(sceneGraph.GetContentStorage().cbegin(), sceneGraph.GetContentStorage().cend(), sourceMesh.m_meshData);
        const Containers::SceneGraph::NodeIndex baseMeshIndex = sceneGraph.ConvertToNodeIndex(baseMeshIt);
        const AZStd::string_view baseMeshName{sceneGraph.GetNodeName(baseMeshIndex).GetName(), sceneGraph.GetNodeName(baseMeshIndex).GetNameLength()};

        // Get the blend shapes for the given mesh
        AZStd::unordered_map<AZStd::string, SourceBlendShapeInfo> blendShapeInfos = GetBlendShapeInfos(scene, sourceMesh.m_meshData.get());

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
                    const AZStd::string_view sourceMeshName{sceneGraph.GetNodeName(morphMeshParentIndex).GetName(), sceneGraph.GetNodeName(morphMeshParentIndex).GetNameLength()};

                    AZ_Assert(AZ::StringFunc::Equal(baseMeshName, sourceMeshName, /*bCaseSensitive=*/true),
                        "Scene graph mesh node (%.*s) has a different name than the product mesh (%.*s).",
                        AZ_STRING_ARG(sourceMeshName), AZ_STRING_ARG(baseMeshName));

                    const DataTypes::MatrixType globalTransform = Utilities::BuildWorldTransform(sceneGraph, sceneNodeIndex);
                    BuildMorphTargetMesh(vertexOffset, sourceMesh, productMesh, metaAssetCreator, blendShapeName, blendShapeData, globalTransform, coordSysConverter, scene.GetSourceFilename());
                }
            }
        }
    }

    float MorphTargetExporter::CalcPositionDeltaTolerance(const ModelAssetBuilderComponent::SourceMeshContent& mesh) const
    {
        AZ::Aabb meshAabb = AZ::Aabb::CreateNull();

        const unsigned int numVertices = static_cast<unsigned int>(mesh.m_meshData->GetVertexCount());
        for (unsigned int i = 0; i < numVertices; ++i)
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
        const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter,
        const AZStd::string& sourceSceneFilename)
    {
        const float tolerance = CalcPositionDeltaTolerance(sourceMesh);
        AZ::Aabb deltaPositionAabb = AZ::Aabb::CreateNull();

        AZStd::vector<PackedCompressedMorphTargetDelta>& packedCompressedMorphTargetVertexData = productMesh.m_morphTargetVertexData;

        MorphTargetMetaAsset::MorphTarget metaData;
        metaData.m_meshNodeName = sourceMesh.m_name.GetStringView();
        metaData.m_morphTargetName = blendShapeName;

        // Determine the vertex index range for the morph target.
        const uint32_t numVertices = blendShapeData->GetVertexCount();
        if (blendShapeData->GetVertexCount() != sourceMesh.m_meshData->GetVertexCount())
        {
            AZ_Error(ModelAssetBuilderComponent::s_builderName, false,
                "Skipping blend shape (%s) as it contains more/less vertices (%d) than the neutral mesh (%d). "
                "The blend shape is most likely influencing multiple meshes, which is currently not supported.",
                blendShapeName.c_str(), numVertices, sourceMesh.m_meshData->GetVertexCount());
            return;
        }

        // The start index is after any previously added deltas
        metaData.m_startIndex = aznumeric_cast<uint32_t>(packedCompressedMorphTargetVertexData.size());


        // Multiply normal by inverse transpose to avoid incorrect values produced by non-uniformly scaled transforms.
        DataTypes::MatrixType globalTransformN = globalTransform.GetInverseFull().GetTranspose();
        globalTransformN.SetTranslation(AZ::Vector3::CreateZero());

        AZStd::vector<AZ::Vector3> uncompressedPositionDeltas;
        uncompressedPositionDeltas.reserve(numVertices);

        AZStd::vector<CompressedMorphTargetDelta> compressedDeltas;
        compressedDeltas.reserve(numVertices);

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
                // Add a new delta
                numMorphedVertices++;
                compressedDeltas.emplace_back(RPI::CompressedMorphTargetDelta{});
                RPI::CompressedMorphTargetDelta& currentDelta = compressedDeltas.back();

                // Set the morphed index
                currentDelta.m_morphedVertexIndex = vertexIndex + vertexOffset;

                const AZ::Vector3 deltaPosition = targetPosition - neutralPosition;
                deltaPositionAabb.AddPoint(deltaPosition);

                // We can't compress the positions until they've all been gathered so we know the min/max,
                // so keep track of the uncompressed positions for now
                uncompressedPositionDeltas.emplace_back(deltaPosition);

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

                    currentDelta.m_normalX = Compress<uint8_t>(deltaNormal.GetX(), MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                    currentDelta.m_normalY = Compress<uint8_t>(deltaNormal.GetY(), MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                    currentDelta.m_normalZ = Compress<uint8_t>(deltaNormal.GetZ(), MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                }

                // Tangent
                {
                    // Insert zero-delta until morphed tangents are supported in SceneAPI
                    currentDelta.m_tangentX = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                    currentDelta.m_tangentY = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                    currentDelta.m_tangentZ = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                }

                // Bitangent
                {
                    // Insert zero-delta until morphed bitangents are supported in SceneAPI
                    currentDelta.m_bitangentX = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                    currentDelta.m_bitangentY = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                    currentDelta.m_bitangentZ = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_tangentSpaceDeltaMin, MorphTargetDeltaConstants::s_tangentSpaceDeltaMax);
                }

                // Color
                {
                    metaData.m_hasColorDeltas = true;
                    productMesh.m_hasMorphedColors = true;
                    currentDelta.m_colorR = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_colorDeltaMin, MorphTargetDeltaConstants::s_colorDeltaMax);
                    currentDelta.m_colorG = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_colorDeltaMin, MorphTargetDeltaConstants::s_colorDeltaMax);
                    currentDelta.m_colorB = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_colorDeltaMin, MorphTargetDeltaConstants::s_colorDeltaMax);
                    currentDelta.m_colorA = Compress<uint8_t>(0.0f, MorphTargetDeltaConstants::s_colorDeltaMin, MorphTargetDeltaConstants::s_colorDeltaMax);
                }
            }
        }

        metaData.m_numVertices = numMorphedVertices;

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

        metaData.m_wrinkleMask = GetWrinkleMask(sourceSceneFilename, blendShapeName);

        metaAssetCreator.AddMorphTarget(metaData);

        AZ_Assert(uncompressedPositionDeltas.size() == compressedDeltas.size(), "Number of uncompressed (%d) and compressed position delta components (%d) do not match.",
            uncompressedPositionDeltas.size(), compressedDeltas.size());

        // Compress the position deltas. (Only the newly added ones from this morph target)
        for (size_t i = 0; i < uncompressedPositionDeltas.size(); i++)
        {
            compressedDeltas[i].m_positionX = Compress<uint16_t>(uncompressedPositionDeltas[i].GetX(), metaData.m_minPositionDelta, metaData.m_maxPositionDelta);
            compressedDeltas[i].m_positionY = Compress<uint16_t>(uncompressedPositionDeltas[i].GetY(), metaData.m_minPositionDelta, metaData.m_maxPositionDelta);
            compressedDeltas[i].m_positionZ = Compress<uint16_t>(uncompressedPositionDeltas[i].GetZ(), metaData.m_minPositionDelta, metaData.m_maxPositionDelta);
        }

        // Now that we have all our compressed deltas, they need to be packed
        // the way the shader expects to read them and added to the product mesh
        packedCompressedMorphTargetVertexData.reserve(packedCompressedMorphTargetVertexData.size() + compressedDeltas.size());
        for (size_t i = 0; i < compressedDeltas.size(); ++i)
        {
            packedCompressedMorphTargetVertexData.emplace_back(RPI::PackMorphTargetDelta(compressedDeltas[i]));
        }

        AZ_Assert((packedCompressedMorphTargetVertexData.size() - metaData.m_startIndex) == numMorphedVertices, "Vertex index range (%d) in morph target meta data does not match number of morphed vertices (%d).",
            packedCompressedMorphTargetVertexData.size() - metaData.m_startIndex, numMorphedVertices);
    }

    Data::Asset<RPI::StreamingImageAsset> MorphTargetExporter::GetWrinkleMask(const AZStd::string& sourceSceneFullFilePath, const AZStd::string& blendShapeName) const
    {
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> imageAsset;

        // See if there is a wrinkle map mask for this mesh
        AZStd::string sceneRelativeFilePath;
        bool relativePathFound = true;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(relativePathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, sourceSceneFullFilePath, sceneRelativeFilePath);

        if (relativePathFound)
        {
            AZ::StringFunc::Path::StripFullName(sceneRelativeFilePath);

            // Get the folder the masks are supposed to be in
            AZStd::string folderName;
            AZ::StringFunc::Path::GetFileName(sourceSceneFullFilePath.c_str(), folderName);
            folderName += "_wrinklemasks";

            // Note: for now, we're assuming the mask is always authored as a .tif
            AZStd::string blendMaskFileName = blendShapeName + "_wrinklemask.tif.streamingimage";

            AZStd::string maskFolderAndFile;
            AZ::StringFunc::Path::Join(folderName.c_str(), blendMaskFileName.c_str(), maskFolderAndFile);

            AZStd::string maskRelativePath;
            AZ::StringFunc::Path::Join(sceneRelativeFilePath.c_str(), maskFolderAndFile.c_str(), maskRelativePath);
            AZ::StringFunc::Path::Normalize(maskRelativePath);

            // Now see if the file exists
            AZ::Data::AssetId maskAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(maskAssetId, &Data::AssetCatalogRequests::GetAssetIdByPath, maskRelativePath.c_str(), AZ::Data::s_invalidAssetType, false);

            if (maskAssetId.IsValid())
            {
                // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
                AZ::Data::AssetManager::Instance().DispatchEvents();

                imageAsset.Create(maskAssetId, AZ::Data::AssetLoadBehavior::PreLoad, false);
            }
        }
        return imageAsset;
    }
} // namespace AZ::RPI
