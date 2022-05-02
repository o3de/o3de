/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Model/ModelAssetBuilderComponent.h>
#include <Model/MaterialAssetBuilderComponent.h>
#include <Model/MorphTargetExporter.h>
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
#include <Atom/RPI.Reflect/Model/MorphTargetDelta.h>
#include <Atom/RPI.Reflect/Model/SkinJointIdPadding.h>
#include <Atom/RPI.Reflect/Model/SkinMetaAssetCreator.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ICoordinateSystemRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ISkinRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IClothRule.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Rules/StaticMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneUtilities.h>
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
#define AZ_RPI_MESHES_SHARE_COMMON_BUFFERS

namespace
{
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

    const char* ShaderSemanticName_SkinJointIndices = "SKIN_JOINTINDICES";
    const char* ShaderSemanticName_SkinWeights = "SKIN_WEIGHTS";
    const AZ::RHI::Format SkinWeightFormat = AZ::RHI::Format::R32_FLOAT; // Single-component, 32-bit floating point per weight

    // Morph targets
    const char* ShaderSemanticName_MorphTargetDeltas = "MORPHTARGET_VERTEXDELTAS";

    // Cloth data
    const char* const ShaderSemanticName_ClothData = "CLOTH_DATA";
    const uint32_t ClothDataFloatsPerVert = 4;
    const AZ::RHI::Format ClothDataFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
}

namespace AZ
{
    class Aabb;

    namespace RPI
    {
        static const uint64_t s_invalidMaterialUid = 0;

        void ModelAssetBuilderComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<ModelAssetBuilderComponent, SceneAPI::SceneCore::ExportingComponent>()
                    ->Version(32);  // Updating morph targets to be per-mesh instead of per-lod
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

            // The Atom Model builder uses the optimized versions of meshes that are
            // placed in the SceneGraph during its generation phase. Users select
            // meshes based on their original name, and the mesh optimizer adds the
            // suffix "_optimized" to these mesh nodes in the scene graph. To target
            // these nodes, first filter for the non-optimized mesh nodes, then remap
            // from the non-optimized one to the optimized one. This callable is used
            // to filter for mesh nodes that are not the optimized ones.
            const auto isNonOptimizedMesh = [](const SceneAPI::Containers::SceneGraph& graph, SceneAPI::Containers::SceneGraph::NodeIndex& index)
            {
                return SceneAPI::Utilities::SceneGraphSelector::IsMesh(graph, index) &&
                    !AZStd::string_view{graph.GetNodeName(index).GetName(), graph.GetNodeName(index).GetNameLength()}.ends_with(SceneAPI::Utilities::OptimizedMeshSuffix);
            };

            if (lodRule)
            {
                selectedMeshPathsByLod.resize(lodRule->GetLodCount());
                for (size_t lod = 0; lod < lodRule->GetLodCount(); ++lod)
                {
                    selectedMeshPathsByLod[lod] = SceneAPI::Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph,
                        lodRule->GetSceneNodeSelectionList(lod), isNonOptimizedMesh, SceneAPI::Utilities::SceneGraphSelector::RemapToOptimizedMesh);
                }
            }

