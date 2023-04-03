/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/UVsGenerator/UVsGenerateComponent.h>
#include <Generation/Components/UVsGenerator/UVsGenerators/SphereMappingUVsGenerator.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>

#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>

#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>

#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ::SceneGenerationComponents
{
    UVsGenerateComponent::UVsGenerateComponent()
    {
        BindToCall(&UVsGenerateComponent::GenerateUVsData);
    }

    void UVsGenerateComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<UVsGenerateComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(1);
        }
    }

    const AZ::SceneAPI::SceneData::UVsRule* UVsGenerateComponent::GetUVsRule(const AZ::SceneAPI::Containers::Scene& scene) const
    {
        for (const auto& object : scene.GetManifest().GetValueStorage())
        {
            if (object->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid()))
            {
                const AZ::SceneAPI::DataTypes::IGroup* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IGroup*>(object.get());
                const AZ::SceneAPI::SceneData::UVsRule* rule = group->GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::UVsRule>().get();
                if (rule)
                {
                    return rule;
                }
            }
        }

        return nullptr;
    }

    AZ::SceneAPI::Events::ProcessingResult UVsGenerateComponent::GenerateUVsData(UVsGenerateContext& context)
    {
        // this component runs regardless of what modifiers are present on the mesh.
        // Set some defaults to use if no settings are present at all:
        AZ::SceneAPI::DataTypes::UVsGenerationMethod defaultGenerationMethod =
            AZ::SceneAPI::SceneData::UVsRule::GetDefaultGenerationMethodWithNoRule();
        bool defaultReplaceExisting = false;

        const AZ::SceneAPI::SceneData::UVsRule* uvsRule = GetUVsRule(context.GetScene());
        const AZ::SceneAPI::DataTypes::UVsGenerationMethod generationMethod = uvsRule ? uvsRule->GetGenerationMethod() : defaultGenerationMethod;
        const bool replaceExisting = uvsRule ? uvsRule->GetReplaceExisting() : defaultReplaceExisting;

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
            // Generate UVs for the mesh
            if (!GenerateUVsForMesh(context.GetScene(), nodeIndex, mesh, generationMethod, replaceExisting))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }
        }

        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }

    bool UVsGenerateComponent::GenerateUVsForMesh(
        AZ::SceneAPI::Containers::Scene& scene,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        AZ::SceneAPI::DataTypes::IMeshData* meshData,
        const AZ::SceneAPI::DataTypes::UVsGenerationMethod generationMethod,
        const bool replaceExisting)
    {
        if (generationMethod == SceneAPI::DataTypes::UVsGenerationMethod::LeaveSceneDataAsIs)
        {
            return true;
        }

        AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
        size_t uvSetCount = CalcUvSetCount(graph, nodeIndex);

        // there might already be existing data there - see if there is.
        AZ::SceneData::GraphData::MeshVertexUVData* dataToFill = nullptr;
        if (uvSetCount > 0)
        {
            // This modifier always works on UV Set #0.
            dataToFill = static_cast<AZ::SceneData::GraphData::MeshVertexUVData*>(FindUvData(graph, nodeIndex, 0));
        }
        AZStd::string currentNodeName = graph.GetNodeName(nodeIndex).GetPath();
        if ((dataToFill) && (!replaceExisting))
        {
            // if there's already data, and we are not set to replace existing, do not generate data
            AZ_Info(
                AZ::SceneAPI::Utilities::LogWindow,
                "Asked to generate UVs for mesh " AZ_STRING_FORMAT " but it already has UVs and 'replace existing' is not set.  Not replacing existing data.\n",
                AZ_STRING_ARG(currentNodeName));
            return true; // this is not an error!
        }

        if (!dataToFill)
        {
            if (!CreateUVsLayer(scene.GetManifest(), nodeIndex, graph, &dataToFill))
            {
                // the above function will emit an error if it fails.
                return false;
            }
        }

        bool allSuccess = true;
        AZ_Info(
            AZ::SceneAPI::Utilities::LogWindow,
            "Generating UVs for " AZ_STRING_FORMAT ".\n",
            AZ_STRING_ARG(currentNodeName));

        switch (generationMethod)
        {
        case AZ::SceneAPI::DataTypes::UVsGenerationMethod::SphericalProjection:
            {
                allSuccess &= AZ::UVsGeneration::Mesh::SphericalMapping::GenerateUVsSphericalMapping(meshData, dataToFill);
                break;
            }
            // for future expansion - add new methods here if you want to support additional methods of UV auto generation.
        default:
            {
                AZ_Assert(false, "Unknown UVs generation method selected (%d) cannot generate UVs.\n", static_cast<AZ::u32>(generationMethod));
                allSuccess = false;
            }
        }

        return allSuccess;
    }

    size_t UVsGenerateComponent::CalcUvSetCount(
        AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        size_t result = 0;
        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(
            graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* data =
                azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
            if (data)
            {
                result++;
            }
        }

        return result;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexUVData* UVsGenerateComponent::FindUvData(
        AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 uvSet) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        AZ::u64 uvSetIndex = 0;
        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(
            graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* data =
                azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
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

    bool UVsGenerateComponent::CreateUVsLayer(
        AZ::SceneAPI::Containers::SceneManifest& manifest,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        AZ::SceneAPI::Containers::SceneGraph& graph,
        SceneData::GraphData::MeshVertexUVData** outUVsData)
    {
        *outUVsData = nullptr;

        AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvData =
            AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexUVData>();

        AZ_Assert(uvData, "Failed to allocate UV data for scene graph.");
        if (!uvData)
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to allocate UV data.\n");
            return false;
        }

       
        const AZStd::string uvSetName =
            AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexUVData>("UV0", manifest);
        uvData->SetCustomName(uvSetName.c_str());

        AZ::SceneAPI::Containers::SceneGraph::NodeIndex newIndex = graph.AddChild(nodeIndex, uvSetName.c_str(), uvData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for UVs attribute.");
        if (!newIndex.IsValid())
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create node in scene graph that stores UV data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        *outUVsData = uvData.get();
        return true;
    }

} // namespace AZ::SceneGenerationComponents
