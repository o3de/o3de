/*
 * All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates
 *or its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root
 *of this distribution(the "License").All use of this software is governed by
 *the License, or, if provided, by the license below or the license
 *accompanying this file.Do not remove or modify any license notices.This file
 *is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *KIND, either express or implied.
 *
 */

#include <Model/ModelAssetBuilderComponent.h>
#include <Model/MaterialAssetBuilderComponent.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

 /**
  * DEBUG DEFINES!
  * These are useful for debugging bad behavior from the builder.
  * By default this builder wants to merge meshes as much as possible
  * to cut down on the number of buffers it has to create. This is generally
  * helpful for rendering but can make debugging difficult.
  *
  * If you experience artifacts from models built by this builder try
  * commenting these out to disable certain merging features. This will
  * produce a large volume of buffers for large models but it should be a lot
  * easier to step through.
  */
#define AZ_RPI_MERGE_MESHES_BY_MATERIAL_UID
#define AZ_RPI_MESHES_SHARE_COMMON_BUFFERS

namespace
{
    const uint32_t IndicesPerFace = 3;
    const AZ::RHI::Format IndicesFormat = AZ::RHI::Format::R32_UINT;

    const uint32_t PositionFloatsPerVert = 3;
    const uint32_t NormalFloatsPerVert   = 3;
    const uint32_t UVFloatsPerVert       = 2;
    const uint32_t ColorFloatsPerVert    = 4;
    const uint32_t TangentFloatsPerVert  = 4; // The 4th channel is used to indicate handedness of the bitangent, either 1 or -1.
    const uint32_t BitangentFloatsPerVert = 3; 
    
    const AZ::RHI::Format PositionFormat = AZ::RHI::Format::R32G32B32_FLOAT;
    const AZ::RHI::Format NormalFormat   = AZ::RHI::Format::R32G32B32_FLOAT;
    const AZ::RHI::Format UVFormat       = AZ::RHI::Format::R32G32_FLOAT;
    const AZ::RHI::Format ColorFormat    = AZ::RHI::Format::R32G32B32A32_FLOAT;
    const AZ::RHI::Format TangentFormat = AZ::RHI::Format::R32G32B32A32_FLOAT; // The 4th channel is used to indicate handedness of the bitangent, either 1 or -1.
    const AZ::RHI::Format BitangentFormat = AZ::RHI::Format::R32G32B32_FLOAT;
}

namespace AZ
{
    class Aabb;

    namespace RPI
    {
        static const char* s_builderName = "Atom Model Builder";
        static const uint64_t s_invalidMaterialUid = 0;

