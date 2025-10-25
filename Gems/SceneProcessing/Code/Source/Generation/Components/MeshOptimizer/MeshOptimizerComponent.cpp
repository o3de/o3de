/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/base.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/reference_wrapper.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/sort.h>
#include <AzCore/Math/Color.h>

#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ISkinRule.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/CustomPropertyData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>
#include <meshoptimizer.h>

namespace AZ { class ReflectContext; }

namespace AZ::SceneGenerationComponents
{
    using AZ::SceneAPI::Containers::SceneGraph;
    using AZ::SceneAPI::DataTypes::IBlendShapeData;
    using AZ::SceneAPI::DataTypes::ILodRule;
    using AZ::SceneAPI::DataTypes::IMeshData;
    using AZ::SceneAPI::DataTypes::IMeshGroup;
    using AZ::SceneAPI::DataTypes::IMeshVertexBitangentData;
    using AZ::SceneAPI::DataTypes::IMeshVertexTangentData;
    using AZ::SceneAPI::DataTypes::IMeshVertexUVData;
    using AZ::SceneAPI::DataTypes::IMeshVertexColorData;
    using AZ::SceneAPI::DataTypes::ISkinWeightData;
    using AZ::SceneAPI::DataTypes::ICustomPropertyData;
    using AZ::SceneAPI::Events::ProcessingResult;
    using AZ::SceneAPI::Events::GenerateSimplificationEventContext;
    using AZ::SceneAPI::SceneCore::GenerationComponent;
    using AZ::SceneData::GraphData::BlendShapeData;
    using AZ::SceneData::GraphData::MeshData;
    using AZ::SceneData::GraphData::MeshVertexBitangentData;
    using AZ::SceneData::GraphData::MeshVertexColorData;
    using AZ::SceneData::GraphData::MeshVertexTangentData;
    using AZ::SceneData::GraphData::MeshVertexUVData;
    using AZ::SceneData::GraphData::SkinWeightData;
    using NodeIndex = AZ::SceneAPI::Containers::SceneGraph::NodeIndex;
    namespace Containers = AZ::SceneAPI::Containers;
    namespace Views = Containers::Views;

    static void OptimizeSkinningInfluences(AZStd::vector<ISkinWeightData::Link>& influences, float tolerance, size_t maxWeights)
    {
        const auto influenceLess = [](const ISkinWeightData::Link& left, const ISkinWeightData::Link& right)
        {
            return left.weight < right.weight;
        };

        // Move all the items greater than the tolerance to the end of the array
        auto removePoint = AZStd::remove_if(
            begin(influences),
            end(influences),
            [tolerance](const ISkinWeightData::Link& influence)
            {
                return influence.weight < tolerance;
            });
        if (removePoint == begin(influences))
        {
            // If this would remove all influences, keep the biggest one
            auto [_, maxElement] = AZStd::minmax_element(begin(influences), end(influences), influenceLess);
            AZStd::swap(removePoint, maxElement);
            ++removePoint;
        }
        // remove all weights below the tolerance
        influences.erase(removePoint, end(influences));

        // reduce number of weights when needed
        while (influences.size() > maxWeights)
        {
            // remove this smallest weight
            const auto [minInfluence, _] = AZStd::minmax_element(begin(influences), end(influences), influenceLess);
            influences.erase(minInfluence);
        }

        // calculate the total weight
        const float totalWeight = AZStd::accumulate(
            begin(influences),
            end(influences),
            0.0f,
            [](float total, const ISkinWeightData::Link& influence)
            {
                return total + influence.weight;
            });

        // normalize
        for (ISkinWeightData::Link& influence : influences)
        {
            influence.weight /= totalWeight;
        }
    }

