/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneData/GraphData/AnimationData.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            static const float g_sceneUtilityEqualityEpsilon = 0.001f;

            CoreProcessingResult AddDataNodeWithContexts(SceneDataPopulatedContextBase& dataPopulated)
            {
                AZ_TraceContext("Node Name", dataPopulated.m_dataName);
                const char* nodeTypeName = dataPopulated.m_graphData ? dataPopulated.m_graphData->RTTI_GetTypeName() : "Null";
                AZ_TraceContext("Node Type", (!nodeTypeName || nodeTypeName[0] == '\0' ? "Null" : nodeTypeName));
                
                Events::ProcessingResultCombiner nodeResults;
                nodeResults += Events::Process(dataPopulated);

                dataPopulated.m_scene.GetGraph().SetContent(dataPopulated.m_currentGraphPosition,
                    AZStd::move(dataPopulated.m_graphData));

                if (azrtti_istypeof<AssImpSceneDataPopulatedContext>(dataPopulated))
                {
                    AssImpSceneDataPopulatedContext* dataPopulatedContext = azrtti_cast<AssImpSceneDataPopulatedContext*>(&dataPopulated);
                    AssImpSceneNodeAppendedContext nodeAppended(*dataPopulatedContext, dataPopulated.m_currentGraphPosition);
                    nodeResults += Events::Process(nodeAppended);

                    AssImpSceneNodeAddedAttributesContext addedAttributes(nodeAppended);
                    nodeResults += Events::Process(addedAttributes);

                    AssImpSceneNodeFinalizeContext finalizeNode(addedAttributes);
                    nodeResults += Events::Process(finalizeNode);
                }

                return nodeResults.GetResult();
            }

            CoreProcessingResult AddAttributeDataNodeWithContexts(SceneAttributeDataPopulatedContextBase& dataPopulated)
            {
                AZ_TraceContext("Node Name", dataPopulated.m_dataName);
                const char* nodeTypeName = dataPopulated.m_graphData ? dataPopulated.m_graphData->RTTI_GetTypeName() : "Null";
                AZ_TraceContext("Node Type", (!nodeTypeName || nodeTypeName[0] == '\0' ? "Null" : nodeTypeName));

                Events::ProcessingResultCombiner nodeResults;
                nodeResults += Events::Process(dataPopulated);

                dataPopulated.m_scene.GetGraph().MakeEndPoint(dataPopulated.m_currentGraphPosition);

                dataPopulated.m_scene.GetGraph().SetContent(dataPopulated.m_currentGraphPosition,
                    AZStd::move(dataPopulated.m_graphData));
                if (azrtti_istypeof<AssImpSceneAttributeDataPopulatedContext>(dataPopulated))
                {
                    AssImpSceneAttributeDataPopulatedContext* dataPopulatedContext = azrtti_cast<AssImpSceneAttributeDataPopulatedContext*>(&dataPopulated);
                    AssImpSceneAttributeNodeAppendedContext nodeAppended(*dataPopulatedContext, dataPopulated.m_currentGraphPosition);
                    nodeResults += Events::Process(nodeAppended);
                }

                return nodeResults.GetResult();
            }

            bool AreSceneGraphsEqual(const CoreSceneGraph& lhsGraph, const CoreSceneGraph& rhsGraph)
            {
                auto lhsContentStorage = lhsGraph.GetContentStorage();
                auto lhsNameStorage = lhsGraph.GetNameStorage();
                
                auto lhsNameContentView = Containers::Views::MakePairView(lhsNameStorage, lhsContentStorage);
                Containers::SceneGraph::NodeIndex lhsRootIndex = lhsGraph.GetRoot();
                auto lhsDownwardView =
                    Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(lhsGraph, lhsRootIndex,
                    lhsNameContentView.begin(), true);

                auto rhsContentStorage = rhsGraph.GetContentStorage();
                auto rhsNameStorage = rhsGraph.GetNameStorage();
                
                auto rhsNameContentView = Containers::Views::MakePairView(rhsNameStorage, rhsContentStorage);
                Containers::SceneGraph::NodeIndex rhsRootIndex = rhsGraph.GetRoot();
                auto rhsDownwardView =
                    Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(rhsGraph, rhsRootIndex,
                    rhsNameContentView.begin(), true);

                auto lhsIt = lhsDownwardView.begin();
                auto rhsIt = rhsDownwardView.begin();

                while (lhsIt != lhsDownwardView.end() && rhsIt != rhsDownwardView.end())
                {
                    if (!IsGraphDataEqual(lhsIt->second, rhsIt->second))
                    {
                        return false;
                    }
                    if (lhsIt->first != rhsIt->first)
                    {
                        return false;
                    }

                    ++lhsIt;
                    ++rhsIt;
                }

                return (lhsIt == lhsDownwardView.end() && rhsIt == rhsDownwardView.end());
            }
            
            bool operator==(const SceneData::GraphData::MeshData& lhs,
                const SceneData::GraphData::MeshData& rhs)
            {
                if (lhs.GetVertexCount() != rhs.GetVertexCount())
                {
                    return false;
                }

                if (lhs.HasNormalData() != rhs.HasNormalData())
                {
                    return false;
                }

                if (lhs.GetFaceCount() != rhs.GetFaceCount())
                {
                    return false;
                }

                bool hasNormals = lhs.HasNormalData();
                unsigned int vertexCount = lhs.GetVertexCount();
                for (unsigned int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    if (lhs.GetPosition(vertexIndex) != rhs.GetPosition(vertexIndex))
                    {
                        return false;
                    }

                    if (hasNormals && (lhs.GetNormal(vertexIndex) != rhs.GetNormal(vertexIndex)))
                    {
                        return false;
                    }
                }

                unsigned int faceCount = lhs.GetFaceCount();
                for (unsigned int faceIndex = 0; faceIndex < faceCount; ++faceIndex)
                {
                    if (lhs.GetFaceMaterialId(faceIndex) != rhs.GetFaceMaterialId(faceIndex))
                    {
                        return false;
                    }

                    if (lhs.GetFaceInfo(faceIndex) != rhs.GetFaceInfo(faceIndex))
                    {
                        return false;
                    }
                }

                return true;
            }

            bool operator==(const SceneData::GraphData::SkinWeightData& lhs,
                const SceneData::GraphData::SkinWeightData& rhs)
            {
                if (lhs.GetVertexCount() != rhs.GetVertexCount())
                {
                    return false;
                }

                if (lhs.GetBoneCount() != rhs.GetBoneCount())
                {
                    return false;
                }

                size_t vertexCount = lhs.GetVertexCount();
                for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    if (lhs.GetLinkCount(vertexIndex) != rhs.GetLinkCount(vertexIndex))
                    {
                        return false;
                    }

                    size_t linkCount = lhs.GetLinkCount(vertexIndex);
                    for (size_t linkIndex = 0; linkIndex < linkCount; ++linkIndex)
                    {
                        const DataTypes::ISkinWeightData::Link lhsLink = lhs.GetLink(vertexIndex, linkIndex);
                        const DataTypes::ISkinWeightData::Link rhsLink = rhs.GetLink(vertexIndex, linkIndex);

                        if (lhsLink.boneId != rhsLink.boneId || !IsClose(lhsLink.weight, rhsLink.weight, g_sceneUtilityEqualityEpsilon))
                        {
                            return false;
                        }

                        if (lhs.GetBoneName(lhsLink.boneId) != rhs.GetBoneName(rhsLink.boneId))
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            bool operator==(const SceneData::GraphData::BoneData& lhs,
                const SceneData::GraphData::BoneData& rhs)
            {
                return (lhs.GetWorldTransform() == rhs.GetWorldTransform());
            }

            bool operator==(const DataTypes::Color& lhs, const DataTypes::Color& rhs)
            {
                if (!IsClose(lhs.alpha, rhs.alpha, g_sceneUtilityEqualityEpsilon) ||
                    !IsClose(lhs.blue, rhs.blue, g_sceneUtilityEqualityEpsilon) ||
                    !IsClose(lhs.green, rhs.green, g_sceneUtilityEqualityEpsilon) ||
                    !IsClose(lhs.red, rhs.red, g_sceneUtilityEqualityEpsilon))
                {
                    return false;
                }

                return true;
            }

            bool operator!=(const DataTypes::Color& lhs, const DataTypes::Color& rhs)
            {
                return !(lhs == rhs);
            }

            bool operator==(const SceneData::GraphData::MeshVertexColorData& lhs,
                const SceneData::GraphData::MeshVertexColorData& rhs)
            {
                if (lhs.GetCount() != rhs.GetCount())
                {
                    return false;
                }

                size_t colorCount = lhs.GetCount();
                for (size_t colorIndex = 0; colorIndex < colorCount; ++colorIndex)
                {
                    if (lhs.GetColor(colorIndex) != rhs.GetColor(colorIndex))
                    {
                        return false;
                    }
                }

                return true;
            }

            bool operator==(const SceneData::GraphData::MeshVertexUVData& lhs,
                const SceneData::GraphData::MeshVertexUVData& rhs)
            {
                if (lhs.GetCount() != rhs.GetCount())
                {
                    return false;
                }
                size_t uvCount = lhs.GetCount();
                for (size_t uvIndex = 0; uvIndex < uvCount; ++uvIndex)
                {
                    if (lhs.GetUV(uvIndex) != rhs.GetUV(uvIndex))
                    {
                        return false;
                    }
                }

                return true;
            }

            bool operator==(const SceneData::GraphData::MaterialData& lhs,
                const SceneData::GraphData::MaterialData& rhs)
            {
                if (lhs.IsNoDraw() != rhs.IsNoDraw())
                {
                    return false;
                }

                if (lhs.GetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse) !=
                    rhs.GetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse))
                {
                    return false;
                }

                if (lhs.GetTexture(DataTypes::IMaterialData::TextureMapType::Specular) !=
                    rhs.GetTexture(DataTypes::IMaterialData::TextureMapType::Specular))
                {
                    return false;
                }

                if (lhs.GetTexture(DataTypes::IMaterialData::TextureMapType::Bump) !=
                    rhs.GetTexture(DataTypes::IMaterialData::TextureMapType::Bump))
                {
                    return false;
                }

                return true;
            }

            bool operator==(const SceneData::GraphData::TransformData& lhs,
                const SceneData::GraphData::TransformData& rhs)
            {
                return lhs.GetMatrix() == rhs.GetMatrix();
            }

            bool operator==(const SceneData::GraphData::AnimationData& lhs,
                const SceneData::GraphData::AnimationData& rhs)
            {
                if (lhs.GetKeyFrameCount() != rhs.GetKeyFrameCount())
                {
                    return false;
                }

                size_t keyFrameCount = lhs.GetKeyFrameCount();
                
                for (size_t keyFrameIndex = 0; keyFrameIndex < keyFrameCount; ++keyFrameIndex)
                {
                    if (lhs.GetKeyFrame(keyFrameIndex) != rhs.GetKeyFrame(keyFrameIndex))
                    {
                        return false;
                    }
                }

                return true;
            }

            bool IsGraphDataEqual(const AZStd::shared_ptr<const DataTypes::IGraphObject>& lhs,
                const AZStd::shared_ptr<const DataTypes::IGraphObject>& rhs)
            {
                // If both are null, they are considered equal
                if (!lhs && !rhs)
                {
                    return true;
                }

                // If only one is null, they are considered not equal
                if (!lhs || !rhs)
                {
                    return false;
                }

                // If they have disparate types they are considered not equal
                if (lhs->RTTI_GetType() != rhs->RTTI_GetType())
                {
                    return false;
                }

                if (lhs->RTTI_IsTypeOf(SceneData::GraphData::BoneData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::BoneData* lhsBone = 
                        azrtti_cast<const SceneData::GraphData::BoneData*>(lhs.get());
                    const SceneData::GraphData::BoneData* rhsBone =
                        azrtti_cast<const SceneData::GraphData::BoneData*>(rhs.get());

                    return (*lhsBone == *rhsBone);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::MeshData* lhsMesh =
                        azrtti_cast<const SceneData::GraphData::MeshData*>(lhs.get());
                    const SceneData::GraphData::MeshData* rhsMesh =
                        azrtti_cast<const SceneData::GraphData::MeshData*>(rhs.get());

                    return (*lhsMesh == *rhsMesh);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::SkinWeightData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::SkinWeightData* lhsSkinWeights =
                        azrtti_cast<const SceneData::GraphData::SkinWeightData*>(lhs.get());
                    const SceneData::GraphData::SkinWeightData* rhsSkinWeights =
                        azrtti_cast<const SceneData::GraphData::SkinWeightData*>(rhs.get());

                    return (*lhsSkinWeights == *rhsSkinWeights);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::MeshVertexColorData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::MeshVertexColorData* lhsColorData =
                        azrtti_cast<const SceneData::GraphData::MeshVertexColorData*>(lhs.get());
                    const SceneData::GraphData::MeshVertexColorData* rhsColorData =
                        azrtti_cast<const SceneData::GraphData::MeshVertexColorData*>(rhs.get());

                    return (*lhsColorData == *rhsColorData);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::MeshVertexUVData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::MeshVertexUVData* lhsUVData =
                        azrtti_cast<const SceneData::GraphData::MeshVertexUVData*>(lhs.get());
                    const SceneData::GraphData::MeshVertexUVData* rhsUVData =
                        azrtti_cast<const SceneData::GraphData::MeshVertexUVData*>(rhs.get());

                    return (*lhsUVData == *rhsUVData);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::MaterialData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::MaterialData* lhsMaterialData =
                        azrtti_cast<const SceneData::GraphData::MaterialData*>(lhs.get());
                    const SceneData::GraphData::MaterialData* rhsMaterialData =
                        azrtti_cast<const SceneData::GraphData::MaterialData*>(rhs.get());

                    return (*lhsMaterialData == *rhsMaterialData);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::TransformData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::TransformData* lhsTransform =
                        azrtti_cast<const SceneData::GraphData::TransformData*>(lhs.get());
                    const SceneData::GraphData::TransformData* rhsTransform =
                        azrtti_cast<const SceneData::GraphData::TransformData*>(rhs.get());

                    return (*lhsTransform == *rhsTransform);
                }
                else if (lhs->RTTI_IsTypeOf(SceneData::GraphData::AnimationData::TYPEINFO_Uuid()))
                {
                    const SceneData::GraphData::AnimationData* lhsAnimation =
                        azrtti_cast<const SceneData::GraphData::AnimationData*>(lhs.get());
                    const SceneData::GraphData::AnimationData* rhsAnimation =
                        azrtti_cast<const SceneData::GraphData::AnimationData*>(rhs.get());

                    return (*lhsAnimation == *rhsAnimation);
                }


                return true;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