            // Gather the list of nodes in the graph that are selected as part of this
            // MeshGroup defined in context.m_group, then remap to the optimized mesh
            // nodes, if they exist.
            AZStd::vector<AZStd::string> selectedMeshPaths = SceneAPI::Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph,
                context.m_group.GetSceneNodeSelectionList(), isNonOptimizedMesh, SceneAPI::Utilities::SceneGraphSelector::RemapToOptimizedMesh);

            // Iterate over the downwards, breadth-first view into the scene.
            // First we have to split the source mesh data up by lod.
            for (const auto& viewIt : view)
            {
                if (viewIt.second != nullptr &&
                    azrtti_istypeof<MeshData>(viewIt.second.get()))
                {
                    const AZStd::string meshPath(viewIt.first.GetPath(), viewIt.first.GetPathLength());
                    const AZStd::string meshName(viewIt.first.GetName(), viewIt.first.GetNameLength());

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
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Using mesh '%s'", meshPath.c_str());

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

                    // Gather mesh content
                    SourceMeshContent sourceMesh;

                    // Although the nodes used to gather mesh content are the optimized ones (when found), to make
                    // this process transparent for the end-asset generated, the name assigned to the source mesh
                    // content will not include the "_optimized" prefix.
                    AZStd::string_view sourceMeshName = meshName;
                    if (sourceMeshName.ends_with(SceneAPI::Utilities::OptimizedMeshSuffix))
                    {
                        sourceMeshName.remove_suffix(SceneAPI::Utilities::OptimizedMeshSuffix.size());
                    }
                    sourceMesh.m_name = sourceMeshName;

                    const auto node = sceneGraph.Find(meshPath);
                    sourceMesh.m_worldTransform = AZ::SceneAPI::Utilities::DetermineWorldTransform(scene, node, context.m_group.GetRuleContainerConst());

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

                    sourceMesh.m_isMorphed = GetIsMorphed(sceneGraph, node);

                    // Get the cloth data (only for full mesh LOD 0).
                    sourceMesh.m_meshClothData = (lodIndex == 0)
                        ? SceneAPI::DataTypes::IClothRule::FindClothData(
                            sceneGraph, node, sourceMesh.m_meshData->GetVertexCount(), context.m_group.GetRuleContainerConst())
                        : AZStd::vector<AZ::Color>{};

                    // We've traversed this node and all its children that hold
                    // relevant data  We can move it into the list of content for this lod
                    sourceMeshContentList.emplace_back(AZStd::move(sourceMesh));
                }
            }

            // Then in each Lod we need to group all faces by material id.
            // All sub meshes with the same material id get merged
            AZStd::vector<Data::Asset<ModelLodAsset>> lodAssets;
            lodAssets.resize(sourceMeshContentListsByLod.size());

            // Joint name to joint index map used for the skinning influences.
            AZStd::unordered_map<AZStd::string, uint16_t> jointNameToIndexMap;

            AZStd::string modelAssetName = GetAssetFullName(ModelAsset::TYPEINFO_Uuid());
            const AZ::Data::AssetId modelAssetId = CreateAssetId(modelAssetName);

            MorphTargetMetaAssetCreator morphTargetMetaCreator;
            morphTargetMetaCreator.Begin(MorphTargetMetaAsset::ConstructAssetId(modelAssetId, modelAssetName));
            
            ModelAssetCreator modelAssetCreator;
            modelAssetCreator.Begin(modelAssetId);

            uint32_t lodIndex = 0;
            for (const SourceMeshContentList& sourceMeshContentList : sourceMeshContentListsByLod)
            {
                ModelLodAssetCreator lodAssetCreator;
                m_lodName = AZStd::string::format("lod%d", lodIndex);
                AZStd::string lodAssetName = GetAssetFullName(ModelLodAsset::TYPEINFO_Uuid());
                lodAssetCreator.Begin(CreateAssetId(lodAssetName));

                {
                    ProductMeshContentList lodMeshes = SourceMeshListToProductMeshList(context, sourceMeshContentList, jointNameToIndexMap, morphTargetMetaCreator);

                    PadVerticesForSkinning(lodMeshes);

                    // By default, we merge meshes that share the same material
                    bool canMergeMeshes = true;

                    AZStd::shared_ptr<const SceneAPI::SceneData::StaticMeshAdvancedRule> staticMeshAdvancedRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneAPI::SceneData::StaticMeshAdvancedRule>();
                    if (staticMeshAdvancedRule && !staticMeshAdvancedRule->MergeMeshes())
                    {
                        // If the merge meshes option is disabled in the advanced mesh rule, don't merge meshes
                        canMergeMeshes = false;
                    }
                    else
                    {
                        for (const SourceMeshContent& sourceMesh : sourceMeshContentList)
                        {
                            if (sourceMesh.m_isMorphed)
                            {
                                // Merging meshes shuffles around the order of the vertices, but morph targets rely on having an index that tell them which vertices to morph
                                // We do not merge morphed meshes so that this index is preserved and correct.
                                // If we keep track of the ordering changes in MergeMeshesByMaterialUid and then re-mapped the MORPHTARGET_VERTEXINDICES buffer
                                // we could potentially enable merging meshes that are morphed. But for now, disable merging.
                                canMergeMeshes = false;
                                break;
                            }
                        }
                    }

                    if (canMergeMeshes)
                    {
                        lodMeshes = MergeMeshesByMaterialUid(lodMeshes);
                    }

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
                        if (!CreateMesh(meshView, indexBuffer, streamBuffers, modelAssetCreator, lodAssetCreator, context.m_materialsByUid))
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

            // Fill the skin meta asset
            if (!jointNameToIndexMap.empty())
            {
                SkinMetaAssetCreator skinCreator;
                skinCreator.Begin(SkinMetaAsset::ConstructAssetId(modelAssetId, modelAssetName));

                skinCreator.SetJointNameToIndexMap(jointNameToIndexMap);

                if (!skinCreator.End(context.m_outputSkinMetaAsset))
                {
                    AZ_Warning(s_builderName, false, "Cannot create skin meta asset. Skinning influences won't be automatically relinked.");
                }
            }

            // Fill the morph target meta asset
            if (!morphTargetMetaCreator.IsEmpty())
            {
                if (!morphTargetMetaCreator.End(context.m_outputMorphTargetMetaAsset))
                {
                    AZ_Warning(s_builderName, false, "Cannot create morph target meta asset for model asset '%s'.", modelAssetName.c_str());
                }
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
                    AZ_Warning(s_builderName, false,
                        "Found multiple tangent data sets for mesh '%s'. Only the first will be used.",
                        content.m_name.GetCStr());
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
                    AZ_Warning(s_builderName, false,
                        "Found multiple bitangent data sets for mesh '%s'. Only the first will be used.",
                        content.m_name.GetCStr());
                }
            }
            else if (azrtti_istypeof<MaterialData>(data.get()))
            {
                auto materialData = AZStd::static_pointer_cast<const MaterialData>(data);
                content.m_materials.push_back(materialData->GetUniqueId());
            }
            else if (azrtti_istypeof<SkinData>(data.get()))
            {
                content.m_skinData.emplace_back(data, static_cast<const SkinData*>(data.get()));
            }
        }

        ModelAssetBuilderComponent::ProductMeshContentList ModelAssetBuilderComponent::SourceMeshListToProductMeshList(
            const ModelAssetBuilderContext& context,
            const SourceMeshContentList& sourceMeshList,
            AZStd::unordered_map<AZStd::string, uint16_t>& jointNameToIndexMap,
            MorphTargetMetaAssetCreator& morphTargetMetaCreator)
        {
            ProductMeshContentList productMeshList;

            using Face = SceneAPI::DataTypes::IMeshData::Face;
            using FaceList = AZStd::vector<Face>;
            struct UidFaceList
            {
                MaterialUid m_materialUid;
                FaceList m_faceList;
            };
            using FacesByMaterialUid = AZStd::vector<UidFaceList>;
            using ProductList = AZStd::vector<FacesByMaterialUid>;

            ProductList productList;
            productList.resize(sourceMeshList.size());

            AZStd::vector<SceneAPI::DataTypes::MatrixType> meshTransforms;
            meshTransforms.reserve(sourceMeshList.size());

            size_t productMeshCount = 0;

            MorphTargetExporter morphTargetExporter;

            // Break up source data by material uid. We don't do any merging at this point,
            // and we don't sort by material id at this point so that the resulting vertex data
            // will have a 1-1 relationship with the source data. This ensures morph target indices
            // don't need to be re-mapped, as long as the meshes aren't merged later
            // We just can't output a mesh that has faces with multiple materials.
            for (size_t i = 0; i < sourceMeshList.size(); ++i)
            {
                const SourceMeshContent& sourceMeshContent = sourceMeshList[i];
                FacesByMaterialUid& productsByMaterialUid = productList[i];

                meshTransforms.push_back(sourceMeshContent.m_worldTransform);

                const auto& meshData = sourceMeshContent.m_meshData;

                const uint32_t faceCount = meshData->GetFaceCount();

                MaterialUid currentMaterialId = std::numeric_limits<MaterialUid>::max();
                for (uint32_t j = 0; j < faceCount; ++j)
                {
                    const Face& faceInfo = meshData->GetFaceInfo(j);
                    const MaterialUid matUid = sourceMeshContent.GetMaterialUniqueId(meshData->GetFaceMaterialId(j));

                    // Start a new product mesh if the material changed
                    if (currentMaterialId != matUid)
                    {
                        UidFaceList uidFaceList;
                        uidFaceList.m_materialUid = matUid;
                        productsByMaterialUid.push_back(uidFaceList);
                        currentMaterialId = matUid;
                    }

                    // Add the faceinfo to the current product mesh
                    UidFaceList& currentFaceList = productsByMaterialUid.back();
                    currentFaceList.m_faceList.push_back(faceInfo);                    
                }

                productMeshCount += productsByMaterialUid.size();
            }
            productMeshList.reserve(productMeshCount);

            // Get the default values if there is no skin rule
            m_skinRuleSettings = SceneAPI::DataTypes::GetDefaultSkinRuleSettings();

            // Get the skin rule, if it exists
            if (const auto* skinRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneAPI::DataTypes::ISkinRule>().get())
            {
                m_skinRuleSettings.m_maxInfluencesPerVertex = skinRule->GetMaxWeightsPerVertex();
                m_skinRuleSettings.m_weightThreshold = skinRule->GetWeightThreshold();
            }

            
            // Keep track of the order of sub-meshes for morph targets.
            // We cannot re-order sub-meshes after this unless we also update the morph target data
            // This is because one morph target may impact multiple sub-meshes, and there may be
            // multiple product sub-meshes for each source mesh, so a given morph target may be
            // split into multiple dispatches, and we use this index to track which mesh is associated
            // with which dispatch
            uint32_t productMeshIndex = 0;
            
            // Once per source-mesh, since productList is 1-1 with source mesh
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
                bool warnedExcessOfSkinInfluences = false;

                uint32_t totalVertexCountForThisSourceMesh = 0;
                for (const auto& it : productsByMaterialUid)
                {
                    ProductMeshContent productMesh;
                    productMesh.m_name = sourceMesh.m_name;

                    productMesh.m_materialUid = it.m_materialUid;

                    const FaceList& faceInfoList = it.m_faceList;
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
                    AZStd::vector<float>& clothData = productMesh.m_clothData;

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

                    const bool hasClothData = !sourceMesh.m_meshClothData.empty();
                    if (hasClothData)
                    {
                        AZ_Assert(sourceMesh.m_meshClothData.size() == vertexCount,
                            "Vertex Count %d does not match mesh cloth data size %d", vertexCount, sourceMesh.m_meshClothData.size());
                        clothData.reserve(vertexCount * ClothDataFloatsPerVert);
                    }

                    const bool hasSkinData = !sourceMesh.m_skinData.empty();
                    if (hasSkinData)
                    {
                        // Skinned meshes require that positions, normals, tangents, bitangents, all exist and have the same number
                        // of total elements. Pad buffers with missing data to make them align with positions and normals
                        if (!sourceMesh.m_meshTangents)
                        {
                            tangents.resize(vertexCount * TangentFloatsPerVert, 1.0f);
                            AZ_Warning(s_builderName, false, "Mesh '%s' is missing tangents and no defaults were generated. Skinned meshes require tangents. Dummy tangents will be inserted, which may result in rendering artifacts.", sourceMesh.m_name.GetCStr());
                        }
                        if (!sourceMesh.m_meshBitangents)
                        {
                            bitangents.resize(vertexCount * BitangentFloatsPerVert, 1.0f);
                            AZ_Warning(s_builderName, false, "Mesh '%s' is missing bitangents and no defaults were generated. Skinned meshes require bitangents. Dummy bitangents will be inserted, which may result in rendering artifacts.", sourceMesh.m_name.GetCStr());
                        }

                        productMesh.m_influencesPerVertex = CalculateMaxUsedSkinInfluencesPerVertex(
                            sourceMesh, oldToNewIndices, warnedExcessOfSkinInfluences);

                        const uint32_t totalInfluences = productMesh.m_influencesPerVertex * aznumeric_cast<uint32_t>(vertexCount);
                        productMesh.m_skinJointIndices.reserve(totalInfluences + CalculateJointIdPaddingCount(totalInfluences));
                        productMesh.m_skinWeights.reserve(totalInfluences);
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

                        pos = context.m_coordSysConverter.ConvertVector3(pos);

                        positions.push_back(pos.GetX());
                        positions.push_back(pos.GetY());
                        positions.push_back(pos.GetZ());

                        // Multiply normal by inverse transpose to avoid 
                        // incorrect values produced by non-uniformly scaled
                        // transforms.
                        normal = inverseTranspose.TransformVector(normal);
                        normal = context.m_coordSysConverter.ConvertVector3(normal);
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
                            tangent = context.m_coordSysConverter.ConvertVector3(tangent);
                            tangent.Normalize();

                            tangents.push_back(tangent.GetX());
                            tangents.push_back(tangent.GetY());
                            tangents.push_back(tangent.GetZ());
                            tangents.push_back(bitangentSign);

                            if (sourceMesh.m_meshBitangents)
                            {
                                AZ::Vector3 bitangent = sourceMesh.m_meshBitangents->GetBitangent(oldIndex);

                                bitangent = meshTransform.TransformVector(bitangent);
                                bitangent = context.m_coordSysConverter.ConvertVector3(bitangent);
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

                        // Gather Cloth Data
                        if (hasClothData)
                        {
                            const AZ::Color& vertexClothData = sourceMesh.m_meshClothData[oldIndex];

                            clothData.push_back(vertexClothData.GetR());
                            clothData.push_back(vertexClothData.GetG());
                            clothData.push_back(vertexClothData.GetB());
                            clothData.push_back(vertexClothData.GetA());
                        }

                        // Gather skinning influences
                        if (hasSkinData)
                        {
                            // Warn about excess of skin influences once per-source mesh.
                            GatherVertexSkinningInfluences(sourceMesh, productMesh, jointNameToIndexMap, oldIndex);
                        }
                    }// for each vertex in old to new indices

                    // A morph target that only influenced one source mesh might be split over multiple product meshes
                    // if the source mesh had multiple materials and was split up.
                    // So here, we need to know the start and end indices of the current product mesh within the original source
                    // mesh, so that when we process a morph target on the source mesh, we can ignore it if it doesn't impact the
                    // current product mesh and we can include it if it does. Furthermore, this leads to a 1:N relationship between
                    // morph target animations and actual morph target dispatches
                    morphTargetExporter.ProduceMorphTargets(
                        productMeshIndex, totalVertexCountForThisSourceMesh, oldToNewIndices, context.m_scene, sourceMesh, productMesh,
                        morphTargetMetaCreator, context.m_coordSysConverter);
                    productMeshIndex++;
                    totalVertexCountForThisSourceMesh += static_cast<uint32_t>(vertexCount);

                    productMeshList.emplace_back(productMesh);

                }// for each product mesh in productsByMaterialUid
            }// for each product in productList (for each source mesh)

            return productMeshList;
        }

        void ModelAssetBuilderComponent::PadVerticesForSkinning(ProductMeshContentList& productMeshList)
        {
            // Check if this is a skinned mesh
            if (!productMeshList.empty() && !productMeshList[0].m_skinWeights.empty())
            {
                for (ProductMeshContent& productMesh : productMeshList)
                {
                    size_t vertexCount = productMesh.m_positions.size() / PositionFloatsPerVert;

                    // Skinned meshes require that positions, normals, tangents, bitangents, all exist and have the same number
                    // of total elements. Pad buffers with missing data to make them align with positions and normals
                    if (productMesh.m_tangents.empty())
                    {
                        productMesh.m_tangents.resize(vertexCount * TangentFloatsPerVert, 1.0f);
                        AZ_Warning(s_builderName, false, "Mesh '%s' is missing tangents and no defaults were generated. Skinned meshes require tangents. Dummy tangents will be inserted, which may result in rendering artifacts.", productMesh.m_name.GetCStr());
                    }
                    if (productMesh.m_bitangents.empty())
                    {
                        productMesh.m_bitangents.resize(vertexCount * BitangentFloatsPerVert, 1.0f);
                        AZ_Warning(s_builderName, false, "Mesh '%s' is missing bitangents and no defaults were generated. Skinned meshes require bitangents. Dummy bitangents will be inserted, which may result in rendering artifacts.", productMesh.m_name.GetCStr());
                    }
                }
            }
        }

        uint32_t ModelAssetBuilderComponent::CalculateMaxUsedSkinInfluencesPerVertex(
            const SourceMeshContent& sourceMesh,
            const AZStd::map<uint32_t, uint32_t>& oldToNewIndicesMap,
            bool& warnedExcessOfSkinInfluences) const
        {
            uint32_t influencesPerVertex = 0;
            for (const auto& [oldIndex, newIndex] : oldToNewIndicesMap)
            {
                uint32_t influenceCountForCurrentVertex = 0;
                for (const auto& skinData : sourceMesh.m_skinData)
                {
                    const size_t numSkinInfluences = skinData->GetLinkCount(oldIndex);

                    // Check all the links and add any with a weight over the threshold to the running count
                    for (size_t influenceIndex = 0; influenceIndex < numSkinInfluences; ++influenceIndex)
                    {
                        const AZ::SceneAPI::DataTypes::ISkinWeightData::Link& link = skinData->GetLink(oldIndex, influenceIndex);

                        const float weight = link.weight;

                        if (weight > m_skinRuleSettings.m_weightThreshold)
                        {
                            ++influenceCountForCurrentVertex;
                        }
                    }
                }
                influencesPerVertex = AZStd::max(influencesPerVertex, influenceCountForCurrentVertex);
            }

            if (influencesPerVertex > m_skinRuleSettings.m_maxInfluencesPerVertex)
            {
                AZ_Warning(
                    s_builderName, warnedExcessOfSkinInfluences,
                    "Mesh %s has more skin influences (%d) than the maximum (%d). Skinning influences won't be normalized. "
                    "It's also not guaranteed that the excess skin influences that are cut off will be the lowest weight influences. "
                    "Maximum number of skin influences can be increased with a Skin Modifier in Scene Settings.",
                    sourceMesh.m_name.GetCStr(), influencesPerVertex,
                    m_skinRuleSettings.m_maxInfluencesPerVertex);
                warnedExcessOfSkinInfluences = true;
            }

            influencesPerVertex = AZStd::min(influencesPerVertex, m_skinRuleSettings.m_maxInfluencesPerVertex);

            // Round up to a multiple of two, since influences are processed two at a time in the shader
            return AZ::RoundUpToMultiple(influencesPerVertex, 2u);
        }

        void ModelAssetBuilderComponent::GatherVertexSkinningInfluences(
            const SourceMeshContent& sourceMesh,
            ProductMeshContent& productMesh,
            AZStd::unordered_map<AZStd::string, uint16_t>& jointNameToIndexMap,
            size_t vertexIndex) const
        {
            AZStd::vector<uint16_t>& skinJointIndices = productMesh.m_skinJointIndices;
            AZStd::vector<float>& skinWeights = productMesh.m_skinWeights;

            size_t numInfluencesAdded = 0;
            for (const auto& skinData : sourceMesh.m_skinData)
            {
                const size_t numSkinInfluences = skinData->GetLinkCount(vertexIndex);

                for (size_t influenceIndex = 0; influenceIndex < numSkinInfluences; ++influenceIndex)
                {
                    const AZ::SceneAPI::DataTypes::ISkinWeightData::Link& link = skinData->GetLink(vertexIndex, influenceIndex);

                    const float weight = link.weight;
                    const AZStd::string& boneName = skinData->GetBoneName(link.boneId);

                    // The bone id is a local bone id to the mesh. Since there could be multiple meshes, we store a global index to this asset,
                    // which is guaranteed to be unique. Later we will translate those indices back using the skinmetadata.
                    if (!jointNameToIndexMap.contains(boneName))
                    {
                        jointNameToIndexMap[boneName] = aznumeric_caster(jointNameToIndexMap.size());
                    }
                    const AZ::u16 jointIndex = jointNameToIndexMap[boneName];

                    // Add skin influence
                    if (weight > m_skinRuleSettings.m_weightThreshold)
                    {
                        if (numInfluencesAdded < productMesh.m_influencesPerVertex)
                        {
                            skinJointIndices.push_back(jointIndex);
                            skinWeights.push_back(weight);
                            numInfluencesAdded++;
                        }
                    }
                }
            }

            for (size_t influenceIndex = numInfluencesAdded; influenceIndex < productMesh.m_influencesPerVertex; ++influenceIndex)
            {
                skinJointIndices.push_back(0);
                skinWeights.push_back(0.0f);
            }
        }

        ModelAssetBuilderComponent::ProductMeshContentList ModelAssetBuilderComponent::MergeMeshesByMaterialUid(const ProductMeshContentList& productMeshList)
        {
            ProductMeshContentList finalMeshList;
            {
                AZStd::unordered_map<MaterialUid, ProductMeshContentList> meshesByMatUid;

                // First pass to reserve memory
                // This saves time with very large meshes
                {
                    AZStd::unordered_map<MaterialUid, size_t> meshCountByMatUid;

                    for (const ProductMeshContent& mesh : productMeshList)
                    {
                        if (mesh.CanBeMerged())
                        {
                            meshCountByMatUid[mesh.m_materialUid]++;
                        }
                    }

                    for (const auto& it : meshCountByMatUid)
                    {
                        meshesByMatUid[it.first].reserve(it.second);
                    }
                }

                size_t unmergeableMeshCount = 0;
                for (const ProductMeshContent& mesh : productMeshList)
                {
                    if (mesh.CanBeMerged())
                    {
                        meshesByMatUid[mesh.m_materialUid].push_back(mesh);
                    }
                    else
                    {
                        unmergeableMeshCount++;
                    }
                }

                const size_t mergedMeshCount = meshesByMatUid.size();
                finalMeshList.reserve(mergedMeshCount + unmergeableMeshCount);

                // Add the merged meshes
                for (const auto& it : meshesByMatUid)
                {
                    const ProductMeshContentList& meshList = it.second;

                    ProductMeshContent mergedMesh = MergeMeshList(meshList, RemapIndices);
                    mergedMesh.m_materialUid = it.first;
                    ValidateStreamAlignment(mergedMesh);

                    finalMeshList.emplace_back(AZStd::move(mergedMesh));
                }

                // Add the unmergeable meshes
                for (const ProductMeshContent& mesh : productMeshList)
                {
                    if (!mesh.CanBeMerged())
                    {
                        ValidateStreamAlignment(mesh);
                        finalMeshList.emplace_back(mesh);
                    }
                }
            }

            return finalMeshList;
        }

        template<typename T>
        void ModelAssetBuilderComponent::ValidateStreamSize([[maybe_unused]] size_t expectedVertexCount, [[maybe_unused]] const AZStd::vector<T>& bufferData, [[maybe_unused]] AZ::RHI::Format format, [[maybe_unused]] const char* streamName) const
        {
#if defined(AZ_ENABLE_TRACING)
            size_t actualVertexCount = (bufferData.size() * sizeof(T)) / RHI::GetFormatSize(format);
#endif
            AZ_Error(s_builderName, expectedVertexCount == actualVertexCount, "VertexStream '%s' does not match the expected vertex count. This typically means multiple sub-meshes have mis-matched vertex stream layouts (such as one having more uv sets than the other) but are assigned the same material in the dcc tool so they were merged.", streamName);
        }

        void ModelAssetBuilderComponent::ValidateStreamAlignment(const ProductMeshContent& mesh) const
        {
            size_t expectedVertexCount = mesh.m_positions.size() * sizeof(mesh.m_positions[0]) / RHI::GetFormatSize(PositionFormat);
            if (!mesh.m_normals.empty())
            {
                ValidateStreamSize(expectedVertexCount, mesh.m_normals, NormalFormat, "NORMAL");
            }
            if (!mesh.m_tangents.empty())
            {
                ValidateStreamSize(expectedVertexCount, mesh.m_tangents, TangentFormat, "TANGENT");
            }
            if (!mesh.m_bitangents.empty())
            {
                ValidateStreamSize(expectedVertexCount, mesh.m_bitangents, BitangentFormat, "BITANGENT");
            }
            for (size_t i = 0; i < mesh.m_uvSets.size(); ++i)
            {
                ValidateStreamSize(expectedVertexCount, mesh.m_uvSets[i], UVFormat, mesh.m_uvCustomNames[i].GetCStr());
            }
            for (size_t i = 0; i < mesh.m_colorSets.size(); ++i)
            {
                ValidateStreamSize(expectedVertexCount, mesh.m_colorSets[i], ColorFormat, mesh.m_colorCustomNames[i].GetCStr());
            }
            if (!mesh.m_clothData.empty())
            {
                ValidateStreamSize(expectedVertexCount, mesh.m_clothData, ClothDataFormat, ShaderSemanticName_ClothData);
            }
            if (!mesh.m_skinJointIndices.empty())
            {
                ValidateStreamSize(expectedVertexCount * mesh.m_influencesPerVertex, mesh.m_skinJointIndices, AZ::RHI::Format::R16_UINT, ShaderSemanticName_SkinJointIndices);
            }
            if (!mesh.m_skinWeights.empty())
            {
                ValidateStreamSize(expectedVertexCount * mesh.m_influencesPerVertex, mesh.m_skinWeights, SkinWeightFormat, ShaderSemanticName_SkinWeights);
            }
        }

        ModelAssetBuilderComponent::ProductMeshView ModelAssetBuilderComponent::CreateViewToEntireMesh(const ProductMeshContent& mesh)
        {
            ProductMeshView meshView;
            meshView.m_name = mesh.m_name.GetStringView();

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

            const size_t uvSetCount = mesh.m_uvSets.size();
            meshView.m_uvSetViews.reserve(uvSetCount);
            meshView.m_uvCustomNames.resize(uvSetCount);
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

            if (!mesh.m_skinJointIndices.empty() && !mesh.m_skinWeights.empty())
            {
                const size_t numSkinInfluences = mesh.m_skinWeights.size();

                uint32_t jointIndicesSizeInBytes = static_cast<uint32_t>(numSkinInfluences * sizeof(uint16_t));
                meshView.m_skinJointIndicesView = RHI::BufferViewDescriptor::CreateRaw(0, jointIndicesSizeInBytes);
                meshView.m_skinWeightsView = RHI::BufferViewDescriptor::CreateTyped(0, static_cast<uint32_t>(numSkinInfluences), SkinWeightFormat);
            }

            if (!mesh.m_morphTargetVertexData.empty())
            {
                const size_t numTotalVertices = mesh.m_morphTargetVertexData.size();
                meshView.m_morphTargetVertexDataView = RHI::BufferViewDescriptor::CreateStructured(0, static_cast<uint32_t>(numTotalVertices), sizeof(PackedCompressedMorphTargetDelta));
            }

            if (!mesh.m_clothData.empty())
            {
                auto meshClothDataFloatCount = static_cast<uint32_t>(mesh.m_clothData.size());
                AZ_Assert((meshClothDataFloatCount % ClothDataFloatsPerVert) == 0,
                    "Unexpected number of cloth data elements (%d), it should contain a multiple of %d elements.", meshClothDataFloatCount, ClothDataFloatsPerVert);

                auto meshClothDataCount = meshClothDataFloatCount / ClothDataFloatsPerVert;
                AZ_Assert(meshClothDataCount == meshPositionCount,
                    "Number of cloth data elements (%d) does not match the number of positions (%d) in the mesh", meshClothDataCount, meshPositionCount);

                meshView.m_clothDataView = RHI::BufferViewDescriptor::CreateTyped(0, meshClothDataCount, ClothDataFormat);
            }

            meshView.m_materialUid = mesh.m_materialUid;

            return meshView;
        }

        void ModelAssetBuilderComponent::MergeMeshesToCommonBuffers(
            ProductMeshContentList& lodMeshList,
            ProductMeshContent& lodMeshContent,
            ProductMeshViewList& meshViews)
        {
            meshViews.reserve(lodMeshList.size());

            // We want to merge these meshes into one large 
            // ProductMesh. That large buffer gets set on the LOD directly
            // rather than a Mesh in the LOD.
            ProductMeshContentAllocInfo lodBufferInfo;

            bool isFirstMesh = true;
            for (ProductMeshContent& mesh : lodMeshList)
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
                auto meshClothDataFloatCount = static_cast<uint32_t>(mesh.m_clothData.size());

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

                if (!mesh.m_clothData.empty())
                {
                    const uint32_t elementOffset = static_cast<uint32_t>(lodBufferInfo.m_clothDataFloatCount) / ClothDataFloatsPerVert;
                    meshView.m_clothDataView = RHI::BufferViewDescriptor::CreateTyped(elementOffset, meshVertexCount, ClothDataFormat);
                    lodBufferInfo.m_clothDataFloatCount += meshClothDataFloatCount;
                }

                meshView.m_materialUid = mesh.m_materialUid;

                if (!mesh.m_skinJointIndices.empty() && !mesh.m_skinWeights.empty())
                {
                    if (!isFirstMesh && lodBufferInfo.m_jointIdsCount == 0)
                    {
                        AZ_Error(
                            s_builderName, false,
                            "Attempting to merge a mix of static and skinned meshes, this will fail on buffer generation later. Mesh with "
                            "name %s is skinned, but previous meshes were not skinned.",
                            mesh.m_name.GetCStr());
                    }
                    AZ_Assert(mesh.m_skinJointIndices.size() == mesh.m_skinWeights.size(),
                        "Number of skin influence joint indices (%d) should match the number of weights (%d).",
                        mesh.m_skinJointIndices.size(), mesh.m_skinWeights.size());

                    AZ_Assert(mesh.m_skinWeights.size() % mesh.m_influencesPerVertex == 0,
                        "The number of skin influences per vertex (%d) is not a multiple of the total number of skinning weights (%d). This means that not every vertex has exactly (%d) skinning weights and invalidates the data.",
                        mesh.m_skinWeights.size(), mesh.m_influencesPerVertex, mesh.m_influencesPerVertex);

                    uint32_t prevJointIdCount = aznumeric_cast<uint32_t>(lodBufferInfo.m_jointIdsCount);
                    uint32_t newJointIdCount = aznumeric_cast<uint32_t>(mesh.m_skinJointIndices.size());

                    // Pad the joint id buffer if it ends too soon, so the next view can start aligned
                    uint32_t extraIdCount = CalculateJointIdPaddingCount(newJointIdCount);

                    // Pad the buffer
                    AZStd::vector<uint16_t> extraIds(extraIdCount, 0);
                    mesh.m_skinJointIndices.insert(
                        mesh.m_skinJointIndices.end(), extraIds.begin(), extraIds.end());

                    AZ_Assert(prevJointIdCount * sizeof(uint16_t) % 16 == 0, "Failed to align the joint id offset along a 16-byte boundary");

                    // For the view itself, we only want a view that includes the real ids, not the padding, so use newJointIdCount
                    meshView.m_skinJointIndicesView = RHI::BufferViewDescriptor::CreateRaw(
                        /*byteOffset=*/prevJointIdCount * sizeof(uint16_t),
                        /*byteCount*/ newJointIdCount * sizeof(uint16_t));

                    // For the purpose of tracking the size of the buffer, include the padding
                    lodBufferInfo.m_jointIdsCount += (newJointIdCount + extraIdCount);

                    // Weights are more straightforward, just add any new weights
                    const uint32_t prevJointWeightCount = aznumeric_cast<uint32_t>(lodBufferInfo.m_jointWeightsCount);
                    const uint32_t newJointWeightCount = aznumeric_cast<uint32_t>(mesh.m_skinWeights.size());
                    meshView.m_skinWeightsView = RHI::BufferViewDescriptor::CreateTyped(/*elementOffset=*/prevJointWeightCount, newJointWeightCount, SkinWeightFormat);
                    lodBufferInfo.m_jointWeightsCount += newJointWeightCount;
                }
                else if (lodBufferInfo.m_jointIdsCount > 0)
                {
                    AZ_Error(s_builderName, false, "Attempting to merge a mix of static and skinned meshes, this will fail on buffer generation later. Mesh with name %s is not skinned, but previous meshes were skinned.",
                        mesh.m_name.GetCStr());
                }

                if (!mesh.m_morphTargetVertexData.empty())
                {
                    const size_t numPrevVertexDeltas = lodBufferInfo.m_morphTargetVertexDeltaCount;
                    const size_t numNewVertexDeltas = mesh.m_morphTargetVertexData.size();

                    meshView.m_morphTargetVertexDataView = RHI::BufferViewDescriptor::CreateStructured(/*elementOffset=*/ static_cast<uint32_t>(numPrevVertexDeltas), static_cast<uint32_t>(numNewVertexDeltas), sizeof(PackedCompressedMorphTargetDelta));

                    lodBufferInfo.m_morphTargetVertexDeltaCount += numNewVertexDeltas;
                }

                meshViews.emplace_back(AZStd::move(meshView));
                isFirstMesh = false;
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
                size_t clothDataCount = 0;
                AZStd::vector<size_t> uvSetCounts;
                AZStd::vector<size_t> colorSetCounts;

                for (const ProductMeshContent& mesh : productMeshList)
                {
                    indexCount += mesh.m_indices.size();
                    positionCount += mesh.m_positions.size();
                    normalCount += mesh.m_normals.size();
                    tangentCount += mesh.m_tangents.size();
                    bitangentCount += mesh.m_bitangents.size();
                    clothDataCount += mesh.m_clothData.size();

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
                mergedMesh.m_clothData.reserve(clothDataCount);

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

                if (!mesh.m_skinJointIndices.empty())
                {
                    mergedMesh.m_skinJointIndices.insert(
                        mergedMesh.m_skinJointIndices.end(), mesh.m_skinJointIndices.begin(), mesh.m_skinJointIndices.end());
                }

                if (!mesh.m_skinWeights.empty())
                {
                    mergedMesh.m_skinWeights.insert(
                        mergedMesh.m_skinWeights.end(), mesh.m_skinWeights.begin(), mesh.m_skinWeights.end());
                }

                if (!mesh.m_morphTargetVertexData.empty())
                {
                    const auto& sourceMorphTargetData = mesh.m_morphTargetVertexData;
                    auto& mergedMorphTargetData = mergedMesh.m_morphTargetVertexData;
                    mergedMorphTargetData.insert(mergedMorphTargetData.end(), sourceMorphTargetData.begin(), sourceMorphTargetData.end());
                }

                if (!mesh.m_clothData.empty())
                {
                    mergedMesh.m_clothData.insert(
                        mergedMesh.m_clothData.end(), mesh.m_clothData.begin(), mesh.m_clothData.end());
                }
            }

            return mergedMesh;
        }

        template<typename T>
        bool ModelAssetBuilderComponent::BuildStructuredStreamBuffer(
            AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& outStreamBuffers,
            const AZStd::vector<T>& bufferData,
            const RHI::ShaderSemantic& semantic,
            const AZ::Name& customStreamName)
        {
            AZStd::string bufferName = semantic.ToString();
            size_t elementCount = bufferData.size();
            size_t elementSize = sizeof(T);
            Outcome<Data::Asset<BufferAsset>> bufferOutcome = CreateStructuredBufferAsset(bufferData.data(), elementCount, elementSize, bufferName);

            if (!bufferOutcome.IsSuccess())
            {
                AZ_Error(s_builderName, false, "Failed to build %s stream", semantic.ToString().data());
                return false;
            }

            outStreamBuffers.push_back({ semantic, customStreamName, {bufferOutcome.GetValue(), bufferOutcome.GetValue()->GetBufferViewDescriptor()} });
            return true;
        };

        template<typename T>
        bool ModelAssetBuilderComponent::BuildRawStreamBuffer(
            AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& outStreamBuffers,
            const AZStd::vector<T>& bufferData,
            const RHI::ShaderSemantic& semantic,
            const AZ::Name& customStreamName)
        {
            AZStd::string bufferName = semantic.ToString();
            size_t sizeInBytes = bufferData.size() * sizeof(T);
            Outcome<Data::Asset<BufferAsset>> bufferOutcome = CreateRawBufferAsset(bufferData.data(), sizeInBytes, bufferName);

            if (!bufferOutcome.IsSuccess())
            {
                AZ_Error(s_builderName, false, "Failed to build %s stream", semantic.ToString().data());
                return false;
            }

            outStreamBuffers.push_back({ semantic, customStreamName, {bufferOutcome.GetValue(), bufferOutcome.GetValue()->GetBufferViewDescriptor()} });
            return true;
        };

        template<typename T>
        bool ModelAssetBuilderComponent::BuildTypedStreamBuffer(
            AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& outStreamBuffers,
            const AZStd::vector<T>& bufferData,
            AZ::RHI::Format format,
            const RHI::ShaderSemantic& semantic,
            const AZ::Name& customStreamName)
        {
            AZStd::string bufferName = semantic.ToString();
            size_t floatsPerElement = RHI::GetFormatSize(format) / sizeof(T);
            Outcome<Data::Asset<BufferAsset>> bufferOutcome = CreateTypedBufferAsset(bufferData.data(), bufferData.size() / floatsPerElement, format, bufferName);

            if (!bufferOutcome.IsSuccess())
            {
                AZ_Error(s_builderName, false, "Failed to build %s stream", semantic.ToString().data());
                return false;
            }

            outStreamBuffers.push_back({semantic, customStreamName, {bufferOutcome.GetValue(), bufferOutcome.GetValue()->GetBufferViewDescriptor()}});
            return true;
        };

        template<typename T>
        bool ModelAssetBuilderComponent::BuildStreamBuffer(size_t vertexCount,
            AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& outStreamBuffers,
            const AZStd::vector<T>& bufferData,
            AZ::RHI::Format format,
            const RHI::ShaderSemantic& semantic,
            const AZ::Name& customStreamName)
        {
            size_t expectedElementCount = vertexCount * RHI::GetFormatComponentCount(format);
            if (expectedElementCount != bufferData.size())
            {
                AZ_Error(s_builderName, false, "Failed to build %s stream. Expected %d elements but found %d.", semantic.ToString().data(), expectedElementCount, bufferData.size());
                return false;
            }

            AZStd::string bufferName = semantic.ToString();
            Outcome<Data::Asset<BufferAsset>> bufferOutcome = CreateTypedBufferAsset(bufferData.data(), vertexCount, format, bufferName);
            if (!bufferOutcome.IsSuccess())
            {
                AZ_Error(s_builderName, false, "Failed to build %s stream", semantic.ToString().data());
                return false;
            }

            outStreamBuffers.push_back({semantic, customStreamName, {bufferOutcome.GetValue(), bufferOutcome.GetValue()->GetBufferViewDescriptor()}});
            return true;
        };

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
            const AZStd::vector<float>& clothData = lodBufferContent.m_clothData;

            // Build Index Buffer ...
            {
                Outcome<Data::Asset<BufferAsset>> indexBufferOutcome = CreateTypedBufferAsset(indices.data(), indices.size(), IndicesFormat, "index");
                if (!indexBufferOutcome.IsSuccess())
                {
                    AZ_Error(s_builderName, false, "Failed to build index stream");
                    return false;
                }

                outIndexBuffer = { indexBufferOutcome.GetValue(), indexBufferOutcome.GetValue()->GetBufferViewDescriptor() };
            }

            // Build various stream buffers ...
            if (!BuildTypedStreamBuffer<float>(outStreamBuffers, positions, PositionFormat, RHI::ShaderSemantic{"POSITION"}))
            {
                return false;
            }

            if (!BuildTypedStreamBuffer<float>(outStreamBuffers, normals, NormalFormat, RHI::ShaderSemantic{"NORMAL"}))
            {
                return false;
            }

            if (!tangents.empty())
            {
                if (!BuildTypedStreamBuffer<float>(outStreamBuffers, tangents, TangentFormat, RHI::ShaderSemantic{"TANGENT"}))
                {
                    return false;
                }
            }

            if (!bitangents.empty())
            {
                if (!BuildTypedStreamBuffer<float>(outStreamBuffers, bitangents, BitangentFormat, RHI::ShaderSemantic{"BITANGENT"}))
                {
                    return false;
                }
            }
            
            for (size_t i = 0; i < uvSets.size(); ++i)
            {
                if (!BuildTypedStreamBuffer<float>(outStreamBuffers, uvSets[i], UVFormat, RHI::ShaderSemantic{"UV", i}, uvCustomNames[i]))
                {
                    return false;
                }
            }

            for (size_t i = 0; i < colorSets.size(); ++i)
            {
                if (!BuildTypedStreamBuffer<float>(outStreamBuffers, colorSets[i], ColorFormat, RHI::ShaderSemantic{"COLOR", i}, colorCustomNames[i]))
                {
                    return false;
                }
            }

            // Skinning buffers
            const AZStd::vector<uint16_t>& skinJointIndices = lodBufferContent.m_skinJointIndices;
            const AZStd::vector<float>& skinWeights = lodBufferContent.m_skinWeights;
            if (!skinJointIndices.empty() && !skinWeights.empty())
            {
                if (!BuildRawStreamBuffer<uint16_t>(outStreamBuffers, skinJointIndices, RHI::ShaderSemantic{ShaderSemanticName_SkinJointIndices}))
                {
                    return false;
                }

                if (!BuildStreamBuffer<float>(skinWeights.size(), outStreamBuffers, skinWeights, SkinWeightFormat, RHI::ShaderSemantic{ShaderSemanticName_SkinWeights}))
                {
                    return false;
                }
            }

            // Morph target buffers
            const AZStd::vector<PackedCompressedMorphTargetDelta>& morphTargetVertexDeltas = lodBufferContent.m_morphTargetVertexData;
            if (!morphTargetVertexDeltas.empty())
            {
                if (!BuildStructuredStreamBuffer<PackedCompressedMorphTargetDelta>(outStreamBuffers, morphTargetVertexDeltas,
                    RHI::ShaderSemantic{ ShaderSemanticName_MorphTargetDeltas }))
                {
                    return false;
                }
            }
            
            if (!clothData.empty())
            {
                if (!BuildTypedStreamBuffer<float>(outStreamBuffers, clothData, ClothDataFormat, RHI::ShaderSemantic{ ShaderSemanticName_ClothData }))
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
            ModelAssetCreator& modelAssetCreator,
            ModelLodAssetCreator& lodAssetCreator,
            const MaterialAssetsByUid& materialAssetsByUid)
        {
            lodAssetCreator.BeginMesh();
            
            if (meshView.m_materialUid != s_invalidMaterialUid)
            {
                auto iter = materialAssetsByUid.find(meshView.m_materialUid);
                if (iter != materialAssetsByUid.end())
                {
                    ModelMaterialSlot materialSlot;
                    materialSlot.m_stableId = static_cast<AZ::RPI::ModelMaterialSlot::StableId>(meshView.m_materialUid);
                    materialSlot.m_displayName = iter->second.m_name;
                    materialSlot.m_defaultMaterialAsset = iter->second.m_asset;

                    modelAssetCreator.AddMaterialSlot(materialSlot);
                    lodAssetCreator.SetMeshMaterialSlot(materialSlot.m_stableId);
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

            // Set skin buffers
            if (meshView.m_skinJointIndicesView.m_elementCount > 0 && meshView.m_skinWeightsView.m_elementCount > 0)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{ShaderSemanticName_SkinJointIndices}, AZ::Name(), meshView.m_skinJointIndicesView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }

                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{ShaderSemanticName_SkinWeights}, AZ::Name(), meshView.m_skinWeightsView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            // Set morph target buffers
            if (meshView.m_morphTargetVertexDataView.m_elementCount > 0)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{ShaderSemanticName_MorphTargetDeltas}, AZ::Name(),
                    meshView.m_morphTargetVertexDataView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            // Set cloth data buffer
            if (meshView.m_clothDataView.m_elementCount > 0)
            {
                if (!SetMeshStreamBufferById(RHI::ShaderSemantic{ ShaderSemanticName_ClothData }, AZ::Name(), meshView.m_clothDataView, lodStreamBuffers, lodAssetCreator))
                {
                    return false;
                }
            }

            lodAssetCreator.EndMesh();

            return true;
        }

        Outcome<Data::Asset<BufferAsset>> ModelAssetBuilderComponent::CreateTypedBufferAsset(
            const void* data, const size_t elementCount, RHI::Format format, const AZStd::string& bufferName)
        {
            RHI::BufferViewDescriptor bufferViewDescriptor =
                RHI::BufferViewDescriptor::CreateTyped(0, static_cast<uint32_t>(elementCount), format);

            return CreateBufferAsset(data, bufferViewDescriptor, bufferName);
        }

        Outcome<Data::Asset<BufferAsset>> ModelAssetBuilderComponent::CreateStructuredBufferAsset(
            const void* data, const size_t elementCount, const size_t elementSize, const AZStd::string& bufferName)
        {
            RHI::BufferViewDescriptor bufferViewDescriptor =
                RHI::BufferViewDescriptor::CreateStructured(0, static_cast<uint32_t>(elementCount), static_cast<uint32_t>(elementSize));

            return CreateBufferAsset(data, bufferViewDescriptor, bufferName);
        }

        Outcome<Data::Asset<BufferAsset>> ModelAssetBuilderComponent::CreateRawBufferAsset(
            const void* data, const size_t totalSizeInBytes, const AZStd::string& bufferName)
        {
            RHI::BufferViewDescriptor bufferViewDescriptor =
                RHI::BufferViewDescriptor::CreateRaw(0, static_cast<uint32_t>(totalSizeInBytes));

            return CreateBufferAsset(data, bufferViewDescriptor, bufferName);
        }

        Outcome<Data::Asset<BufferAsset>> ModelAssetBuilderComponent::CreateBufferAsset(
            const void* data, const RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName)
        {
            BufferAssetCreator creator;
            AZStd::string bufferAssetName = GetAssetFullName(BufferAsset::TYPEINFO_Uuid(), bufferName);
            creator.Begin(CreateAssetId(bufferAssetName));

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

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
                    vpos.Set(reinterpret_cast<const float*>(&buffer[i]));
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

        bool ModelAssetBuilderComponent::GetIsMorphed(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
        {
            // Note: In here we are checking directly in the scene graph. We are also suppose to check if user selected those morph target in blendshape rule, that work
            // will be done when the mesh group support blendshape rule.
            auto contentStorage = graph.GetContentStorage();
            auto downwardsView = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<AZ::SceneAPI::Containers::Views::BreadthFirst>(graph, nodeIndex, contentStorage.begin(), true);
            auto filteredView = AZ::SceneAPI::Containers::Views::MakeFilterView(downwardsView, AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IBlendShapeData>());
            return (filteredView.begin() != filteredView.end());
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