    // sort influences on weights, from big to small
    static void SortInfluencesByWeight(AZStd::vector<ISkinWeightData::Link>& influences)
    {
        AZStd::sort(
            begin(influences),
            end(influences),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.weight > rhs.weight;
            });
    }

    MeshOptimizerComponent::MeshOptimizerComponent()
    {
        BindToCall(&MeshOptimizerComponent::OptimizeMeshes);
    }

    void MeshOptimizerComponent::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MeshOptimizerComponent, GenerationComponent>()->Version(12); // Fix vertex welding
        }
    }

    // Recurse through the SceneAPI's iterator types, extracting the real underlying iterator.
    struct ConvertToHierarchyIterator
    {
        template<typename T, typename U>
        static auto Unwrap(const Containers::Views::ConvertIterator<T, U>& it)
        {
            return Unwrap(it.GetBaseIterator());
        }

        template<typename T, typename U>
        static auto Unwrap(const Containers::Views::FilterIterator<T, U>& it)
        {
            return Unwrap(it.GetBaseIterator());
        }

        template<typename T>
        static auto Unwrap(const Containers::Views::SceneGraphChildIterator<T>& it)
        {
            return it.GetHierarchyIterator();
        }
    };

    bool MeshOptimizerComponent::HasAnyBlendShapeChild(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex)
    {
        return !Containers::MakeDerivedFilterView<IBlendShapeData>(
            Views::MakeSceneGraphChildView(graph, nodeIndex, graph.GetContentStorage().cbegin(), true)
        ).empty();
    }

    static ICustomPropertyData::PropertyMap& FindOrCreateCustomPropertyData(
        Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& nodeIndex)
    {
        NodeIndex customPropertyIndex =
            SceneAPI::Utilities::GetImmediateChildOfType(graph, nodeIndex, azrtti_typeid<ICustomPropertyData>());

        if (!customPropertyIndex.IsValid())
        {
            // If no custom property data node exists, insert one
            AZStd::shared_ptr<SceneData::GraphData::CustomPropertyData> createdCustumPropertyData =
                AZStd::make_shared<SceneData::GraphData::CustomPropertyData>();
            customPropertyIndex = graph.AddChild(nodeIndex, "custom_properties", AZStd::move(createdCustumPropertyData));
        }

        ICustomPropertyData* customPropertyDataNode =
            azrtti_cast<ICustomPropertyData*>(graph.GetNodeContent(customPropertyIndex).get());

        return customPropertyDataNode->GetPropertyMap();
    }

    static bool HasOptimizedMeshNode(ICustomPropertyData::PropertyMap& propertyMap)
    {
        // Now look up the optimized index
        auto iter = propertyMap.find(SceneAPI::Utilities::OptimizedMeshPropertyMapKey);
        if (iter != propertyMap.end())
        {
            const auto& [key, optimizedAnyIndex] = *iter;
            if (!optimizedAnyIndex.empty() && optimizedAnyIndex.is<NodeIndex>())
            {
                return true;
            }
        }

        return false;
    }

    static void* MeshoptimizerAllocate(size_t size)
    {
        return azmalloc(size);
    }

    static void MeshoptimizerFree(void* ptr)
    {
        azfree(ptr);
    }

    ProcessingResult MeshOptimizerComponent::OptimizeMeshes(GenerateSimplificationEventContext& context) const
    {
        // Set up meshoptimizer allocation routine
        meshopt_setAllocator(MeshoptimizerAllocate, MeshoptimizerFree);

        // Iterate over all graph content and filter out all meshes.
        SceneGraph& graph = context.GetScene().GetGraph();

        // Build a list of mesh data nodes.
        AZStd::vector<AZStd::pair<const IMeshData*, NodeIndex>> meshes;
        const auto meshNodes = Containers::MakeDerivedFilterView<IMeshData>(graph.GetContentStorage());
        for (auto it = meshNodes.cbegin(); it != meshNodes.cend(); ++it)
        {
            // Get the mesh data and node index and store them in the vector as a pair, so we can iterate over them later.
            // The sequential calls to GetBaseIterator unwrap the layers of FilterIterators from the MakeDerivedFilterView
            meshes.emplace_back(&(*it), graph.ConvertToNodeIndex(it.GetBaseIterator().GetBaseIterator()));
        }

        const auto meshGroups = Containers::MakeDerivedFilterView<IMeshGroup>(context.GetScene().GetManifest().GetValueStorage());

        AZStd::unordered_map<const IMeshGroup*, AZStd::vector<AZStd::string_view>> selectedNodes;

        const auto addSelectionListToMap = [&selectedNodes](const IMeshGroup& meshGroup, const SceneAPI::DataTypes::ISceneNodeSelectionList& selectionList)
        {
            selectionList.EnumerateSelectedNodes(
                [&selectedNodes, &meshGroup](const AZStd::string& name)
                {
                    selectedNodes[&meshGroup].emplace_back(name);
                    return true;
                });
        };

        for (const IMeshGroup& meshGroup : meshGroups)
        {
            addSelectionListToMap(meshGroup, meshGroup.GetSceneNodeSelectionList());
            const ILodRule* lodRule = meshGroup.GetRuleContainerConst().FindFirstByType<SceneAPI::DataTypes::ILodRule>().get();
            if (lodRule)
            {
                for (size_t lod = 0; lod < lodRule->GetLodCount(); ++lod)
                {
                    addSelectionListToMap(meshGroup, lodRule->GetSceneNodeSelectionList(lod));
                }
            }
        }

        const auto childNodes = [&graph](NodeIndex nodeIndex) { return Views::MakeSceneGraphChildView(graph, nodeIndex, graph.GetContentStorage().cbegin(), true); };
        const auto nodeIndexes = [&graph](const auto& view)
        {
            AZStd::vector<NodeIndex> indexes;
            indexes.reserve(AZStd::distance(view.begin(), view.end()));
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                indexes.emplace_back(graph.ConvertToNodeIndex(ConvertToHierarchyIterator::Unwrap(it)));
            }
            return indexes;
        };

        // Iterate over them. We had to build the array before as this method can insert new nodes, so using the iterator directly would fail.
        for (const auto& [mesh, nodeIndex] : meshes)
        {
            // A Mesh can have multiple child nodes that contain other data streams, like uvs and tangents

            const auto uvDatasView = Containers::MakeDerivedFilterView<IMeshVertexUVData>(childNodes(nodeIndex));
            const auto tangentDatasView = Containers::MakeDerivedFilterView<IMeshVertexTangentData>(childNodes(nodeIndex));
            const auto bitangentDatasView = Containers::MakeDerivedFilterView<IMeshVertexBitangentData>(childNodes(nodeIndex));
            const auto skinWeightDatasView = Containers::MakeDerivedFilterView<ISkinWeightData>(childNodes(nodeIndex));
            const auto colorDatasView = Containers::MakeDerivedFilterView<IMeshVertexColorData>(childNodes(nodeIndex));

            const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexUVData>> uvDatas(uvDatasView.begin(), uvDatasView.end());
            const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexTangentData>> tangentDatas(tangentDatasView.begin(), tangentDatasView.end());
            const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexBitangentData>> bitangentDatas(bitangentDatasView.begin(), bitangentDatasView.end());
            const AZStd::vector<AZStd::reference_wrapper<const ISkinWeightData>> skinWeightDatas(skinWeightDatasView.begin(), skinWeightDatasView.end());
            const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexColorData>> colorDatas(colorDatasView.begin(), colorDatasView.end());

            const AZStd::string_view nodePath(graph.GetNodeName(nodeIndex).GetPath(), graph.GetNodeName(nodeIndex).GetPathLength());

            for (const IMeshGroup& meshGroup : meshGroups)
            {
                if (!selectedNodes.contains(&meshGroup))
                {
                    AZ_Warning(
                        AZ::SceneAPI::Utilities::LogWindow,
                        false,
                        "MeshGroup %s wasn't found in the list of selected nodes.",
                        meshGroup.GetName().c_str());
                    continue;
                }

                // Skip meshes that are not used by this mesh group
                if (AZStd::find(selectedNodes.at(&meshGroup).cbegin(), selectedNodes.at(&meshGroup).cend(), nodePath) == selectedNodes.at(&meshGroup).cend())
                {
                    continue;
                }

                ICustomPropertyData::PropertyMap& unoptimizedPropertyMap = FindOrCreateCustomPropertyData(graph, nodeIndex);
                if (HasOptimizedMeshNode(unoptimizedPropertyMap))
                {
                    // There is already an optimized mesh node for this mesh, so skip it.
                    // There must be another mesh group already referencing this mesh node.
                    continue;
                }

                const bool hasBlendShapes = HasAnyBlendShapeChild(graph, nodeIndex);

                auto [optimizedMesh, optimizedUVs, optimizedTangents, optimizedBitangents, optimizedVertexColors, optimizedSkinWeights] = OptimizeMesh(mesh, uvDatas, tangentDatas, bitangentDatas, colorDatas, skinWeightDatas, meshGroup);

                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Optimized mesh '%s': Original: %zu vertices -> optimized: %zu vertices, %0.02f%% of the original (hasBlendShapes=%s)",
                    graph.GetNodeName(nodeIndex).GetName(),
                    mesh->GetVertexCount(),
                    optimizedMesh->GetVertexCount(),
                    ((float)optimizedMesh->GetVertexCount() / (float)mesh->GetVertexCount()) * 100.0f,
                    hasBlendShapes ? "Yes" : "No"
                );

                // Insert a new node for the optimized mesh
                const AZStd::string name =
                    SceneAPI::Utilities::SceneGraphSelector::GenerateOptimizedMeshNodeName(graph, nodeIndex, meshGroup);
                const NodeIndex optimizedMeshNodeIndex =
                    graph.AddChild(graph.GetNodeParent(nodeIndex), name.c_str(), AZStd::move(optimizedMesh));

                if (!optimizedMeshNodeIndex.IsValid())
                {
                    // An invalid node index usually happens when the name is invalid.
                    // An error will already be printed so no need for one here.
                    return ProcessingResult::Failure;
                }

                // Copy any custom properties from the original mesh to the optimized mesh
                ICustomPropertyData::PropertyMap& optimizedPropertyMap = FindOrCreateCustomPropertyData(graph, optimizedMeshNodeIndex);
                optimizedPropertyMap = unoptimizedPropertyMap;

                // Add a mapping from the optimized node back to the original node so it can also be looked up later
                optimizedPropertyMap[SceneAPI::Utilities::OriginalUnoptimizedMeshPropertyMapKey] =
                    AZStd::make_any<NodeIndex>(nodeIndex);

                // Add the optimized node index to the original mesh's custom property map so it can be looked up later
                unoptimizedPropertyMap[SceneAPI::Utilities::OptimizedMeshPropertyMapKey] =
                    AZStd::make_any<NodeIndex>(optimizedMeshNodeIndex);

                auto addOptimizedNodes = [&graph, &optimizedMeshNodeIndex](const auto& originalNodeIndexes, auto& optimizedNodes)
                {
                    AZ_PUSH_DISABLE_WARNING(, "-Wrange-loop-analysis") // remove when we upgrade from clang 6.0
                    for (const auto& [originalNodeIndex, optimizedNode] : Containers::Views::MakePairView(originalNodeIndexes, optimizedNodes))
                    AZ_POP_DISABLE_WARNING
                    {
                        const AZStd::string optimizedName {graph.GetNodeName(originalNodeIndex).GetName(), graph.GetNodeName(originalNodeIndex).GetNameLength()};
                        const NodeIndex optimizedNodeIndex = graph.AddChild(optimizedMeshNodeIndex, optimizedName.c_str(), AZStd::move(optimizedNode));
                        if (graph.IsNodeEndPoint(originalNodeIndex))
                        {
                            graph.MakeEndPoint(optimizedNodeIndex);
                        }
                    }
                };
                addOptimizedNodes(nodeIndexes(Containers::MakeDerivedFilterView<IMeshVertexUVData>(childNodes(nodeIndex))), optimizedUVs);
                addOptimizedNodes(nodeIndexes(Containers::MakeDerivedFilterView<IMeshVertexTangentData>(childNodes(nodeIndex))), optimizedTangents);
                addOptimizedNodes(nodeIndexes(Containers::MakeDerivedFilterView<IMeshVertexBitangentData>(childNodes(nodeIndex))), optimizedBitangents);
                addOptimizedNodes(nodeIndexes(Containers::MakeDerivedFilterView<IMeshVertexColorData>(childNodes(nodeIndex))), optimizedVertexColors);

                if (optimizedSkinWeights)
                {
                    const NodeIndex optimizedSkinNodeIndex = graph.AddChild(optimizedMeshNodeIndex, "skinWeights", AZStd::move(optimizedSkinWeights));
                    graph.MakeEndPoint(optimizedSkinNodeIndex);
                }

                for (const NodeIndex& blendShapeNodeIndex : nodeIndexes(Containers::MakeDerivedFilterView<IBlendShapeData>(childNodes(nodeIndex))))
                {
                    const IBlendShapeData* blendShapeNode = static_cast<IBlendShapeData*>(graph.GetNodeContent(blendShapeNodeIndex).get());
                    auto [optimizedBlendShape, _1, _2, _3 , _4, _5] = OptimizeMesh(blendShapeNode, {}, {}, {}, {}, {}, meshGroup);

                    const AZStd::string optimizedName {graph.GetNodeName(blendShapeNodeIndex).GetName(), graph.GetNodeName(blendShapeNodeIndex).GetNameLength()};
                    const NodeIndex optimizedNodeIndex = graph.AddChild(optimizedMeshNodeIndex, optimizedName.c_str(), AZStd::move(optimizedBlendShape));
                    if (graph.IsNodeEndPoint(blendShapeNodeIndex))
                    {
                        graph.MakeEndPoint(optimizedNodeIndex);
                    }
                }

                const AZStd::array skippedChildTypes {
                    // Skip copying the optimized nodes since we've already
                    // populated those nodes with the optimized data
                    azrtti_typeid<IMeshData>(),
                    azrtti_typeid<IMeshVertexUVData>(),
                    azrtti_typeid<IMeshVertexTangentData>(),
                    azrtti_typeid<IMeshVertexBitangentData>(),
                    azrtti_typeid<IMeshVertexColorData>(),
                    azrtti_typeid<ISkinWeightData>(),
                    azrtti_typeid<IBlendShapeData>(),
                    // Skip copying the custom property data because we've already copied it above
                    azrtti_typeid<ICustomPropertyData>()
                };

                // Copy the children of the original mesh node, but skip any nodes we have already populated
                for (const NodeIndex& childNodeIndex : nodeIndexes(childNodes(nodeIndex)))
                {
                    const AZStd::shared_ptr<SceneAPI::DataTypes::IGraphObject>& childNode = graph.GetNodeContent(childNodeIndex);

                    if (!AZStd::any_of(
                            skippedChildTypes.begin(),
                            skippedChildTypes.end(),
                            [&childNode](const AZ::Uuid& typeId)
                            {
                                return AZ::RttiIsTypeOf(typeId, childNode.get());
                            }))
                    {
                        const AZStd::string optimizedName {graph.GetNodeName(childNodeIndex).GetName(), graph.GetNodeName(childNodeIndex).GetNameLength()};
                        const NodeIndex optimizedNodeIndex = graph.AddChild(optimizedMeshNodeIndex, optimizedName.c_str(), childNode);
                        if (graph.IsNodeEndPoint(childNodeIndex))
                        {
                            graph.MakeEndPoint(optimizedNodeIndex);
                        }
                    }
                }
            }
        }

        return ProcessingResult::Success;
    }

    template<class DataNodeType, class MeshBuilderLayerType>
    AZStd::vector<AZStd::unique_ptr<DataNodeType>> makeSceneGraphNodesForMeshBuilderLayers(const MeshBuilderLayerType& meshBuilderLayers)
    {
        AZStd::vector<AZStd::unique_ptr<DataNodeType>> layers(meshBuilderLayers.size());
        AZStd::generate(layers.begin(), layers.end(), []
        {
            return AZStd::make_unique<DataNodeType>();
        });
        return layers;
    };

    static void GetIndices(const IBlendShapeData* blendShape, AZStd::vector<AZ::u32>& indices, [[maybe_unused]] AZStd::vector<AZ::u32>& materialIds)
    {
        // Add the index data
        for (AZ::u32 faceIndex = 0; faceIndex < blendShape->GetFaceCount(); ++faceIndex)
        {
            for (AZ::u32 faceVertex = 0; faceVertex < 3; ++faceVertex)
            {
                indices[faceIndex * 3 + faceVertex] = blendShape->GetFaceVertexIndex(faceIndex, faceVertex);
            }
        }
    }

    static void GetIndices(const IMeshData* mesh, AZStd::vector<AZ::u32>& indices, AZStd::vector<AZ::u32>& materialIds)
    {
        // Add the index data
        for (AZ::u32 faceIndex = 0; faceIndex < mesh->GetFaceCount(); ++faceIndex)
        {
            for (AZ::u32 faceVertex = 0; faceVertex < 3; ++faceVertex)
            {
                indices[faceIndex * 3 + faceVertex] = mesh->GetFaceVertexIndex(faceIndex, faceVertex);
                materialIds[faceIndex * 3 + faceVertex] = mesh->GetFaceMaterialId(faceIndex);

            }
        }
    }


    template<class MeshDataType>
    AZStd::tuple<
        AZStd::unique_ptr<MeshDataType>,
        AZStd::vector<AZStd::unique_ptr<MeshVertexUVData>>,
        AZStd::vector<AZStd::unique_ptr<MeshVertexTangentData>>,
        AZStd::vector<AZStd::unique_ptr<MeshVertexBitangentData>>,
        AZStd::vector<AZStd::unique_ptr<MeshVertexColorData>>,
        AZStd::unique_ptr<AZ::SceneAPI::DataTypes::ISkinWeightData>
    > MeshOptimizerComponent::OptimizeMesh(
        const MeshDataType* meshData,
        const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexUVData>>& uvs,
        const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexTangentData>>& tangents,
        const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexBitangentData>>& bitangents,
        const AZStd::vector<AZStd::reference_wrapper<const IMeshVertexColorData>>& vertexColors,
        const AZStd::vector<AZStd::reference_wrapper<const ISkinWeightData>>& skinWeights,
        const AZ::SceneAPI::DataTypes::IMeshGroup& meshGroup)
    {
        const size_t vertexCount = meshData->GetVertexCount();
        const size_t faceCount = meshData->GetFaceCount();
        const size_t indexCount = meshData->GetVertexIndexCount();

        AZ_Error(AZ::SceneAPI::Utilities::LogWindow,
            indexCount == faceCount * 3,
            "Face count doesn't match index Count!");

        AZStd::vector<AZ::u32> indices(indexCount);
        AZStd::vector<AZ::u32> materialIds(indexCount);
        GetIndices(meshData, indices, materialIds);

        const auto* skinRule = meshGroup.GetRuleContainerConst().FindFirstByType<SceneAPI::DataTypes::ISkinRule>().get();
        const AZ::u32 maxWeightsPerVertex = skinRule ? skinRule->GetMaxWeightsPerVertex() : 4;
        const float weightThreshold = skinRule ? skinRule->GetWeightThreshold() : 0.001f;

        AZStd::vector<AZ::u32> vertexRemap(vertexCount);
        const size_t optimizedVertexCount = meshopt_generateVertexRemap(
            vertexRemap.data(),
            indices.data(),
            indexCount,
            reinterpret_cast<const float*>(meshData->GetPositions().data()),
            vertexCount,
            sizeof(AZ::Vector3));

        // Create the resulting nodes
        struct ResultingType
        {
            // When this method is called with an IMeshData node, it is generating a MeshData node. When called on an
            // IBlendShapeData node, it is generating a BlendShapeData node.
            static constexpr auto type(const IMeshData*) -> MeshData;
            static constexpr auto type(const IBlendShapeData*) -> BlendShapeData;
        };

        auto optimizedMesh = AZStd::make_unique<decltype(ResultingType::type(meshData))>();
        optimizedMesh->CloneAttributesFrom(meshData);
        optimizedMesh->GetPositions().resize_no_construct(optimizedVertexCount);
        optimizedMesh->GetNormals().resize_no_construct(optimizedVertexCount);
        AZStd::vector<AZ::u32> optimizedMaterialIds(indexCount);
        AZStd::vector<AZ::u32> optimizedIndices(indexCount);

        // First remap
        meshopt_remapVertexBuffer(optimizedMesh->GetPositions().data(), meshData->GetPositions().data(),
            vertexCount, sizeof(AZ::Vector3), vertexRemap.data());
        meshopt_remapVertexBuffer(optimizedMesh->GetNormals().data(), meshData->GetNormals().data(),
            vertexCount, sizeof(AZ::Vector3), vertexRemap.data());
        meshopt_remapIndexBuffer(optimizedIndices.data(), indices.data(), indexCount, vertexRemap.data());
        meshopt_remapIndexBuffer(optimizedMaterialIds.data(), materialIds.data(), indexCount, vertexRemap.data());

        // additional optimizations that rearranges index order, needs to recalculate material Ids.
        AZStd::unordered_map<IMeshData::Face, AZ::u32> faceMaterialIdMap;
        for (size_t faceIndex = 0; faceIndex < faceCount; faceIndex++)
        {
            IMeshData::Face faceKey = {{
                optimizedIndices[faceIndex*3],
                optimizedIndices[faceIndex*3+1],
                optimizedIndices[faceIndex*3+2]
            }};
            faceMaterialIdMap[faceKey] = optimizedMaterialIds[faceIndex*3];
        }

        meshopt_optimizeVertexCache(optimizedIndices.data(), optimizedIndices.data(), indexCount, optimizedVertexCount);
        meshopt_optimizeOverdraw(optimizedIndices.data(), optimizedIndices.data(), indexCount,
            reinterpret_cast<const float*>(optimizedMesh->GetPositions().data()), optimizedVertexCount, sizeof(AZ::Vector3), 1.05f);

        for (size_t faceIndex = 0; faceIndex < faceCount; faceIndex++)
        {
            IMeshData::Face faceKey = {{
                optimizedIndices[faceIndex*3],
                optimizedIndices[faceIndex*3+1],
                optimizedIndices[faceIndex*3+2]
            }};

            AZ_Assert(faceMaterialIdMap.contains(faceKey), "Cannot find material Id for face %zu!", faceIndex);
            optimizedMaterialIds[faceIndex*3] =
            optimizedMaterialIds[faceIndex*3+1] =
            optimizedMaterialIds[faceIndex*3+2] = faceMaterialIdMap[faceKey];
        }

        // Vertex fetch cache optimization
        AZStd::vector<AZ::u32> fetchCacheRemap(optimizedVertexCount);
        size_t fetchCacheOptimizedVertexCount =
            meshopt_optimizeVertexFetchRemap(fetchCacheRemap.data(), optimizedIndices.data(), indexCount, optimizedVertexCount);
        meshopt_remapVertexBuffer(optimizedMesh->GetPositions().data(), optimizedMesh->GetPositions().data(),
            optimizedVertexCount, sizeof(AZ::Vector3), fetchCacheRemap.data());
        meshopt_remapVertexBuffer(optimizedMesh->GetNormals().data(), optimizedMesh->GetNormals().data(),
            optimizedVertexCount, sizeof(AZ::Vector3), fetchCacheRemap.data());
        meshopt_remapIndexBuffer(indices.data(), optimizedIndices.data(), indexCount, fetchCacheRemap.data());
        meshopt_remapIndexBuffer(materialIds.data(), optimizedMaterialIds.data(), indexCount, fetchCacheRemap.data());
        optimizedMesh->GetPositions().resize_no_construct(fetchCacheOptimizedVertexCount);
        optimizedMesh->GetNormals().resize_no_construct(fetchCacheOptimizedVertexCount);
        optimizedIndices = AZStd::move(indices);
        optimizedMaterialIds = AZStd::move(materialIds);

        AZStd::vector<AZStd::unique_ptr<MeshVertexUVData>> optimizedUVs = makeSceneGraphNodesForMeshBuilderLayers<MeshVertexUVData>(uvs);
        AZStd::vector<AZStd::unique_ptr<MeshVertexTangentData>> optimizedTangents = makeSceneGraphNodesForMeshBuilderLayers<MeshVertexTangentData>(tangents);
        AZStd::vector<AZStd::unique_ptr<MeshVertexBitangentData>> optimizedBitangents = makeSceneGraphNodesForMeshBuilderLayers<MeshVertexBitangentData>(bitangents);
        AZStd::vector<AZStd::unique_ptr<MeshVertexColorData>> optimizedVertexColors = makeSceneGraphNodesForMeshBuilderLayers<MeshVertexColorData>(vertexColors);
        AZStd::unique_ptr<SkinWeightData> optimizedSkinWeights = nullptr;

        if (!skinWeights.empty())
        {
            optimizedSkinWeights = AZStd::make_unique<SkinWeightData>();
            optimizedSkinWeights->ResizeContainerSpace(fetchCacheOptimizedVertexCount);
        }

        // Copy node attributes
        AZStd::apply([]([[maybe_unused]] const auto&&... nodePairView) {
            ((AZStd::for_each(begin(nodePairView), end(nodePairView), [](const auto& nodePair) {
                auto& originalNode = nodePair.first;
                auto& optimizedNode = nodePair.second;
                optimizedNode->CloneAttributesFrom(&originalNode.get());
            })), ...);
        }, std::tuple {
            Views::MakePairView(uvs, optimizedUVs),
            Views::MakePairView(tangents, optimizedTangents),
            Views::MakePairView(bitangents, optimizedBitangents),
            Views::MakePairView(vertexColors, optimizedVertexColors),
        });

        for (size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            AZ::u32 index0 = optimizedIndices[faceIndex*3];
            AZ::u32 index1 = optimizedIndices[faceIndex*3+1];
            AZ::u32 index2 = optimizedIndices[faceIndex*3+2];

            AZ::u32 optimizedMaterialId = optimizedMaterialIds[faceIndex*3];
            AZ_Assert((optimizedMaterialId == optimizedMaterialIds[faceIndex*3+1] &&
                       optimizedMaterialId == optimizedMaterialIds[faceIndex*3+2]),
                "Triangle material Ids doesn't match!");

            AddFace(optimizedMesh.get(), index0, index1, index2, optimizedMaterialId);
        }

        // C++ 20 only
        auto twoLayerRemap = [vertexCount, optimizedVertexCount, fetchCacheOptimizedVertexCount, &vertexRemap, &fetchCacheRemap]
            <typename T>(AZStd::vector<T>& target, const AZStd::vector<T>& source)
        {
            target.resize_no_construct(optimizedVertexCount);
            meshopt_remapVertexBuffer(target.data(), source.data(), vertexCount, sizeof(T), vertexRemap.data());
            meshopt_remapVertexBuffer(target.data(), target.data(), optimizedVertexCount, sizeof(T), fetchCacheRemap.data());
            target.resize_no_construct(fetchCacheOptimizedVertexCount);
        };

        for (auto [uvNode, optimizedUVNode] : Containers::Views::MakePairView(uvs, optimizedUVs))
        {
            twoLayerRemap(optimizedUVNode->GetUVs(), uvNode.get().GetUVs());
        }
        for (auto [tangentNode, optimizedTangentNode] : Containers::Views::MakePairView(tangents, optimizedTangents))
        {
            twoLayerRemap(optimizedTangentNode->GetTangents(), tangentNode.get().GetTangents());
        }
        for (auto [bitangentNode, optimizedBitangentNode] : Containers::Views::MakePairView(bitangents, optimizedBitangents))
        {
            twoLayerRemap(optimizedBitangentNode->GetBitangents(), bitangentNode.get().GetBitangents());
        }
        for (auto [vertexColorNode, optimizedVertexColorNode] : Containers::Views::MakePairView(vertexColors, optimizedVertexColors))
        {
            twoLayerRemap(optimizedVertexColorNode->GetColors(), vertexColorNode.get().GetColors());
        }

        if (optimizedSkinWeights)
        {
            AZStd::vector<size_t> skinWeightIndices(vertexCount);
            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                skinWeightIndices[vertexIndex]  = vertexIndex; //std::itoa
            }
            meshopt_remapVertexBuffer(skinWeightIndices.data(), skinWeightIndices.data(), vertexCount, sizeof(size_t), vertexRemap.data());
            meshopt_remapVertexBuffer(skinWeightIndices.data(), skinWeightIndices.data(), optimizedVertexCount, sizeof(size_t), fetchCacheRemap.data());
            skinWeightIndices.resize_no_construct(fetchCacheOptimizedVertexCount);

#if defined(AZ_ENABLE_TRACING)
            bool influencesFoundForThisVertex = false;
#endif
            for (size_t vertexIndex = 0; vertexIndex < fetchCacheOptimizedVertexCount; vertexIndex++)
            {
                const size_t originalVertexIndex = skinWeightIndices[vertexIndex];
                // Set any real weights, if they exist
                for (const auto& skinWeightData : skinWeights)
                {
                    const size_t linkCount = skinWeightData.get().GetLinkCount(originalVertexIndex);
                    AZ_Assert(
                        linkCount <= maxWeightsPerVertex,
                        "MeshOptimizer - The configured maximum influence count is less than the current link count.");

                    // Check that either the current skinWeightData doesn't have any influences for this vertex,
                    // or that none of the ones which came before it had any influences for this vertex.
                    AZ_Assert(
                        linkCount == 0 || influencesFoundForThisVertex == false,
                        "Two different skinWeightData instances in skinWeights apply to the same vertex. "
                        "The mesh optimizer assumes there will only ever be one skinWeightData that impacts a given vertex.");
#if defined(AZ_ENABLE_TRACING)
                    // Mark that at least one influence has been found for this vertex
                    influencesFoundForThisVertex |= linkCount > 0;
#endif

                    for (size_t linkIndex = 0; linkIndex < linkCount; ++linkIndex)
                    {
                        ISkinWeightData::Link link = skinWeightData.get().GetLink(originalVertexIndex, linkIndex);
                        link.boneId = optimizedSkinWeights->GetBoneId(skinWeights[0].get().GetBoneName(link.boneId));
                        optimizedSkinWeights->AppendLink(vertexIndex, link);
                    }
                }
                if (optimizedSkinWeights->GetLinks(vertexIndex).size() > 0)
                {
                    // optimize the weights and sort them from big to small weight
                    OptimizeSkinningInfluences(optimizedSkinWeights->GetLinks(vertexIndex), weightThreshold, maxWeightsPerVertex);
                    SortInfluencesByWeight(optimizedSkinWeights->GetLinks(vertexIndex));
                }
            }
        }

        return AZStd::make_tuple(
            AZStd::move(optimizedMesh),
            AZStd::move(optimizedUVs),
            AZStd::move(optimizedTangents),
            AZStd::move(optimizedBitangents),
            AZStd::move(optimizedVertexColors),
            AZStd::move(optimizedSkinWeights)
        );
    }

    void MeshOptimizerComponent::AddFace(AZ::SceneData::GraphData::BlendShapeData* blendShape, unsigned int index1, unsigned int index2, unsigned int index3, [[maybe_unused]] unsigned int faceMaterialId)
    {
        blendShape->AddFace({index1, index2, index3});
    }
    void MeshOptimizerComponent::AddFace(AZ::SceneData::GraphData::MeshData* mesh, unsigned int index1, unsigned int index2, unsigned int index3, unsigned int faceMaterialId)
    {
        mesh->AddFace({index1, index2, index3}, faceMaterialId);
    }
} // namespace AZ::SceneGenerationComponents
