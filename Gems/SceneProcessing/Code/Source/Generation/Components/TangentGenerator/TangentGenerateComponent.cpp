/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#include <SceneAPI/SceneData/Rules/TangentsRule.h>

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

#include <AzToolsFramework/Debug/TraceContext.h>

#include <AzCore/Math/Vector4.h>
#include <AzCore/std/smart_ptr/make_shared.h>


namespace AZ::SceneGenerationComponents
{
    TangentGenerateComponent::TangentGenerateComponent()
    {
        BindToCall(&TangentGenerateComponent::GenerateTangentData);
    }


    void TangentGenerateComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TangentGenerateComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(1);
        }
    }


    AZStd::vector<AZ::SceneAPI::DataTypes::TangentSpace> TangentGenerateComponent::CollectRequiredTangentSpaces(const AZ::SceneAPI::Containers::Scene& scene) const
    {
        AZStd::vector<AZ::SceneAPI::DataTypes::TangentSpace> result;

        for (const auto& object : scene.GetManifest().GetValueStorage())
        {
            if (object->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid()))
            {
                const AZ::SceneAPI::DataTypes::IGroup* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IGroup*>(object.get());
                const AZ::SceneAPI::SceneData::TangentsRule* rule = group->GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::TangentsRule>().get();
                if (rule)
                {
                    if (AZStd::find(result.begin(), result.end(), rule->GetTangentSpace()) == result.end())
                    {
                        result.emplace_back(rule->GetTangentSpace());
                    }
                }
            }
        }

        return result;
    }


    AZ::SceneAPI::Events::ProcessingResult TangentGenerateComponent::GenerateTangentData(TangentGenerateContext& context)
    {
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
            if (!GenerateTangentsForMesh(context.GetScene(), nodeIndex, mesh))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }

            // Now that we have the tangents and bitangents, calculate the tangent w values for the ones that we imported from the scene file, as they only have xyz.
            UpdateFbxTangentWValues(graph, nodeIndex, mesh);
        }

        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }


    void TangentGenerateComponent::UpdateFbxTangentWValues(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::SceneAPI::DataTypes::IMeshData* meshData)
    {
        // Iterate over all UV sets.
        AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData = AZ::SceneAPI::SceneData::TangentsRule::FindUVData(graph, nodeIndex, 0);
        size_t uvSetIndex = 0;
        while (uvData)
        {
            // Get the tangents and bitangents from the source scene.
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData*    fbxTangentData   = AZ::SceneAPI::SceneData::TangentsRule::FindTangentData(graph, nodeIndex, uvSetIndex, AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene);
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData*  fbxBitangentData = AZ::SceneAPI::SceneData::TangentsRule::FindBitangentData(graph, nodeIndex, uvSetIndex, AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene);

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
                    fbxTangentData->SetTangent(i, tangent);
                }
            }

            // Find the next UV set.
            uvData = AZ::SceneAPI::SceneData::TangentsRule::FindUVData(graph, nodeIndex, ++uvSetIndex);
        }
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

    bool TangentGenerateComponent::GenerateTangentsForMesh(AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::SceneAPI::DataTypes::IMeshData* meshData)
    {
        AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

        // Check if we have any UV data, if not, we cannot possibly generate the tangents.
        AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData = AZ::SceneAPI::SceneData::TangentsRule::FindUVData(graph, nodeIndex, 0);
        if (!uvData)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "We cannot generate tangents for this mesh, as it has no UV coordinates!\n");
            return true; // No fatal error
        }

        // Check if we had tangents inside the source scene file.
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData*    fbxTangentData   = AZ::SceneAPI::SceneData::TangentsRule::FindTangentData(graph, nodeIndex, 0, AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene);
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData*  fbxBitangentData = AZ::SceneAPI::SceneData::TangentsRule::FindBitangentData(graph, nodeIndex, 0, AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene);

        // Check what tangent spaces we need.
        AZStd::vector<AZ::SceneAPI::DataTypes::TangentSpace> requiredSpaces = CollectRequiredTangentSpaces(scene);

        // If we have no tangent rules, so if the required spaces is empty.
        if (requiredSpaces.empty())
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Mesh '%s' has no tangents rule, assuming MikkT tangent space on UV set 0, using normalized tangents and orthogonal bitangents!\n", scene.GetGraph().GetNodeName(nodeIndex).GetName());
            requiredSpaces.emplace_back(AZ::SceneAPI::DataTypes::TangentSpace::MikkT);
        }

        // If all we need is import from the source scene, and we have tangent data from the source scene already, then skip generating.
        if ((requiredSpaces.size() == 1 && requiredSpaces[0] == AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene) && fbxTangentData && fbxBitangentData)
        {
            return true;
        }

        // Find all blend shape data under the mesh. We need to generate the tangent and bitangent for blend shape as well.
        AZStd::vector<AZ::SceneData::GraphData::BlendShapeData*> blendShapes;
        FindBlendShapes(graph, nodeIndex, blendShapes);

        // Generate all the tangent spaces we need.
        // Do this for every UV set.
        bool allSuccess = true;
        size_t uvSetIndex = 0;
        while (uvData)
        {
            for (AZ::SceneAPI::DataTypes::TangentSpace space : requiredSpaces)
            {
                switch (space)
                {
                // If we want Fbx tangents, we don't need to do anything for that.
                case AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene:
                {
                    allSuccess &= true;
                }
                break;

                // Generate using MikkT space.
                case AZ::SceneAPI::DataTypes::TangentSpace::MikkT:
                {
                    allSuccess &= AZ::TangentGeneration::Mesh::MikkT::GenerateTangents(
                        scene.GetManifest(), graph, nodeIndex, const_cast<AZ::SceneAPI::DataTypes::IMeshData*>(meshData), uvSetIndex);
                    for (AZ::SceneData::GraphData::BlendShapeData* blendShape : blendShapes)
                    {
                        allSuccess &= AZ::TangentGeneration::BlendShape::MikkT::GenerateTangents(blendShape, uvSetIndex);
                    }
                }
                break;

                // If we use EMotion FX calculated tangents, we don't need to generate this here.
                case AZ::SceneAPI::DataTypes::TangentSpace::EMotionFX:
                    allSuccess &= true;
                    break;

                default:
                {
                    AZ_Assert(false, "Unknown tangent space selected (spaceID=%d) for UV set %d, cannot generate tangents!\n", static_cast<AZ::u32>(space), uvSetIndex);
                    allSuccess = false;
                }
                }
            }

            // Try to find the next UV set.
            uvData = AZ::SceneAPI::SceneData::TangentsRule::FindUVData(graph, nodeIndex, ++uvSetIndex);
        }

        return allSuccess;
    }


    bool TangentGenerateComponent::CreateTangentBitangentLayers(AZ::SceneAPI::Containers::SceneManifest& manifest, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, size_t numVerts, size_t uvSetIndex, AZ::SceneAPI::DataTypes::TangentSpace tangentSpace, const char* spaceName, AZ::SceneAPI::Containers::SceneGraph& graph, AZ::SceneAPI::DataTypes::IMeshVertexTangentData** outTangentData, AZ::SceneAPI::DataTypes::IMeshVertexBitangentData** outBitangentData)
    {
        *outTangentData     = nullptr;
        *outBitangentData   = nullptr;

        //-------------------------------------------------------------
        // Create tangent layer.
        //-------------------------------------------------------------
        AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();
        tangentData->Resize(numVerts);

        AZ_Assert(tangentData, "Failed to allocate tangent data for scene graph.");
        if (!tangentData)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to allocate tangent data.\n");
            return false;
        }

        tangentData->SetTangentSetIndex(uvSetIndex);
        tangentData->SetTangentSpace(tangentSpace);

        const AZStd::string tangentGeneratedName = AZStd::string::format("TangentSet_%s_%zu", spaceName, uvSetIndex);
        const AZStd::string tangentSetName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(tangentGeneratedName, manifest);
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex newIndex = graph.AddChild(nodeIndex, tangentSetName.c_str(), tangentData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for tangent attribute.");
        if (!newIndex.IsValid())
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to create node in scene graph that stores tangent data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        //-------------------------------------------------------------
        // Create bitangent layer.
        //-------------------------------------------------------------
        AZStd::shared_ptr<AZ::SceneData::GraphData::MeshVertexBitangentData> bitangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexBitangentData>();
        bitangentData->Resize(numVerts);

        AZ_Assert(bitangentData, "Failed to allocate bitangent data for scene graph.");
        if (!bitangentData)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to allocate bitangent data.\n");
            return false;
        }

        bitangentData->SetBitangentSetIndex(uvSetIndex);
        bitangentData->SetTangentSpace(tangentSpace);

        const AZStd::string bitangentGeneratedName = AZStd::string::format("BitangentSet_%s_%zu", spaceName, uvSetIndex);
        const AZStd::string bitangentSetName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(bitangentGeneratedName, manifest);
        newIndex = graph.AddChild(nodeIndex, bitangentSetName.c_str(), bitangentData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for bitangent attribute.");
        if (!newIndex.IsValid())
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to create node in scene graph that stores bitangent data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        *outTangentData     = tangentData.get();
        *outBitangentData   = bitangentData.get();
        return true;
    }
} // namespace AZ::SceneGenerationComponents