        void ModelAssetBuilderComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<ModelAssetBuilderComponent, SceneAPI::SceneCore::ExportingComponent>()
                    ->Version(19);  // [ATOM-14419]
            }
        }

        ModelAssetBuilderComponent::ModelAssetBuilderComponent()
        {
            BindToCall(&ModelAssetBuilderComponent::BuildModel);
        }

        //Supports a case-insensitive check for "lodN" or "lod_N" or "lod-N" or "lod:N" or "lod|N" or "lod#N" or "lod N" at the end of the name for the current node or an ancestor node.
        //Returns -1 if no valid naming convention is found.
        int GetLodIndexByNamingConvention(const char* name, size_t len)
        {
            //look for "lodN"
            if (len >= 4)
            {   
                const char* subStr = &name[len - 4];
                if (azstrnicmp(subStr, "lod", 3) == 0)
                {
                    const char lastLetter = name[len - 1];
                    if (AZStd::is_digit(lastLetter))
                    {
                        return static_cast<int>(lastLetter) - '0';
                    }
                }
            }

            //look for "lod_N"
            if (len >= 5)
            {
                const char* subStr = &name[len - 5];
                if (azstrnicmp(subStr, "lod", 3) == 0)
                {
                    if (strchr("_-:|# ", name[len - 2]))
                    {
                        const char lastLetter = name[len - 1];
                        if (AZStd::is_digit(lastLetter))
                        {
                            return static_cast<int>(lastLetter) - '0';
                        }
                    }
                }
            }
            return -1;
        }

        SceneAPI::Events::ProcessingResult ModelAssetBuilderComponent::BuildModel(ModelAssetBuilderContext& context)
        {
            {
                auto assetIdOutcome = RPI::AssetUtils::MakeAssetId("ResourcePools/DefaultVertexBufferPool.resourcepool", 0);
                if (!assetIdOutcome.IsSuccess())
                {
                    return SceneAPI::Events::ProcessingResult::Failure;
                }
                m_systemInputAssemblyBufferPoolId = assetIdOutcome.GetValue();
            }

            m_createdSubId.clear();

            m_modelName = context.m_group.GetName();

            const auto& scene = context.m_scene;
            const auto& sceneGraph = scene.GetGraph();

            m_sourceUuid = scene.GetSourceGuid();

            auto names = sceneGraph.GetNameStorage();
            auto content = sceneGraph.GetContentStorage();

            // Create a downwards, breadth-first view into the scene
            auto pairView = AZ::SceneAPI::Containers::Views::MakePairView(names, content);
            auto view = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<
                AZ::SceneAPI::Containers::Views::BreadthFirst>(
                    sceneGraph, sceneGraph.GetRoot(), pairView.cbegin(), true);

            AZStd::vector<SourceMeshContentList> sourceMeshContentListsByLod;

            AZStd::shared_ptr<const SceneAPI::DataTypes::ILodRule> lodRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneAPI::DataTypes::ILodRule>();
            AZStd::vector<AZStd::vector<AZStd::string>> selectedMeshPathsByLod;
            if (lodRule)
            {
                selectedMeshPathsByLod.resize(lodRule->GetLodCount());
                for (size_t lod = 0; lod < lodRule->GetLodCount(); ++lod)
                {
                    selectedMeshPathsByLod[lod] = SceneAPI::Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph,
                        lodRule->GetSceneNodeSelectionList(lod), SceneAPI::Utilities::SceneGraphSelector::IsMesh);
                }
            }

            // Gather the list of nodes in the graph that are selected as part of this
            // MeshGroup defined in context.m_group.
            AZStd::vector<AZStd::string> selectedMeshPaths = SceneAPI::Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph,
                context.m_group.GetSceneNodeSelectionList(), SceneAPI::Utilities::SceneGraphSelector::IsMesh);

            // Iterate over the downwards, breadth-first view into the scene.
            // First we have to split the source mesh data up by lod.
            for (const auto& viewIt : view)
            {
                if (viewIt.second != nullptr &&
                    azrtti_istypeof<MeshData>(viewIt.second.get()))
                {
                    const AZStd::string& meshPath = viewIt.first.GetPath();
                    const AZStd::string& meshName = viewIt.first.GetName();

                    uint32_t lodIndex = 0; // Default to the 0th LOD if nothing is found
                    if (lodRule)
                    {
                        // The LodRule contains the objects for Lod1 through LodN. Objects at Lod0 are not include in the LodRule
                        for (size_t lod = 0; lod < selectedMeshPathsByLod.size(); ++lod)
                        {
                            AZStd::vector<AZStd::string>& paths = selectedMeshPathsByLod[lod];
                            const auto it = AZStd::find(paths.begin(), paths.end(), meshPath);
                            if (it != paths.end())
                            {
                                lodIndex = aznumeric_cast<uint32_t>(lod + 1);
                                break;
                            }
                        }
                        if (lodIndex == 0)
                        {
                            // Object was not found in the LodRule, but we still need to see if it was in the selection list
                            const auto selectedMeshPathsIt = AZStd::find(selectedMeshPaths.begin(), selectedMeshPaths.end(), meshPath);
                            if(selectedMeshPathsIt == selectedMeshPaths.end())
                            {
                                continue;
                            }
                        }
                    }
                    else
                    {
                        // Skip the mesh if it's not in the MeshGroup's selected mesh list
                        const auto selectedMeshPathsIt = AZStd::find(selectedMeshPaths.begin(), selectedMeshPaths.end(), meshPath);
                        if(selectedMeshPathsIt == selectedMeshPaths.end())
                        {
                            continue;
                        }

                        // Select the Lod that this mesh is part of                        
                        {
                            int lodIndexFromName = GetLodIndexByNamingConvention(meshName.c_str(), meshName.size());
                            if (lodIndexFromName >= 0)
                            {
                                lodIndex = aznumeric_cast<uint32_t>(lodIndexFromName);
                            }
                            else
                            {
                                // If the mesh node's name doesn't have the LOD identifier in it lets walk the parent hierarchy
                                // The first parent node that has the LOD identifier is the LOD this mesh will be a part of
                                SceneAPI::Containers::SceneGraph::NodeIndex meshNodeIndex = sceneGraph.Find(meshPath);

                                SceneAPI::Containers::SceneGraph::NodeIndex parentNodeIndex = sceneGraph.GetNodeParent(meshNodeIndex);
                                while (parentNodeIndex != sceneGraph.GetRoot())
                                {
                                    const SceneAPI::Containers::SceneGraph::Name& parentNodeName = sceneGraph.GetNodeName(parentNodeIndex);

                                    lodIndexFromName = GetLodIndexByNamingConvention(parentNodeName.GetName(), parentNodeName.GetNameLength());
                                    if (lodIndexFromName >= 0)
                                    {
                                        lodIndex = aznumeric_cast<uint32_t>(lodIndexFromName);
                                        break;
                                    }

                                    parentNodeIndex = sceneGraph.GetNodeParent(parentNodeIndex);
                                }
                            }
                        }
                    }

                    // Find which LodAssetBuilder we need to add this mesh to
                    // If the lod is new we need to create and begin a new builder
                    if (lodIndex + 1 >= sourceMeshContentListsByLod.size())
                    {
                        sourceMeshContentListsByLod.resize(lodIndex + 1);
                    }

                    SourceMeshContentList& sourceMeshContentList = sourceMeshContentListsByLod[lodIndex];

                    //Gather mesh content
                    SourceMeshContent sourceMesh;
                    sourceMesh.m_name = meshName;

                    const auto node = sceneGraph.Find(meshPath);
                    sourceMesh.m_worldTransform = GetWorldTransform(sceneGraph, node);
                    auto sibling = sceneGraph.GetNodeChild(node);

                    AddToMeshContent(viewIt.second, sourceMesh);

                    bool traversing = true;
                    while (traversing)
                    {
                        if (sibling.IsValid())
                        {
                            auto siblingContent = sceneGraph.GetNodeContent(sibling);

                            AddToMeshContent(siblingContent, sourceMesh);

                            sibling = sceneGraph.GetNodeSibling(sibling);
                        }
                        else
                        {
                            traversing = false;
                        }
                    }

                    // We've traversed this node and all its children that hold
                    // relevant data  We can move it into the list of content for this lod
                    sourceMeshContentList.emplace_back(AZStd::move(sourceMesh));
                }
            }

            // Then in each Lod we need to group all faces by material id.
            // All sub meshes with the same material id get merged
            AZStd::vector<Data::Asset<ModelLodAsset>> lodAssets;
            lodAssets.resize(sourceMeshContentListsByLod.size());

            uint32_t lodIndex = 0;
            for (const SourceMeshContentList& sourceMeshContentList : sourceMeshContentListsByLod)
            {
                ModelLodAssetCreator lodAssetCreator;
                m_lodName = AZStd::string::format("lod%d", lodIndex);
                AZStd::string lodAssetName = GetAssetFullName(ModelLodAsset::TYPEINFO_Uuid());
                lodAssetCreator.Begin(CreateAssetId(lodAssetName));

                {
                    ProductMeshContentList lodMeshes = SourceMeshListToProductMeshList(sourceMeshContentList);

#if defined(AZ_RPI_MERGE_MESHES_BY_MATERIAL_UID)
                    lodMeshes = MergeMeshesByMaterialUid(lodMeshes);
#endif

#if defined(AZ_RPI_MESHES_SHARE_COMMON_BUFFERS)
                    // We shouldn't need a mesh name for the buffer names since meshed are sharing common buffers
                    m_meshName = "";
                    ProductMeshViewList lodMeshViews;

                    ProductMeshContent mergedMesh;
                    MergeMeshesToCommonBuffers(lodMeshes, mergedMesh, lodMeshViews);

                    BufferAssetView indexBuffer;
                    AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo> streamBuffers;

                    if (!CreateModelLodBuffers(mergedMesh, indexBuffer, streamBuffers, lodAssetCreator))
                    {
                        return AZ::SceneAPI::Events::ProcessingResult::Failure;
                    }

                    for (const ProductMeshView& meshView : lodMeshViews)
                    {
                        if (!CreateMesh(meshView, indexBuffer, streamBuffers, lodAssetCreator, context.m_materialsByUid))
                        {
                            return AZ::SceneAPI::Events::ProcessingResult::Failure;
                        }
                    }
#else
                    uint32_t meshIndex = 0;
                    for (const ProductMeshContent& mesh : lodMeshes)
                    {
                        const ProductMeshView meshView = CreateViewToEntireMesh(mesh);

                        BufferAssetView indexBuffer;
                        AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo> streamBuffers;

                        // Mesh name in ProductMeshContent could be duplicated so generate unique mesh name using index 
                        m_meshName = AZStd::string::format("mesh%d", meshIndex++);

                        if (!CreateModelLodBuffers(mesh, indexBuffer, streamBuffers, lodAssetCreator))
                        {
                            return AZ::SceneAPI::Events::ProcessingResult::Failure;
                        }

                        if (!CreateMesh(meshView, indexBuffer, streamBuffers, lodAssetCreator, context.m_materialsByUid))
                        {
                            return AZ::SceneAPI::Events::ProcessingResult::Failure;
                        }
                    }
#endif
                }

                if (!lodAssetCreator.End(lodAssets[lodIndex]))
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Failure;
                }
                lodAssets[lodIndex].SetHint(lodAssetName); // name will be used for file name when export asset

                lodIndex++;
            }
            sourceMeshContentListsByLod.clear();

            // Build the final asset structure
            ModelAssetCreator modelAssetCreator;
            AZStd::string modelAssetName = GetAssetFullName(ModelAsset::TYPEINFO_Uuid());
            modelAssetCreator.Begin(CreateAssetId(modelAssetName));

            // Finalize all LOD assets
            for (auto& lodAsset : lodAssets)
            {
                modelAssetCreator.AddLodAsset(AZStd::move(lodAsset));
            }

            // Finalize the model
            if (!modelAssetCreator.End(context.m_outputModelAsset))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }
            context.m_outputModelAsset.SetHint(modelAssetName);

            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }

        void ModelAssetBuilderComponent::AddToMeshContent(
            const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IGraphObject>& data,
            SourceMeshContent& content)
        {
            if (azrtti_istypeof<MeshData>(data.get()))
            {
                auto meshData = AZStd::static_pointer_cast<const MeshData>(data);
                content.m_meshData = meshData;
            }
            else if (azrtti_istypeof<UVData>(data.get()))
            {
                auto uvData = AZStd::static_pointer_cast<const UVData>(data);
                content.m_meshUVData.push_back(uvData);
            }
            else if (azrtti_istypeof<ColorData>(data.get()))
            {
                auto colorData = AZStd::static_pointer_cast<const ColorData>(data);
                content.m_meshColorData.push_back(colorData);
            }
            else if (azrtti_istypeof<TangentData>(data.get()))
            {
                auto tangentData = AZStd::static_pointer_cast<const TangentData>(data);
                if (!content.m_meshTangents)
                {
                    content.m_meshTangents = tangentData;
                }
                else
                {
                    AZ_Warning(s_builderName, false, "Found multiple tangent data sets. Only the first will be used.");
                }
            }
            else if (azrtti_istypeof<BitangentData>(data.get()))
            {
                auto bitangentData = AZStd::static_pointer_cast<const BitangentData>(data);
                if (!content.m_meshBitangents)
                {
                    content.m_meshBitangents = bitangentData;
                }
                else
                {
                    AZ_Warning(s_builderName, false, "Found multiple bitangent data sets. Only the first will be used.");
                }
            }
            else if (azrtti_istypeof<MaterialData>(data.get()))
            {
                auto materialData = AZStd::static_pointer_cast<const MaterialData>(data);
                content.m_materials.push_back(materialData->GetUniqueId());
            }
        }

        ModelAssetBuilderComponent::ProductMeshContentList ModelAssetBuilderComponent::SourceMeshListToProductMeshList(
            const SourceMeshContentList& sourceMeshList)
        {
            ProductMeshContentList productMeshList;

            using Face = SceneAPI::DataTypes::IMeshData::Face;
            using FaceList = AZStd::vector<Face>;
            using FacesByMaterialUid = AZStd::unordered_map<MaterialUid, FaceList>;
            using ProductList = AZStd::vector<FacesByMaterialUid>;

            ProductList productList;
            productList.resize(sourceMeshList.size());

            AZStd::vector<SceneAPI::DataTypes::MatrixType> meshTransforms;
            meshTransforms.reserve(sourceMeshList.size());

            size_t productMeshCount = 0;

            // Break up source data by material uid. We don't do any merging
            // we just can't output a mesh that has faces with multiple materials.
            for (size_t i = 0; i < sourceMeshList.size(); ++i)
            {
                const SourceMeshContent& sourceMeshContent = sourceMeshList[i];
                FacesByMaterialUid& productsByMaterialUid = productList[i];

                meshTransforms.push_back(sourceMeshContent.m_worldTransform);

                const auto& meshData = sourceMeshContent.m_meshData;

                const uint32_t faceCount = meshData->GetFaceCount();

                for (uint32_t j = 0; j < faceCount; ++j)
                {
                    const Face& faceInfo = meshData->GetFaceInfo(j);
                    const MaterialUid matUid = sourceMeshContent.GetMaterialUniqueId(meshData->GetFaceMaterialId(j));

                    FaceList& faceInfoList = productsByMaterialUid[matUid];
                    faceInfoList.push_back(faceInfo);
                }

                productMeshCount += productsByMaterialUid.size();
            }

            productMeshList.reserve(productMeshCount);

            for (size_t i = 0; i < productList.size(); ++i)
            {
                const FacesByMaterialUid& productsByMaterialUid = productList[i];
                const SceneAPI::DataTypes::MatrixType& meshTransform = meshTransforms[i];

                const SceneAPI::DataTypes::MatrixType inverseTranspose = meshTransform.GetInverseFull().GetTranspose();

                const SourceMeshContent& sourceMesh = sourceMeshList[i];

                const auto& meshData = sourceMesh.m_meshData;
                const auto& uvContentCollection = sourceMesh.m_meshUVData;
                const size_t uvSetCount = uvContentCollection.size();
                const auto& colorContentCollection = sourceMesh.m_meshColorData;
                const size_t colorSetCount = colorContentCollection.size();

                for (const auto& it : productsByMaterialUid)
                {
                    ProductMeshContent productMesh;
                    productMesh.m_name = sourceMesh.m_name;

                    productMesh.m_materialUid = it.first;

                    const FaceList& faceInfoList = it.second;
                    uint32_t indexCount = static_cast<uint32_t>(faceInfoList.size()) * 3;

                    productMesh.m_indices.reserve(indexCount);

                    for (const Face& faceInfo : faceInfoList)
                    {
                        productMesh.m_indices.push_back(faceInfo.vertexIndex[0]);
                        productMesh.m_indices.push_back(faceInfo.vertexIndex[1]);
                        productMesh.m_indices.push_back(faceInfo.vertexIndex[2]);
                    }

                    // We need to both gather a collection of unique 
                    // indices so that we don't gather duplicate vertex data
                    // while also correcting the collection of indices 
                    // that we have so that they start at 0 and are contiguous. 
                    AZStd::map<uint32_t, uint32_t> oldToNewIndices;
                    uint32_t newIndex = 0;
                    for (uint32_t& index : productMesh.m_indices)
                    {
                        if (oldToNewIndices.find(index) == oldToNewIndices.end())
                        {
                            oldToNewIndices[index] = newIndex;
                            newIndex++;
                        }

                        index = oldToNewIndices[index];
                    }

                    AZStd::vector<float>& positions = productMesh.m_positions;
                    AZStd::vector<float>& normals = productMesh.m_normals;
                    AZStd::vector<float>& tangents = productMesh.m_tangents;
                    AZStd::vector<float>& bitangents = productMesh.m_bitangents;
                    AZStd::vector<AZStd::vector<float>>& uvSets = productMesh.m_uvSets;
                    AZStd::vector<AZ::Name>& uvNames = productMesh.m_uvCustomNames;
                    AZStd::vector<AZStd::vector<float>>& colorSets = productMesh.m_colorSets;
                    AZStd::vector<AZ::Name>& colorNames = productMesh.m_colorCustomNames;

                    const size_t vertexCount = oldToNewIndices.size();
                    positions.reserve(vertexCount * PositionFloatsPerVert);
                    normals.reserve(vertexCount * NormalFloatsPerVert);

                    if (sourceMesh.m_meshTangents)
                    {
                        tangents.reserve(vertexCount * TangentFloatsPerVert);

                        if (sourceMesh.m_meshBitangents)
                        {
                            bitangents.reserve(vertexCount * BitangentFloatsPerVert);
                        }
                    }

                    uvNames.reserve(uvSetCount);
                    for (auto& uvContent : uvContentCollection)
                    {
                        uvNames.push_back(uvContent->GetCustomName());
                    }

                    uvSets.resize(uvSetCount);
                    for (auto& uvSet : uvSets)
                    {
                        uvSet.reserve(vertexCount * UVFloatsPerVert);
                    }

                    colorNames.reserve(colorSetCount);
                    for (auto& colorContent : colorContentCollection)
                    {
                        colorNames.push_back(colorContent->GetCustomName());
                    }

                    colorSets.resize(colorSetCount);
                    for (auto& colorSet : colorSets)
                    {
                        colorSet.reserve(vertexCount * ColorFloatsPerVert);
                    }

                    for (const auto& itr : oldToNewIndices)
                    {
                        // We use the 'old' index as that properly indexes 
                        // into the old mesh data. The 'new' index is used for properly
                        // indexing into this new collection that we're building here.
                        const uint32_t oldIndex = itr.first;

                        AZ::Vector3 pos = meshData->GetPosition(oldIndex);
                        AZ::Vector3 normal = meshData->GetNormal(oldIndex);

                        // Pre-multiply transform
                        pos = meshTransform * pos;

                        positions.push_back(pos.GetX());
                        positions.push_back(pos.GetY());
                        positions.push_back(pos.GetZ());

                        // Multiply normal by inverse transpose to avoid 
                        // incorrect values produced by non-uniformly scaled
                        // transforms.
                        normal = inverseTranspose.TransformVector(normal);
                        normal.Normalize();

                        normals.push_back(normal.GetX());
                        normals.push_back(normal.GetY());
                        normals.push_back(normal.GetZ());

                        if (sourceMesh.m_meshTangents)
                        {
                            AZ::Vector4 tangentWithW = sourceMesh.m_meshTangents->GetTangent(oldIndex);
                            AZ::Vector3 tangent = tangentWithW.GetAsVector3();
                            float bitangentSign = tangentWithW.GetW();

                            tangent = meshTransform.TransformVector(tangent);
                            tangent.Normalize();

                            tangents.push_back(tangent.GetX());
                            tangents.push_back(tangent.GetY());
                            tangents.push_back(tangent.GetZ());
                            tangents.push_back(bitangentSign);

                            if (sourceMesh.m_meshBitangents)
                            {
                                AZ::Vector3 bitangent = sourceMesh.m_meshBitangents->GetBitangent(oldIndex);

                                bitangent = meshTransform.TransformVector(bitangent);
                                bitangent.Normalize();

                                bitangents.push_back(bitangent.GetX());
                                bitangents.push_back(bitangent.GetY());
                                bitangents.push_back(bitangent.GetZ());
                            }
                        }

                        // Gather UVs
                        for (uint32_t ii = 0; ii < uvSetCount; ++ii)
                        {
                            auto& uvs = uvSets[ii];
                            const auto& uvContent = uvContentCollection[ii];

                            AZ::Vector2 uv = uvContent->GetUV(oldIndex);

                            uvs.push_back(uv.GetX());
                            uvs.push_back(uv.GetY());
                        }

                        // Gather Colors
                        for (uint32_t ii = 0; ii < colorSetCount; ++ii)
                        {
                            auto& colors = colorSets[ii];
                            const auto& colorContent = colorContentCollection[ii];

                            SceneAPI::DataTypes::Color color = colorContent->GetColor(oldIndex);

                            colors.push_back(color.red);
                            colors.push_back(color.green);
                            colors.push_back(color.blue);
                            colors.push_back(color.alpha);
                        }
                    }

                    productMeshList.emplace_back(productMesh);
                }
            }

            return productMeshList;
        }

        ModelAssetBuilderComponent::ProductMeshContentList ModelAssetBuilderComponent::MergeMeshesByMaterialUid(const ProductMeshContentList& productMeshList)
        {
            ProductMeshContentList mergedMeshList;

            {
                AZStd::unordered_map<MaterialUid, ProductMeshContentList> meshesByMatUid;

                // First pass to reserve memory
                // This saves time with very large meshes
                {
                    AZStd::unordered_map<MaterialUid, size_t> meshCountByMatUid;

                    for (const ProductMeshContent& mesh : productMeshList)
                    {
                        meshCountByMatUid[mesh.m_materialUid]++;
                    }

                    for (const auto& it : meshCountByMatUid)
                    {
                        meshesByMatUid[it.first].reserve(it.second);
                    }
                }

                for (const ProductMeshContent& mesh : productMeshList)
                {
                    meshesByMatUid[mesh.m_materialUid].push_back(mesh);
                }

                const size_t mergedMeshCount = meshesByMatUid.size();

                mergedMeshList.reserve(mergedMeshCount);

                for (const auto& it : meshesByMatUid)
                {
                    ProductMeshContent mergedMesh = MergeMeshList(it.second, RemapIndices);
                    mergedMesh.m_materialUid = it.first;

                    mergedMeshList.emplace_back(AZStd::move(mergedMesh));
                }
            }

            return mergedMeshList;
        }

        ModelAssetBuilderComponent::ProductMeshView ModelAssetBuilderComponent::CreateViewToEntireMesh(const ProductMeshContent& mesh)
        {
            ProductMeshView meshView;

            auto meshIndexCount = static_cast<uint32_t>(mesh.m_indices.size());
            auto meshPositionsFloatCount = static_cast<uint32_t>(mesh.m_positions.size());
            auto meshNormalsFloatCount = static_cast<uint32_t>(mesh.m_normals.size());

            auto meshPositionCount = meshPositionsFloatCount / PositionFloatsPerVert;
            auto meshNormalsCount = meshNormalsFloatCount / NormalFloatsPerVert;

            meshView.m_indexView = RHI::BufferViewDescriptor::CreateTyped(0, meshIndexCount, IndicesFormat);
            meshView.m_positionView = RHI::BufferViewDescriptor::CreateTyped(0, meshPositionCount, PositionFormat);
            if (meshNormalsCount > 0)
            {
                meshView.m_normalView = RHI::BufferViewDescriptor::CreateTyped(0, meshNormalsCount, NormalFormat);
            }

            meshView.m_uvSetViews.reserve(mesh.m_uvSets.size());
            meshView.m_uvCustomNames.resize(mesh.m_uvCustomNames.size());
            AZ_Assert(mesh.m_uvSets.size() == mesh.m_uvCustomNames.size(), "UV set size doesn't match the number of custom uv names");
            for (uint32_t uvSetIndex = 0; uvSetIndex < mesh.m_uvSets.size(); uvSetIndex++)
            {
                const auto& uvSet = mesh.m_uvSets[uvSetIndex];
                auto uvFloatCount = static_cast<uint32_t>(uvSet.size());
                auto uvCount = uvFloatCount / UVFloatsPerVert;

                meshView.m_uvSetViews.push_back(RHI::BufferViewDescriptor::CreateTyped(0, uvCount, UVFormat));
                meshView.m_uvCustomNames.push_back(mesh.m_uvCustomNames[uvSetIndex]);
            }

            meshView.m_colorSetViews.reserve(mesh.m_colorSets.size());
            meshView.m_colorCustomNames.resize(mesh.m_colorCustomNames.size());
            for (uint32_t colorSetIndex = 0; colorSetIndex < mesh.m_colorSets.size(); colorSetIndex++)
            {
                const auto& colorSet = mesh.m_colorSets[colorSetIndex];
                auto colorFloatCount = static_cast<uint32_t>(colorSet.size());
                auto colorCount = colorFloatCount / ColorFloatsPerVert;

                meshView.m_colorSetViews.push_back(RHI::BufferViewDescriptor::CreateTyped(0, colorCount, ColorFormat));
                meshView.m_colorCustomNames.push_back(mesh.m_colorCustomNames[colorSetIndex]);
            }

            if (!mesh.m_tangents.empty())
            {
                meshView.m_tangentView = RHI::BufferViewDescriptor::CreateTyped(0, meshNormalsCount, TangentFormat);
            }

            if (!mesh.m_bitangents.empty())
            {
                meshView.m_bitangentView = RHI::BufferViewDescriptor::CreateTyped(0, meshNormalsCount, BitangentFormat);
            }

            meshView.m_materialUid = mesh.m_materialUid;

            return meshView;
        }

        void ModelAssetBuilderComponent::MergeMeshesToCommonBuffers(
            const ProductMeshContentList& lodMeshList,
            ProductMeshContent& lodMeshContent,
            ProductMeshViewList& meshViews)
        {
            meshViews.reserve(lodMeshList.size());

            // We want to merge these meshes into one large 
            // ProductMesh. That large buffer gets set on the LOD directly
            // rather than a Mesh in the LOD.
            ProductMeshContentAllocInfo lodBufferInfo;
            
            for (const ProductMeshContent& mesh : lodMeshList)
            {
                if (lodBufferInfo.m_uvSetFloatCounts.size() < mesh.m_uvSets.size())
                {
                    lodBufferInfo.m_uvSetFloatCounts.resize(mesh.m_uvSets.size());
                }

                if (lodBufferInfo.m_colorSetFloatCounts.size() < mesh.m_colorSets.size())
                {
                    lodBufferInfo.m_colorSetFloatCounts.resize(mesh.m_colorSets.size());
                }

                // Once again we save a lot of time and memory by determining what we 
                // need to allocate up-front

                auto meshIndexCount = static_cast<uint32_t>(mesh.m_indices.size());
                auto meshPositionsFloatCount = static_cast<uint32_t>(mesh.m_positions.size());
                auto meshNormalsFloatCount = static_cast<uint32_t>(mesh.m_normals.size());
                auto meshTangentsFloatCount = static_cast<uint32_t>(mesh.m_tangents.size());
                auto meshBitangentsFloatCount = static_cast<uint32_t>(mesh.m_bitangents.size());

                // For each element we need to:
                // record the offset for the view
                // accumulate the allocation info
                // fill the rest of the data for the view

                ProductMeshView meshView;
                meshView.m_name = mesh.m_name;
                meshView.m_indexView = RHI::BufferViewDescriptor::CreateTyped(static_cast<uint32_t>(lodBufferInfo.m_indexCount), meshIndexCount, IndicesFormat);
                lodBufferInfo.m_indexCount += meshIndexCount;

                const uint32_t meshVertexCount = meshPositionsFloatCount / PositionFloatsPerVert;

                if (!mesh.m_positions.empty())
                {
                    const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_positionsFloatCount) / PositionFloatsPerVert;
                    meshView.m_positionView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, PositionFormat);
                    lodBufferInfo.m_positionsFloatCount += meshPositionsFloatCount;
                }

                if (!mesh.m_normals.empty())
                {
                    const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_normalsFloatCount) / NormalFloatsPerVert;
                    meshView.m_normalView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, NormalFormat);
                    lodBufferInfo.m_normalsFloatCount += meshNormalsFloatCount;
                }

                if (!mesh.m_tangents.empty())
                {
                    const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_tangentsFloatCount) / TangentFloatsPerVert;
                    meshView.m_tangentView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, TangentFormat);
                    lodBufferInfo.m_tangentsFloatCount += meshTangentsFloatCount;
                }

                if (!mesh.m_bitangents.empty())
                {
                    const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_bitangentsFloatCount) / BitangentFloatsPerVert;
                    meshView.m_bitangentView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, BitangentFormat);
                    lodBufferInfo.m_bitangentsFloatCount += meshBitangentsFloatCount;
                }

                const size_t uvSetCount = mesh.m_uvSets.size();
                if (uvSetCount > 0)
                {
                    meshView.m_uvSetViews.resize(uvSetCount);
                    meshView.m_uvCustomNames.resize(uvSetCount);
                    for (size_t i = 0; i < uvSetCount; ++i)
                    {
                        meshView.m_uvCustomNames[i] = mesh.m_uvCustomNames[i];

                        auto& uvSetView = meshView.m_uvSetViews[i];

                        const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_uvSetFloatCounts[i]) / UVFloatsPerVert;
                        uvSetView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, UVFormat);

                        const auto uvCount = static_cast<uint32_t>(mesh.m_uvSets[i].size());
                        lodBufferInfo.m_uvSetFloatCounts[i] += uvCount;
                    }
                }

                const size_t colorSetCount = mesh.m_colorSets.size();
                if (colorSetCount > 0)
                {
                    meshView.m_colorSetViews.resize(colorSetCount);
                    meshView.m_colorCustomNames.resize(colorSetCount);
                    for (size_t i = 0; i < colorSetCount; ++i)
                    {
                        meshView.m_colorCustomNames[i] = mesh.m_colorCustomNames[i];

                        auto& colorSetView = meshView.m_colorSetViews[i];

                        const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_colorSetFloatCounts[i]) / ColorFloatsPerVert;
                        colorSetView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, ColorFormat);

                        const auto colorCount = static_cast<uint32_t>(mesh.m_colorSets[i].size());
                        lodBufferInfo.m_colorSetFloatCounts[i] += colorCount;
                    }
                }

                meshView.m_materialUid = mesh.m_materialUid;

                meshViews.emplace_back(AZStd::move(meshView));

            }

            // Now that we have the views settled, we can just merge the mesh
            lodMeshContent = MergeMeshList(lodMeshList, PreserveIndices);
        }

        ModelAssetBuilderComponent::ProductMeshContent ModelAssetBuilderComponent::MergeMeshList(
            const ProductMeshContentList& productMeshList,
            IndicesOperation indicesOp)
        {
            ProductMeshContent mergedMesh;

            // A preallocation pass for the merged mesh
            {
                size_t indexCount = 0;
                size_t positionCount = 0;
                size_t normalCount = 0;
                size_t tangentCount = 0;
                size_t bitangentCount = 0;
                AZStd::vector<size_t> uvSetCounts;
                AZStd::vector<size_t> colorSetCounts;

                for (const ProductMeshContent& mesh : productMeshList)
                {
                    indexCount += mesh.m_indices.size();
                    positionCount += mesh.m_positions.size();
                    normalCount += mesh.m_normals.size();
                    tangentCount += mesh.m_tangents.size();
                    bitangentCount += mesh.m_bitangents.size();

                    if (mesh.m_uvSets.size() > uvSetCounts.size())
                    {
                        uvSetCounts.resize(mesh.m_uvSets.size());
                    }

                    for (size_t i = 0; i < mesh.m_uvSets.size(); ++i)
                    {
                        uvSetCounts[i] += mesh.m_uvSets[i].size();
                    }

                    if (mesh.m_colorSets.size() > colorSetCounts.size())
                    {
                        colorSetCounts.resize(mesh.m_colorSets.size());
                    }

                    for (size_t i = 0; i < mesh.m_colorSets.size(); ++i)
                    {
                        colorSetCounts[i] += mesh.m_colorSets[i].size();
                    }
                }

                mergedMesh.m_indices.reserve(indexCount);
                mergedMesh.m_positions.reserve(positionCount);
                mergedMesh.m_normals.reserve(normalCount);
                mergedMesh.m_tangents.reserve(tangentCount);
                mergedMesh.m_bitangents.reserve(bitangentCount);

                mergedMesh.m_uvCustomNames.resize(uvSetCounts.size());
                for (auto& mesh : productMeshList)
                {
                    int32_t nameCount = aznumeric_cast<int32_t>(mesh.m_uvCustomNames.size());
                    // Backward stack, the first mesh defines the name.
                    for (int32_t i = nameCount - 1; i >= 0; --i)
                    {
                        mergedMesh.m_uvCustomNames[i] = mesh.m_uvCustomNames[i];
                    }
                }

                mergedMesh.m_uvSets.resize(uvSetCounts.size());
                for (size_t i = 0; i < uvSetCounts.size(); ++i)
                {
                    mergedMesh.m_uvSets[i].reserve(uvSetCounts[i]);
                }

                mergedMesh.m_colorCustomNames.resize(colorSetCounts.size());
                for (auto& mesh : productMeshList)
                {
                    int32_t nameCount = aznumeric_cast<int32_t>(mesh.m_colorCustomNames.size());
                    // Backward stack, the first mesh defines the name.
                    for (int32_t i = nameCount - 1; i >= 0; --i)
                    {
                        mergedMesh.m_colorCustomNames[i] = mesh.m_colorCustomNames[i];
                    }
                }

                mergedMesh.m_colorSets.resize(colorSetCounts.size());
                for (size_t i = 0; i < colorSetCounts.size(); ++i)
                {
                    mergedMesh.m_colorSets[i].reserve(colorSetCounts[i]);
                }
            }

            uint32_t tailIndex = 0;

            // Append each common mesh onto this LOD-wide mesh 
            for (const ProductMeshContent& mesh : productMeshList)
            {
                if(mergedMesh.m_name.IsEmpty())
                {
                    mergedMesh.m_name = mesh.m_name;
                }
                else
                {
                    mergedMesh.m_name = AZStd::string::format("%s+%s", mergedMesh.m_name.GetCStr(), mesh.m_name.GetCStr());
                }

                AZStd::vector<uint32_t> indices = mesh.m_indices;

                if (indicesOp == RemapIndices)
                {
                    /**
                     * Remap indices to start where the last mesh left off
                     * If mesh 0 has indices 0,1,2 and mesh 1 has indices 0,1,2
                     * we need to rescale them so that mesh 1 has indices 3,4,5
                     */
                    uint32_t largestIndex = 0;

                    for (uint32_t& index : indices)
                    {
                        index += tailIndex;

                        if (index > largestIndex)
                        {
                            largestIndex = index;
                        }
                    }

                    // +1 because if the largest index is 5 we want the next index to start at 6
                    tailIndex = largestIndex + 1;
                }

                mergedMesh.m_indices.insert(
                    mergedMesh.m_indices.end(), indices.begin(), indices.end());

                if (!mesh.m_positions.empty())
                {
                    mergedMesh.m_positions.insert(
                        mergedMesh.m_positions.end(), mesh.m_positions.begin(), mesh.m_positions.end());
                }

                if (!mesh.m_normals.empty())
                {
                    mergedMesh.m_normals.insert(
                        mergedMesh.m_normals.end(), mesh.m_normals.begin(), mesh.m_normals.end());
                }

                if (!mesh.m_tangents.empty())
                {
                    mergedMesh.m_tangents.insert(
                        mergedMesh.m_tangents.end(), mesh.m_tangents.begin(), mesh.m_tangents.end());
                }

                if (!mesh.m_bitangents.empty())
                {
                    mergedMesh.m_bitangents.insert(
                        mergedMesh.m_bitangents.end(), mesh.m_bitangents.begin(), mesh.m_bitangents.end());
                }

                const size_t uvSetCount = mesh.m_uvSets.size();
                for (size_t i = 0; i < uvSetCount; ++i)
                {
                    mergedMesh.m_uvSets[i].insert(
                        mergedMesh.m_uvSets[i].end(), mesh.m_uvSets[i].begin(), mesh.m_uvSets[i].end());
                }

                const size_t colorSetCount = mesh.m_colorSets.size();
                for (size_t i = 0; i < colorSetCount; ++i)
                {
                    mergedMesh.m_colorSets[i].insert(
                        mergedMesh.m_colorSets[i].end(), mesh.m_colorSets[i].begin(), mesh.m_colorSets[i].end());
                }
            }

            return mergedMesh;
        }

        bool ModelAssetBuilderComponent::CreateModelLodBuffers(
            const ProductMeshContent& lodBufferContent,
            BufferAssetView& outIndexBuffer,
            AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& outStreamBuffers,
            ModelLodAssetCreator& lodAssetCreator)
        {
            const AZStd::vector<uint32_t>& indices = lodBufferContent.m_indices;
            const AZStd::vector<float>& positions = lodBufferContent.m_positions;
            const AZStd::vector<float>& normals = lodBufferContent.m_normals;
            const AZStd::vector<float>& tangents = lodBufferContent.m_tangents;
            const AZStd::vector<float>& bitangents = lodBufferContent.m_bitangents;
            const AZStd::vector<AZStd::vector<float>>& uvSets = lodBufferContent.m_uvSets;
            const AZStd::vector<AZ::Name>& uvCustomNames = lodBufferContent.m_uvCustomNames;
            const AZStd::vector<AZStd::vector<float>>& colorSets = lodBufferContent.m_colorSets;
            const AZStd::vector<AZ::Name>& colorCustomNames = lodBufferContent.m_colorCustomNames;

            const size_t vertexCount = positions.size() / PositionFloatsPerVert;

            // Build Index Buffer ...
            {
                Outcome<Data::Asset<BufferAsset>> indexBufferOutcome = CreateBufferAsset(indices.data(), indices.size(), IndicesFormat, "index");
                if (!indexBufferOutcome.IsSuccess())
                {
                    AZ_Error(s_builderName, false, "Failed to build index stream");
                    return false;
                }

                outIndexBuffer = { indexBufferOutcome.GetValue(), indexBufferOutcome.GetValue()->GetBufferViewDescriptor() };
            }

            // Build various stream buffers ...

            auto buildStreamBuffer = [this, vertexCount, &outStreamBuffers](
                const AZStd::vector<float>& floats,
                AZ::RHI::Format format,
                const RHI::ShaderSemantic& semantic,
                const AZ::Name& customStreamName = AZ::Name())
            {
                size_t expectedElementCount = vertexCount * RHI::GetFormatComponentCount(format);
                if (expectedElementCount != floats.size())
                {
                    AZ_Error(s_builderName, false, "Failed to build %s stream. Expected %d elements but found %d.", semantic.ToString().data(), expectedElementCount, floats.size());
                    return false;
                }

                AZStd::string bufferName = semantic.ToString();
                Outcome<Data::Asset<BufferAsset>> bufferOutcome = CreateBufferAsset(floats.data(), vertexCount, format, bufferName);
                if (!bufferOutcome.IsSuccess())
                {
                    AZ_Error(s_builderName, false, "Failed to build %s stream", semantic.ToString().data());
                    return false;
                }

                outStreamBuffers.push_back({semantic, customStreamName, {bufferOutcome.GetValue(), bufferOutcome.GetValue()->GetBufferViewDescriptor()}});
                return true;
            };

            if (!buildStreamBuffer(positions, PositionFormat, RHI::ShaderSemantic{"POSITION"}))
            {
                return false;
            }

            if (!buildStreamBuffer(normals, NormalFormat, RHI::ShaderSemantic{"NORMAL"}))
            {
                return false;
            }

            if (!tangents.empty())
            {
                if (!buildStreamBuffer(tangents, TangentFormat, RHI::ShaderSemantic{"TANGENT"}))
                {
                    return false;
                }
            }

            if (!bitangents.empty())
            {
                if (!buildStreamBuffer(bitangents, BitangentFormat, RHI::ShaderSemantic{"BITANGENT"}))
                {
                    return false;
                }
            }
            
            for (size_t i = 0; i < uvSets.size(); ++i)
            {
                if (!buildStreamBuffer(uvSets[i], UVFormat, RHI::ShaderSemantic{"UV", i}, uvCustomNames[i]))
                {
                    return false;
                }
            }

            for (size_t i = 0; i < colorSets.size(); ++i)
            {
                if (!buildStreamBuffer(colorSets[i], ColorFormat, RHI::ShaderSemantic{"COLOR", i}, colorCustomNames[i]))
                {
                    return false;
                }
            }
            
            lodAssetCreator.SetLodIndexBuffer(outIndexBuffer.GetBufferAsset());

            for (const auto& streamBufferInfo : outStreamBuffers)
            {
                lodAssetCreator.AddLodStreamBuffer(streamBufferInfo.m_bufferAssetView.GetBufferAsset());
            }

            return true;
        }

        bool ModelAssetBuilderComponent::CreateMesh(
            const ProductMeshView& meshView,
            const BufferAssetView& lodIndexBuffer,
            const AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& lodStreamBuffers,
            ModelLodAssetCreator& lodAssetCreator,
            const MaterialAssetsByUid& materialAssetsByUid)
        {
            lodAssetCreator.BeginMesh();
            
            if (meshView.m_materialUid != s_invalidMaterialUid)
            {
                auto iter = materialAssetsByUid.find(meshView.m_materialUid);
                if (iter != materialAssetsByUid.end())
                {
                    const Data::Asset<MaterialAsset>& materialAsset = iter->second.m_asset;
                    lodAssetCreator.SetMeshMaterialAsset(materialAsset);
                }
            }

            lodAssetCreator.SetMeshName(meshView.m_name);

            // Set the index stream
            BufferAssetView indexBufferAssetView(lodIndexBuffer.GetBufferAsset(), meshView.m_indexView);

            lodAssetCreator.SetMeshIndexBuffer(AZStd::move(indexBufferAssetView));

            {
                // Build the mesh's Aabb
                ModelLodAsset::Mesh::StreamBufferInfo positionStreamBufferInfo;
                const RHI::ShaderSemantic& positionSemantic = RHI::ShaderSemantic{"POSITION"};
                if (!FindStreamBufferById(lodStreamBuffers, positionSemantic, positionStreamBufferInfo))
                {
                    return false;
                }

                const RHI::BufferViewDescriptor& positionBufferViewDescriptor = meshView.m_positionView;

                // Calculate SubMesh's AABB from position stream
                AZ::Aabb subMeshAabb = AZ::Aabb::CreateNull();
                if (CalculateAABB(positionBufferViewDescriptor, *positionStreamBufferInfo.m_bufferAssetView.GetBufferAsset().Get(), subMeshAabb))
                {
                    lodAssetCreator.SetMeshAabb(AZStd::move(subMeshAabb));
                }
                else
                {
                    AZ_Warning(s_builderName, false, "Failed to calculate AABB for Mesh");
                }

                // Set position buffer
                BufferAssetView meshPositionBufferAssetView(
                    positionStreamBufferInfo.m_bufferAssetView.GetBufferAsset(),
                    meshView.m_positionView);

                lodAssetCreator.AddMeshStreamBuffer(positionSemantic, AZ::Name(), meshPositionBufferAssetView);
            }

            // Set normal buffer
            if (meshView.m_normalView.m_elementCount > 0)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{"NORMAL"}, AZ::Name(), meshView.m_normalView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            // Set UV buffers
            for (size_t i = 0; i < meshView.m_uvSetViews.size(); ++i)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{"UV", i}, meshView.m_uvCustomNames[i], meshView.m_uvSetViews[i], lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            // Set Color buffers
            for (size_t i = 0; i < meshView.m_colorSetViews.size(); ++i)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{"COLOR", i}, meshView.m_colorCustomNames[i], meshView.m_colorSetViews[i], lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            // Set Tangent/Bitangent buffer
            if (meshView.m_tangentView.m_elementCount > 0)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{"TANGENT"}, AZ::Name(), meshView.m_tangentView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }
            if (meshView.m_bitangentView.m_elementCount > 0)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{"BITANGENT"}, AZ::Name(), meshView.m_bitangentView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            lodAssetCreator.EndMesh();

            return true;
        }

        Outcome<Data::Asset<BufferAsset>> ModelAssetBuilderComponent::CreateBufferAsset(
            const void* data, const size_t elementCount, RHI::Format format, const AZStd::string& bufferName)
        {
            BufferAssetCreator creator;
            AZStd::string bufferAssetName = GetAssetFullName(BufferAsset::TYPEINFO_Uuid(), bufferName);
            creator.Begin(CreateAssetId(bufferAssetName));

            RHI::BufferViewDescriptor bufferViewDescriptor =
                RHI::BufferViewDescriptor::CreateTyped(0, static_cast<uint32_t>(elementCount), format);

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = bufferViewDescriptor.m_elementSize * bufferViewDescriptor.m_elementCount;

            creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);

            creator.SetBufferViewDescriptor(bufferViewDescriptor);

            creator.SetPoolAsset({ m_systemInputAssemblyBufferPoolId, azrtti_typeid<RPI::ResourcePoolAsset>() });

            Data::Asset<BufferAsset> bufferAsset;
            if (creator.End(bufferAsset))
            {
                bufferAsset.SetHint(bufferAssetName);
                return AZ::Success(bufferAsset);
            }

            return AZ::Failure();
        }

        bool ModelAssetBuilderComponent::SetMeshStreamBufferById(
            const RHI::ShaderSemantic& semantic,
            const AZ::Name& customName,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            const AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& lodStreamBuffers,
            ModelLodAssetCreator& lodAssetCreator)
        {
            ModelLodAsset::Mesh::StreamBufferInfo streamBufferInfo;

            if (FindStreamBufferById(lodStreamBuffers, semantic, streamBufferInfo))
            {
                Data::Asset<BufferAsset> bufferAsset = streamBufferInfo.m_bufferAssetView.GetBufferAsset();

                lodAssetCreator.AddMeshStreamBuffer(semantic, customName, { bufferAsset, bufferViewDescriptor });

                return true;
            }

            AZ_Error(s_builderName, false, "Failed to apply the %s buffer to the mesh", semantic.ToString().data());
            return false;
        }

        AZStd::string ModelAssetBuilderComponent::GetAssetFullName(const TypeId& assetType, const AZStd::string& bufferName)
        {
            AZStd::string fullName;

            if (assetType == ModelAsset::TYPEINFO_Uuid())
            {
                fullName = m_modelName;
            }
            else if (assetType == ModelLodAsset::TYPEINFO_Uuid())
            {
                fullName = AZStd::string::format("%s_%s", m_modelName.c_str(), m_lodName.c_str());
            }
            else
            {
                if (m_meshName.empty())
                {
                    fullName = AZStd::string::format("%s_%s_%s", m_modelName.c_str(), m_lodName.c_str(), bufferName.c_str());
                }
                else
                {
                    fullName = AZStd::string::format("%s_%s_%s_%s", m_modelName.c_str(), m_lodName.c_str(), m_meshName.c_str(), bufferName.c_str());
                }
            }
            return fullName;
        }

        Data::AssetId ModelAssetBuilderComponent::CreateAssetId(const AZStd::string& assetName)
        {
            // The sub id of any model related assets starts with the same prefix 0x10 for first 8 bits
            // And it uses the name hash for the last 24 bits
            static const uint32_t prefix = 0x10000000;

            uint32_t productSubId;
            Data::AssetId assetId;
            assetId.SetInvalid();

            productSubId = prefix | AZ::Crc32(assetName) & 0xffffff;

            if (m_createdSubId.find(productSubId) != m_createdSubId.end())
            {
                AZ_Error("Mesh builder", false, "Duplicate asset sub id for asset [%s]", assetName.c_str());
                return assetId;
            }

            m_createdSubId.insert(productSubId);

            assetId.m_guid = m_sourceUuid;
            assetId.m_subId = productSubId;

            return assetId;
        }

        bool ModelAssetBuilderComponent::CalculateAABB(const RHI::BufferViewDescriptor& bufferViewDesc, const BufferAsset& bufferAsset, AZ::Aabb& aabb)
        {
            const uint32_t elementSize = bufferViewDesc.m_elementSize;
            const uint32_t elementCount = bufferViewDesc.m_elementCount;
            const uint32_t elementOffset = bufferViewDesc.m_elementOffset;
            AZ_Assert(elementOffset + elementCount <= bufferAsset.GetBufferViewDescriptor().m_elementCount, "bufferViewDesc is out of range of bufferAsset");

            // Position is 3 floats
            if (elementSize == sizeof(float) * 3)
            {
                AZ_Assert(bufferViewDesc.m_elementFormat == RHI::Format::R32G32B32_FLOAT, "position buffer format does not match element size");
                
                struct Position { float x,y,z; };
                const Position* buffer = reinterpret_cast<const Position*>(&bufferAsset.GetBuffer()[0]) + elementOffset;

                AZ::Vector3 vpos;    //note: it seems to be fastest to reuse a local Vector3 rather than constructing new ones each loop iteration
                for (uint32_t i = 0; i < elementCount; ++i)
                {
                    vpos.Set(const_cast<float*>(reinterpret_cast<const float*>(&buffer[i])));
                    aabb.AddPoint(vpos);
                }
            }
            // Position is 4 halfs
            else if (elementSize == sizeof(uint16_t) * 4)
            {
                // Can't handle this yet since we have no way to do math on
                // halfs
                AZ_Error(
                    s_builderName, false,
                    "Can't calculate AABB for SubMesh; positions stored "
                    "in halfs not supported.");
                return false;
            }
            else
            {
                // No idea what type of position stream this is
                AZ_Error(
                    s_builderName, false,
                    "Can't calculate AABB for SubMesh; can't determine "
                    "element type of stream.");
                return false;
            }

            return true;
        }
        
        ModelAssetBuilderComponent::MaterialUid ModelAssetBuilderComponent::SourceMeshContent::GetMaterialUniqueId(uint32_t index) const
        {
            if (index >= m_materials.size())
            {
                return s_invalidMaterialUid;
            }

            return m_materials[index];
        }

        bool ModelAssetBuilderComponent::FindStreamBufferById(
            const AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferInfoList,
            const RHI::ShaderSemantic& streamSemantic,
            ModelLodAsset::Mesh::StreamBufferInfo& outStreamBufferInfo)
        {
            for (const auto& streamBufferInfo : streamBufferInfoList)
            {
                if (streamBufferInfo.m_semantic == streamSemantic)
                {
                    outStreamBufferInfo = streamBufferInfo;
                    return true;
                }
            }

            AZ_Error(s_builderName, false, "Attempted to find a buffer for stream %s but failed!", streamSemantic.ToString().data());
            return false;
        }

        SceneAPI::DataTypes::MatrixType ModelAssetBuilderComponent::GetWorldTransform(const SceneAPI::Containers::SceneGraph& sceneGraph, SceneAPI::Containers::SceneGraph::NodeIndex node)
        {
            // the logic here copies the logic in @AZ::RC::WorldMatrixExporter::ConcatenateMatricesUpwards
            namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
            namespace SceneViews = AZ::SceneAPI::Containers::Views;

            SceneAPI::DataTypes::MatrixType transform = SceneAPI::DataTypes::MatrixType::CreateIdentity();

            const SceneAPI::Containers::SceneGraph::NodeHeader* nodeIterator = sceneGraph.ConvertToHierarchyIterator(node);
            auto upwardsView = SceneViews::MakeSceneGraphUpwardsView(sceneGraph, nodeIterator, sceneGraph.GetContentStorage().cbegin(), true);
            for (auto it = upwardsView.begin(); it != upwardsView.end(); ++it)
            {
                if (!(*it))
                {
                    continue;
                }

                const SceneAPI::DataTypes::IGraphObject* nodeTemp = it->get();
                const SceneDataTypes::ITransform* nodeTransform = azrtti_cast<const SceneDataTypes::ITransform*>(nodeTemp);
                if (nodeTransform)
                {
                    transform = nodeTransform->GetMatrix() * transform;
                }
                else
                {
                    // If the translation is not an end point it means it's its own group as opposed to being
                    //      a component of the parent, so only list end point children.
                    auto view = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(sceneGraph, it.GetHierarchyIterator(),
                        sceneGraph.GetContentStorage().begin(), true);
                    auto result = AZStd::find_if(view.begin(), view.end(), SceneAPI::Containers::DerivedTypeFilter<SceneDataTypes::ITransform>());
                    if (result != view.end())
                    {
                        transform = azrtti_cast<const SceneDataTypes::ITransform*>(result->get())->GetMatrix() * transform;
                    }
                }
            }

            return transform;
        }
    } // namespace RPI
} // namespace AZ
