/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/TangentGenerator/TangentGenerateComponent.h>
#include <Generation/Components/TangentGenerator/TangentGenerators/MikkTGenerator.h>
#include <Generation/Components/TangentGenerator/TangentGenerators/BlendShapeMikkTGenerator.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>

#include <AzCore/Math/Vector4.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/make_shared.h>


namespace AZ::SceneGenerationComponents
{
    static constexpr AZStd::string_view DefaultTangentGenerationKey{ "/O3DE/SceneAPI/TangentGenerateComponent/DefaultGenerationMethod" };
    static constexpr AZStd::string_view DebugBitangentFlipKey{ "/O3DE/SceneAPI/TangentGenerateComponent/DebugBitangentFlip" };

    TangentGenerateComponent::TangentGenerateComponent()
    {
        BindToCall(&TangentGenerateComponent::GenerateTangentData);
    }


    void TangentGenerateComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TangentGenerateComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(3);
        }
    }

    const AZ::SceneAPI::SceneData::TangentsRule* TangentGenerateComponent::GetTangentRule(const AZ::SceneAPI::Containers::Scene& scene) const
    {
        for (const auto& object : scene.GetManifest().GetValueStorage())
        {
            if (object->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid()))
            {
                const AZ::SceneAPI::DataTypes::IGroup* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IGroup*>(object.get());
                const AZ::SceneAPI::SceneData::TangentsRule* rule = group->GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::TangentsRule>().get();
                if (rule)
                {
                    return rule;
                }
            }
        }

        return nullptr;
    }

    void TangentGenerateComponent::GetRegistrySettings(
        AZ::SceneAPI::DataTypes::TangentGenerationMethod& defaultGenerationMethod, bool& debugBitangentFlip) const
    {
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZStd::string defaultTangentGenerationMethodString;
            if (settingsRegistry->Get(defaultTangentGenerationMethodString, DefaultTangentGenerationKey))
            {
                const bool isCaseSensitive = false;
                if (AZ::StringFunc::Equal(defaultTangentGenerationMethodString, "MikkT", isCaseSensitive))
                {
                    defaultGenerationMethod = AZ::SceneAPI::DataTypes::TangentGenerationMethod::MikkT;
                }
                else
                {
                    AZ_Warning(
                        AZ::SceneAPI::Utilities::WarningWindow,
                        AZ::StringFunc::Equal(defaultTangentGenerationMethodString, "FromSourceScene", isCaseSensitive),
                        "'" AZ_STRING_FORMAT "' is not a valid default tangent generation method. Check the value of %s in your settings registry, and change "
                        "it to 'FromSourceScene' or 'MikkT'",
                        defaultTangentGenerationMethodString.c_str(), AZ_STRING_ARG(DefaultTangentGenerationKey));
                }
            }

            settingsRegistry->Get(debugBitangentFlip, DebugBitangentFlipKey);
        }
    }

    AZ::SceneAPI::Events::ProcessingResult TangentGenerateComponent::GenerateTangentData(TangentGenerateContext& context)
    {
        // Get any tangent related settings from the settings registry
        AZ::SceneAPI::DataTypes::TangentGenerationMethod defaultGenerationMethod =
            AZ::SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene;
        bool debugBitangentFlip = false;
        GetRegistrySettings(defaultGenerationMethod, debugBitangentFlip);

        // Get the generation setting for this scene
        const AZ::SceneAPI::SceneData::TangentsRule* tangentsRule = GetTangentRule(context.GetScene());
        const AZ::SceneAPI::DataTypes::TangentGenerationMethod generationMethod =
            tangentsRule ? tangentsRule->GetGenerationMethod() : defaultGenerationMethod;

        // Iterate over all graph content and filter out all meshes.
        AZ::SceneAPI::Containers::SceneGraph& graph = context.GetScene().GetGraph();
        AZ::SceneAPI::Containers::SceneGraph::ContentStorageData graphContent = graph.GetContentStorage();

        // Build a list of mesh data nodes.
        AZStd::vector<AZStd::pair<AZ::SceneAPI::DataTypes::IMeshData*, AZ::SceneAPI::Containers::SceneGraph::NodeIndex> > meshes;
        for (auto item = graphContent.begin(); item != graphContent.end(); ++item)
        {
            // Skip anything that isn't a mesh.
            if (!(*item) || !(*item)->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid()))
            {
                continue;
            }

            // Get the mesh data and node index and store them in the vector as a pair, so we can iterate over them later.
            auto* mesh = static_cast<AZ::SceneAPI::DataTypes::IMeshData*>(item->get());
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.ConvertToNodeIndex(item);
            meshes.emplace_back(mesh, nodeIndex);
        }

        // Iterate over them. We had to build the array before as this method can insert new nodes, so using the iterator directly would fail.
        for (auto& [mesh, nodeIndex] : meshes)
        {
            // Generate tangents for the mesh (if this is desired or needed).
            if (!GenerateTangentsForMesh(context.GetScene(), nodeIndex, mesh, generationMethod))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }

            // Now that we have the tangents and bitangents, calculate the tangent w values for the ones that we imported from the scene file, as they only have xyz.
            // But only do this if we are getting tangents from the source scene, because MikkT will provide us with a correct tangent.w already
            if (generationMethod == SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene)
            {
                if (!UpdateFbxTangentWValues(graph, nodeIndex, mesh, debugBitangentFlip))
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Failure;
                }
            }
            
        }

        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }

    bool TangentGenerateComponent::UpdateFbxTangentWValues(
        AZ::SceneAPI::Containers::SceneGraph& graph,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        const AZ::SceneAPI::DataTypes::IMeshData* meshData,
        bool debugBitangentFlip)
    {
        // Iterate over all UV sets.
        AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData = FindUvData(graph, nodeIndex, 0);
        size_t uvSetIndex = 0;
        while (uvData)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData* fbxTangentData = FindTangentData(graph, nodeIndex, uvSetIndex);
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* fbxBitangentData = FindBitangentData(graph, nodeIndex, uvSetIndex);

            if (fbxTangentData && fbxBitangentData)
            {
                const size_t numVerts = uvData->GetCount();
                AZ_Assert((numVerts == fbxTangentData->GetCount()) && (numVerts == fbxBitangentData->GetCount()), "Number of vertices inside UV set is not the same as number of tangents and bitangents.");
                for (size_t i = 0; i < numVerts; ++i)
                {
                    // This code calculates the best tangent.w value, which is either -1 or +1, depending on the bitangent being mirrored or not.
                    // We determine this by checking the angle between the generated tangent by doing a cross product between the tangent and normal, and the actual real bitangent.
                    // It is no guarantee that using "cross(normal, tangent.xyz)* tangent.w" will result in the right bitangent, as the basis might not be orthogonal.
                    // But we still go for the best guess.
                    AZ::Vector4 tangent = fbxTangentData->GetTangent(i);
                    AZ::Vector3 tangentDir = tangent.GetAsVector3();
                    tangentDir.NormalizeSafe();
                    AZ::Vector3 normal = meshData->GetNormal(static_cast<AZ::u32>(i));
                    normal.NormalizeSafe();
                    AZ::Vector3 generatedBitangent = normal.Cross(tangentDir);

                    float dot = fbxBitangentData->GetBitangent(i).Dot(generatedBitangent);
                    dot = AZ::GetMax(dot, -1.0f);
                    dot = AZ::GetMin(dot, 1.0f);
                    const float angle = acosf(dot);
                    if (angle > AZ::Constants::HalfPi)
                    {
                        tangent = fbxTangentData->GetTangent(i);
                        tangent.SetW(-1.0f);
                    }
                    else
                    {
                        tangent = fbxTangentData->GetTangent(i);
                        tangent.SetW(1.0f);
                    }
                        
                    if (debugBitangentFlip)
                    {
                        AZ::Vector4 originalTangent = fbxTangentData->GetTangent(i);

                        if (originalTangent.GetW() > 0.0f)
                        {
                            // If the tangent has a positive w value, the fix for GHI-7125 is going to flip the bitangent
                            // compared to the original behavior. Report an error and fail to process as an indication
                            // that this asset will be impacted by GHI-7125
                            
                            AZ_Error(
                                AZ::SceneAPI::Utilities::ErrorWindow, false,
                                "Tangent w is positive for at least one vertex in the mesh. This model will be impacted by GHI-7125. "
                                "See https://github.com/o3de/o3de/issues/7125 for details.");
                            return false;
                        }
                    }

                    // Update the tangent.w in the scene
                    fbxTangentData->SetTangent(i, tangent);
                }
            }

            // Find the next UV set.
            uvData = FindUvData(graph, nodeIndex, ++uvSetIndex);
        }
                                
        return true;
    }

    void TangentGenerateComponent::FindBlendShapes(
        AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        AZStd::vector<AZ::SceneData::GraphData::BlendShapeData*>& outBlendShapes) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(
            graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneData::GraphData::BlendShapeData* blendShape =
                azrtti_cast<AZ::SceneData::GraphData::BlendShapeData*>(child->second.get());
            if (blendShape)
            {
                outBlendShapes.emplace_back(blendShape);
            }
        }
    }

    bool TangentGenerateComponent::GenerateTangentsForMesh(
        AZ::SceneAPI::Containers::Scene& scene,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        AZ::SceneAPI::DataTypes::IMeshData* meshData,
        AZ::SceneAPI::DataTypes::TangentGenerationMethod ruleGenerationMethod)
    {
        AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

        // Check if we have any UV data, if not, we cannot possibly generate the tangents.
        const size_t uvSetCount = CalcUvSetCount(graph, nodeIndex);
        if (uvSetCount == 0)
        {
            AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Cannot generate tangents for this mesh, as it has no UV coordinates.\n");
            return true; // No fatal error
        }

        const AZ::SceneAPI::SceneData::TangentsRule* tangentsRule = GetTangentRule(scene);

        // Find all blend shape data under the mesh. We need to generate the tangent and bitangent for blend shape as well.
        AZStd::vector<AZ::SceneData::GraphData::BlendShapeData*> blendShapes;
        FindBlendShapes(graph, nodeIndex, blendShapes);

        // Generate tangents/bitangents for all uv sets.
        bool allSuccess = true;
        for (size_t uvSetIndex = 0; uvSetIndex < uvSetCount; ++uvSetIndex)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData = FindUvData(graph, nodeIndex, uvSetIndex);
            if (!uvData)
            {
                AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Cannot generate tangents for uv set %zu as it cannot be retrieved.\n", uvSetIndex);
                continue;
            }

            // Check if we had tangents inside the source scene file.
            AZ::SceneAPI::DataTypes::TangentGenerationMethod generationMethod = ruleGenerationMethod;
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData* tangentData = FindTangentData(graph, nodeIndex, uvSetIndex);
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* bitangentData = FindBitangentData(graph, nodeIndex, uvSetIndex);

            // If all we need is import from the source scene, and we have tangent data from the source scene already, then skip generating.
            if ((generationMethod == AZ::SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene))
            {
                if (tangentData && bitangentData)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Using source scene tangents and bitangents for uv set %zu for mesh '%s'.\n",
                        uvSetIndex, scene.GetGraph().GetNodeName(nodeIndex).GetName());
                    continue;
                }
                else
                {
                    // In case there are no tangents/bitangents while the user selected to use the source ones, default to MikkT.
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Cannot use source scene tangents as there are none in the asset for mesh '%s' for uv set %zu. Defaulting to generating tangents using MikkT.\n",
                        scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
                    generationMethod = AZ::SceneAPI::DataTypes::TangentGenerationMethod::MikkT;
                }
            }

            if (!tangentData)
            {
                if (!AZ::SceneGenerationComponents::TangentGenerateComponent::CreateTangentLayer(scene.GetManifest(), nodeIndex, meshData->GetVertexCount(), uvSetIndex,
                    generationMethod, graph, &tangentData))
                {
                    AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create tangents data set for mesh %s for uv set %zu.\n",
                        scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
                    continue;
                }
            }
            AZ_Assert(tangentData == FindTangentData(graph, nodeIndex, uvSetIndex), "Used tangent data is not the same as the graph returns.");

            if (!bitangentData)
            {
                if (!AZ::SceneGenerationComponents::TangentGenerateComponent::CreateBitangentLayer(scene.GetManifest(), nodeIndex, meshData->GetVertexCount(), uvSetIndex,
                    generationMethod, graph, &bitangentData))
                {
                    AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create bitangents data set for mesh %s for uv set %zu.\n",
                        scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
                    continue;
                }
            }
            AZ_Assert(bitangentData == FindBitangentData(graph, nodeIndex, uvSetIndex), "Used bitangent data is not the same as the graph returns.");

            tangentData->SetGenerationMethod(generationMethod);
            bitangentData->SetGenerationMethod(generationMethod);

            switch (generationMethod)
            {
            // Generate using MikkT space.
            case AZ::SceneAPI::DataTypes::TangentGenerationMethod::MikkT:
            {
                const AZ::SceneAPI::DataTypes::MikkTSpaceMethod tSpaceMethod = tangentsRule ? tangentsRule->GetMikkTSpaceMethod() : AZ::SceneAPI::DataTypes::MikkTSpaceMethod::TSpace;

                allSuccess &= AZ::TangentGeneration::Mesh::MikkT::GenerateTangents(meshData, uvData, tangentData, bitangentData, tSpaceMethod);

                for (AZ::SceneData::GraphData::BlendShapeData* blendShape : blendShapes)
                {
                    allSuccess &= AZ::TangentGeneration::BlendShape::MikkT::GenerateTangents(blendShape, uvSetIndex, tSpaceMethod);
                }
            }
            break;

            default:
            {
                AZ_Assert(false, "Unknown tangent generation method selected (%d) for UV set %d, cannot generate tangents.\n", static_cast<AZ::u32>(generationMethod), uvSetIndex);
                allSuccess = false;
            }
            }
        }

        return allSuccess;
    }

    size_t TangentGenerateComponent::CalcUvSetCount(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        size_t result = 0;
        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
            if (data)
            {
                result++;
            }
        }

        return result;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexUVData* TangentGenerateComponent::FindUvData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 uvSet) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        AZ::u64 uvSetIndex = 0;
        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
            if (data)
            {
                if (uvSetIndex == uvSet)
                {
                    return data;
                }
                uvSetIndex++;
            }
        }

        return nullptr;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexTangentData* TangentGenerateComponent::FindTangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexTangentData*>(child->second.get());
            if (data && setIndex == data->GetTangentSetIndex())
            {
                return data;
            }
        }

        return nullptr;
    }

    bool TangentGenerateComponent::CreateTangentLayer(AZ::SceneAPI::Containers::SceneManifest& manifest,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        size_t numVerts,
        size_t uvSetIndex,
        AZ::SceneAPI::DataTypes::TangentGenerationMethod generationMethod,
        AZ::SceneAPI::Containers::SceneGraph& graph,
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData** outTangentData)
    {
        *outTangentData = nullptr;

        AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();
        tangentData->Resize(numVerts);

        AZ_Assert(tangentData, "Failed to allocate tangent data for scene graph.");
        if (!tangentData)
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to allocate tangent data.\n");
            return false;
        }

        tangentData->SetTangentSetIndex(uvSetIndex);
        tangentData->SetGenerationMethod(generationMethod);

        const AZStd::string tangentGeneratedName = AZStd::string::format("TangentSet_%zu", uvSetIndex);
        const AZStd::string tangentSetName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(tangentGeneratedName, manifest);
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex newIndex = graph.AddChild(nodeIndex, tangentSetName.c_str(), tangentData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for tangent attribute.");
        if (!newIndex.IsValid())
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create node in scene graph that stores tangent data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        *outTangentData = tangentData.get();
        return true;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* TangentGenerateComponent::FindBitangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexBitangentData*>(child->second.get());
            if (data && setIndex == data->GetBitangentSetIndex())
            {
                return data;
            }
        }

        return nullptr;
    }

    bool TangentGenerateComponent::CreateBitangentLayer(AZ::SceneAPI::Containers::SceneManifest& manifest,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        size_t numVerts,
        size_t uvSetIndex,
        AZ::SceneAPI::DataTypes::TangentGenerationMethod generationMethod,
        AZ::SceneAPI::Containers::SceneGraph& graph,
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData** outBitangentData)
    {
        *outBitangentData = nullptr;

        AZStd::shared_ptr<AZ::SceneData::GraphData::MeshVertexBitangentData> bitangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexBitangentData>();
        bitangentData->Resize(numVerts);

        AZ_Assert(bitangentData, "Failed to allocate bitangent data for scene graph.");
        if (!bitangentData)
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to allocate bitangent data.\n");
            return false;
        }

        bitangentData->SetBitangentSetIndex(uvSetIndex);
        bitangentData->SetGenerationMethod(generationMethod);

        const AZStd::string bitangentGeneratedName = AZStd::string::format("BitangentSet_%zu", uvSetIndex);
        const AZStd::string bitangentSetName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(bitangentGeneratedName, manifest);
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex newIndex = graph.AddChild(nodeIndex, bitangentSetName.c_str(), bitangentData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for bitangent attribute.");
        if (!newIndex.IsValid())
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create node in scene graph that stores bitangent data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        *outBitangentData = bitangentData.get();
        return true;
    }
} // namespace AZ::SceneGenerationComponents
