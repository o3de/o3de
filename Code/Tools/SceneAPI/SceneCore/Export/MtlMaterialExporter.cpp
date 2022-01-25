/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <memory>
#include <SceneAPI/SceneCore/Export/MtlMaterialExporter.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <GFxFramework/MaterialIO/Material.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Export
        {
            //
            // BaseMaterialExporterComponent
            //

            Events::ProcessingResult BaseMaterialExporterComponent::ExportMaterialsToTempDir(Events::PreExportEventContext& context, bool registerProducts) const
            {
                using MaterialExporterMap = AZStd::unordered_map<AZStd::string, MtlMaterialExporter>;
                
                AZStd::string textureRootPath = GetTextureRootPath();
                AZ_TraceContext("Texture root", textureRootPath);

                Events::ProcessingResultCombiner result;
                MaterialExporterMap exporters;

                const Containers::SceneManifest& manifest = context.GetScene().GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<DataTypes::ISceneNodeGroup>(valueStorage);
                for (const DataTypes::ISceneNodeGroup& group : view)
                {
                    AZ_TraceContext("Group", group.GetName());

                    const AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainerConst();
                    AZStd::shared_ptr<const DataTypes::IMaterialRule> materialRule = rules.FindFirstByType<DataTypes::IMaterialRule>();
                    bool updateMaterial = materialRule ? materialRule->UpdateMaterials() : false;

                    // Look for a material file in the source directory, which is will be the canonical material to use. If there's none
                    //      then write one in the cache.
                    AZStd::string sourceMaterialPath = context.GetScene().GetSourceFilename();
                    AzFramework::StringFunc::Path::ReplaceExtension(sourceMaterialPath, GFxFramework::MaterialExport::g_mtlExtension);
                    AZ_TraceContext("Material source file path", sourceMaterialPath);
                    bool sourceFileExists = AZ::IO::SystemFile::Exists(sourceMaterialPath.c_str());

                    if (sourceFileExists && !updateMaterial)
                    {
                        // Don't write to the cache if there's a source material as this will be the primary material.
                        continue;
                    }

                    AZStd::string cacheFileName;
                    [[maybe_unused]] bool succeeded = AzFramework::StringFunc::Path::GetFileName(sourceMaterialPath.c_str(), cacheFileName);
                    AZ_Assert(succeeded, "Failed to retrieve a valid material file name from %s", sourceMaterialPath.c_str());

                    AZStd::string materialCachePath;
                    if (!AzFramework::StringFunc::Path::ConstructFull(context.GetOutputDirectory().c_str(), cacheFileName.c_str(),
                        GFxFramework::MaterialExport::g_dccMaterialExtension, materialCachePath, true))
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Failed to construct the full output path for the material.");
                        result += Events::ProcessingResult::Failure;
                        continue;
                    }
                    AZ_TraceContext("Material cache file path", materialCachePath);

                    MtlMaterialExporter::SaveMaterialResult materialResult;
                    auto exporter = exporters.find(materialCachePath);
                    if (exporter == exporters.end())
                    {
                        MtlMaterialExporter newExporter;
                        materialResult = newExporter.SaveMaterialGroup(group, context.GetScene(), textureRootPath);
                        if (materialResult == MtlMaterialExporter::SaveMaterialResult::Success)
                        {
                            exporters.insert(MaterialExporterMap::value_type(AZStd::move(materialCachePath), AZStd::move(newExporter)));
                        }
                    }
                    else
                    {
                        materialResult = exporter->second.AppendMaterialGroup(group, context.GetScene());
                    }

                    switch (materialResult)
                    {
                    case MtlMaterialExporter::SaveMaterialResult::Success:
                        result += Events::ProcessingResult::Success;
                        break;
                    case MtlMaterialExporter::SaveMaterialResult::Skipped:
                        break;
                    case MtlMaterialExporter::SaveMaterialResult::Failure:
                        result += Events::ProcessingResult::Failure;
                        break;
                    }
                }

                for (auto& entry : exporters)
                {
                    AZStd::string& materialCachePath = entry.first;
                    AZ_TraceContext("Material cache file path", materialCachePath);
                    MtlMaterialExporter& exporter = entry.second;
                    const bool update = false;   // No need to update with changes as the cache version will always be clean.
                    if (exporter.WriteToFile(materialCachePath.c_str(), update))
                    {
                        // Materials can belong to multiple groups, but they're currently still referenced by name in engine, so the ID doesn't really matter.
                        //      This is made worse due to the fact that once the material is moved from the cache to the source folder the source id also
                        //      changes. Since there's no good solution until the material update has completed, the hashed file name will have to do.
                        AZStd::string filename;
                        if (!AzFramework::StringFunc::Path::GetFileName(materialCachePath.c_str(), filename))
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "Failed to extract filename from material cache file path.");
                            result += Events::ProcessingResult::Failure;
                        }
                        else
                        {
                            if (registerProducts)
                            {
                                static const Data::AssetType dccMaterialAssetType("{C88469CF-21E7-41EB-96FD-BF14FBB05EDC}"); // from MaterialAsset.h
                                context.GetProductList().AddProduct(AZStd::move(entry.first), Uuid::CreateName(filename.c_str()), dccMaterialAssetType,
                                    AZStd::nullopt, AZStd::nullopt);
                            }
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Material file failed to write to cache.");
                        result += Events::ProcessingResult::Failure;
                    }
                }

                return result.GetResult();
            }

            AZStd::string BaseMaterialExporterComponent::GetTextureRootPath() const
            {
                using AzToolsFramework::AssetSystemRequestBus;

                AZ::IO::Path projectPath;
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    settingsRegistry->Get(projectPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
                }
                if (!projectPath.empty())
                {
                    return projectPath.Native();
                }
                else
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Unable to get determine game folder. Texture path may be invalid.");
                    return "";
                }
            }

            //
            // MaterialExporterComponent
            //
            MaterialExporterComponent::MaterialExporterComponent()
            {
                BindToCall(&MaterialExporterComponent::ExportMaterials);
            }

            Events::ProcessingResult MaterialExporterComponent::ExportMaterials(Events::PreExportEventContext& context) const
            {
                // Creates materials in the intermediate folder but doesn't register them as products with the Asset Processor.
                return BaseMaterialExporterComponent::ExportMaterialsToTempDir(context, false);
            }

            void MaterialExporterComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MaterialExporterComponent, SceneCore::ExportingComponent>()->Version(1);
                }
            }

            //
            // RCMaterialExporterComponent
            //
            RCMaterialExporterComponent::RCMaterialExporterComponent()
            {
                BindToCall(&RCMaterialExporterComponent::ExportMaterials);
            }

            Events::ProcessingResult RCMaterialExporterComponent::ExportMaterials(Events::PreExportEventContext& context) const
            {
                // Creates materials in the intermediate folder and registers them as products with the Asset Processor. This is done
                // for the ResourceCompilerScene as it has logic to deal with legacy issues such when RCScene ran without sub id generation.
                return BaseMaterialExporterComponent::ExportMaterialsToTempDir(context, true);
            }

            void RCMaterialExporterComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<RCMaterialExporterComponent, SceneCore::RCExportingComponent>()->Version(1);
                }
            }

            //
            // MtlMaterialExporter
            //
            
            MtlMaterialExporter::SaveMaterialResult MtlMaterialExporter::SaveMaterialGroup(const DataTypes::ISceneNodeGroup& sceneNodeGroup,
                const Containers::Scene& scene, const AZStd::string& textureRootPath)
            {
                m_textureRootPath = textureRootPath;
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, m_textureRootPath);
                return BuildMaterialGroup(sceneNodeGroup, scene);
            }

            bool MtlMaterialExporter::WriteToFile(const char* filePath, bool updateWithChanges)
            {
                if (AZ::IO::SystemFile::Exists(filePath) && !AZ::IO::SystemFile::IsWritable(filePath))
                {
                    return false;
                }
                return WriteMaterialFile(filePath, m_materialGroup, updateWithChanges);
            }

            MtlMaterialExporter::SaveMaterialResult MtlMaterialExporter::BuildMaterialGroup(const DataTypes::ISceneNodeGroup& sceneNodeGroup, const Containers::Scene& scene)
            {
                //Default rule settings for materials.
                m_materialGroup.m_materials.clear();
                m_materialGroup.m_removeMaterials = false;
                m_materialGroup.m_updateMaterials = false;

                const AZ::SceneAPI::Containers::RuleContainer& rules = sceneNodeGroup.GetRuleContainerConst();

                AZStd::shared_ptr<const DataTypes::IMaterialRule> materialRule = rules.FindFirstByType<DataTypes::IMaterialRule>();
                if (materialRule)
                {
                    m_materialGroup.m_removeMaterials = materialRule->RemoveUnusedMaterials();
                    m_materialGroup.m_updateMaterials = materialRule->UpdateMaterials();
                }

                return AppendMaterialGroup(sceneNodeGroup, scene);
            }
            
            MtlMaterialExporter::SaveMaterialResult MtlMaterialExporter::AppendMaterialGroup(const DataTypes::ISceneNodeGroup& sceneNodeGroup, const Containers::Scene& scene)
            {
                AZ_Assert(!m_textureRootPath.empty(), "Texture root path hasn't been set. Call SaveMaterialGroup before this function to setup the material first.");

                const Containers::SceneGraph& sceneGraph = scene.GetGraph();
                const AZ::SceneAPI::Containers::RuleContainer& rules = sceneNodeGroup.GetRuleContainerConst();

                int physicsMaterialFlags = 0;
                AZStd::vector<AZStd::string> targetNodes;
                for (auto& nodeName : targetNodes)
                {
                    auto index = sceneGraph.Find(nodeName);
                    //if we find any valid nodes add a MaterialInfo and stop.
                    if (index.IsValid())
                    {
                        MaterialInfo info;
                        info.m_name = GFxFramework::MaterialExport::g_stringPhysicsNoDraw;
                        info.m_materialData = nullptr;
                        info.m_usesVertexColoring = false;
                        info.m_physicsMaterialFlags = physicsMaterialFlags;
                        m_materialGroup.m_materials.push_back(info);
                        break;
                    }
                }

                //If we have a material rule process materials
                if (rules.FindFirstByType<DataTypes::IMaterialRule>())
                {
                    // create materials for render nodes
                    AZStd::vector<AZStd::string> renderTargetNodes = Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph, 
                        sceneNodeGroup.GetSceneNodeSelectionList(), Utilities::SceneGraphSelector::IsMesh);
                    for (auto& nodeName : renderTargetNodes)
                    {
                        Containers::SceneGraph::NodeIndex index = sceneGraph.Find(nodeName);
                        if (index.IsValid())
                        {
                            auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(sceneGraph, index,
                                sceneGraph.GetContentStorage().begin(), true);
                            for (auto it = view.begin(); it != view.end(); ++it)
                            {
                                if ((*it) && (*it)->RTTI_IsTypeOf(DataTypes::IMaterialData::TYPEINFO_Uuid()))
                                {
                                    const Containers::SceneGraph::Name& name =
                                        sceneGraph.GetNodeName(sceneGraph.ConvertToNodeIndex(it.GetHierarchyIterator()));
                                    auto material = AZStd::find_if(m_materialGroup.m_materials.begin(), m_materialGroup.m_materials.end(),
                                        [&name](const MaterialInfo& info) -> bool
                                        {
                                            return info.m_name == name.GetName();
                                        });
                                    if (material == m_materialGroup.m_materials.end())
                                    {
                                        AZStd::string nodeName2 = name.GetName();
                                        MaterialInfo info;
                                        info.m_name = nodeName2;
                                        info.m_materialData = azrtti_cast<const DataTypes::IMaterialData*>(*it);
                                        info.m_usesVertexColoring = UsesVertexColoring(sceneNodeGroup, scene, it.GetHierarchyIterator());
                                        info.m_physicsMaterialFlags = 0;
                                        m_materialGroup.m_materials.push_back(info);
                                    }
                                }
                            }
                        }
                    }
                    // create materials for LOD nodes
                    AZStd::shared_ptr<const DataTypes::ILodRule> lodRule = rules.FindFirstByType<DataTypes::ILodRule>();
                    if (lodRule)
                    {
                        for (int lodIndex = 0; lodIndex < lodRule->GetLodCount(); ++lodIndex)
                        {
                            auto& lodSceneNodeList = lodRule->GetSceneNodeSelectionList(lodIndex);

                            AZStd::vector<AZStd::string> lodNodes = 
                                Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph, lodSceneNodeList, Utilities::SceneGraphSelector::IsMesh);

                            for (const AZStd::string& nodeName : lodNodes)
                            {
                                Containers::SceneGraph::NodeIndex index = sceneGraph.Find(nodeName);
                                if (!index.IsValid())
                                {
                                    continue;
                                }
                                    
                                auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(sceneGraph, index,
                                    sceneGraph.GetContentStorage().begin(), true);
                                for (auto it = view.begin(); it != view.end(); ++it)
                                {
                                    if ((*it) && (*it)->RTTI_IsTypeOf(DataTypes::IMaterialData::TYPEINFO_Uuid()))
                                    {
                                        const Containers::SceneGraph::Name& name =
                                            sceneGraph.GetNodeName(sceneGraph.ConvertToNodeIndex(it.GetHierarchyIterator()));
                                            
                                        auto material = AZStd::find_if(
                                            m_materialGroup.m_materials.begin(), 
                                            m_materialGroup.m_materials.end(), 
                                            [&name](const MaterialInfo& info) -> bool
                                        {
                                            return info.m_name == name.GetName();
                                        });

                                        if (material == m_materialGroup.m_materials.end())
                                        {
                                            AZStd::string nodeName2 = name.GetName();
                                            MaterialInfo info;
                                            info.m_name = nodeName2;
                                            info.m_materialData = azrtti_cast<const DataTypes::IMaterialData*>(*it);
                                            info.m_usesVertexColoring = UsesVertexColoring(sceneNodeGroup, scene, it.GetHierarchyIterator());
                                            info.m_physicsMaterialFlags = 0;
                                            m_materialGroup.m_materials.push_back(info);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                return m_materialGroup.m_materials.size() == 0 ? SaveMaterialResult::Skipped : SaveMaterialResult::Success;
            }


            // Check if there's a mesh advanced rule of the given scene node group that
            //      specifically controls vertex coloring. If no rule exists for group, check if there are any
            //      vertex color streams, which would automatically enable the vertex coloring feature.
            bool MtlMaterialExporter::UsesVertexColoring(const DataTypes::ISceneNodeGroup& sceneNodeGroup, const Containers::Scene& scene,
                Containers::SceneGraph::HierarchyStorageConstIterator materialNode) const
            {
                const Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NodeIndex meshNodeIndex = graph.GetNodeParent(graph.ConvertToNodeIndex(materialNode));

                AZStd::shared_ptr<const DataTypes::IMeshAdvancedRule> rule = sceneNodeGroup.GetRuleContainerConst().FindFirstByType<DataTypes::IMeshAdvancedRule>();
                if (rule)
                {
                    return !rule->IsVertexColorStreamDisabled() && !rule->GetVertexColorStreamName().empty();
                }
                else
                {
                    if (DoesMeshNodeHaveColorStreamChild(scene, meshNodeIndex))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool MtlMaterialExporter::WriteMaterialFile(const char* filePath, MaterialGroup& materialGroup, bool updateWithChanges) const
            {
                if (materialGroup.m_materials.size() == 0)
                {
                    // Nothing to write to return true.
                    return true;
                }

                bool update = materialGroup.m_updateMaterials;
                bool cleanUp = materialGroup.m_removeMaterials;

                AZ::GFxFramework::MaterialGroup matGroup;
                AZ::GFxFramework::MaterialGroup doNotRemoveGroup;

                if (updateWithChanges)
                {
                    //Open the MTL file for read if it exists 
                    bool fileRead = matGroup.ReadMtlFile(filePath);
                    AZ_TraceContext("MTL File Name", filePath);
                    if (fileRead)
                    {
                        AZ_TracePrintf(Utilities::LogWindow, "MTL File found and will be updated as needed");
                    }
                    else
                    {
                        AZ_TracePrintf(Utilities::LogWindow, "No existing MTL file found. A new one will be generated.");
                    }
                }
                else
                {
                    update = false;
                    cleanUp = false;
                }

                bool hasPhysicalMaterial = false;

                for (const auto& material : materialGroup.m_materials)
                {
                    AZStd::shared_ptr<AZ::GFxFramework::IMaterial> mat = AZStd::make_shared<AZ::GFxFramework::Material>();
                    mat->EnableUseVertexColor(material.m_usesVertexColoring);
                    mat->SetMaterialFlags(material.m_physicsMaterialFlags);
                    //Done this way to avoid too long of a line.
                    hasPhysicalMaterial |= ((material.m_physicsMaterialFlags & AZ::GFxFramework::EMaterialFlags::MTL_FLAG_NODRAW) != 0);
                    hasPhysicalMaterial |= ((material.m_physicsMaterialFlags & AZ::GFxFramework::EMaterialFlags::MTL_FLAG_NODRAW_TOUCHBENDING) != 0);
                    mat->SetName(material.m_name);
                    if (material.m_materialData)
                    {
                        mat->SetTexture(AZ::GFxFramework::TextureMapType::Diffuse,
                            Utilities::FileUtilities::GetRelativePath(
                                material.m_materialData->GetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse)
                                , m_textureRootPath));

                        mat->SetTexture(AZ::GFxFramework::TextureMapType::Specular,
                            Utilities::FileUtilities::GetRelativePath(
                                material.m_materialData->GetTexture(DataTypes::IMaterialData::TextureMapType::Specular)
                                , m_textureRootPath));

                        mat->SetTexture(AZ::GFxFramework::TextureMapType::Bump,
                            Utilities::FileUtilities::GetRelativePath(
                                material.m_materialData->GetTexture(DataTypes::IMaterialData::TextureMapType::Bump)
                                , m_textureRootPath));

                        mat->SetDiffuseColor(material.m_materialData->GetDiffuseColor());
                        mat->SetSpecularColor(material.m_materialData->GetSpecularColor());
                        mat->SetEmissiveColor(material.m_materialData->GetEmissiveColor());
                        mat->SetOpacity(material.m_materialData->GetOpacity());
                        mat->SetShininess(material.m_materialData->GetShininess());
                    }

                    mat->SetDccMaterialHash(mat->CalculateDccMaterialHash());

                    size_t matIndex = matGroup.FindMaterialIndex(material.m_name);
                    if (update && matIndex != GFxFramework::MaterialExport::g_materialNotFound)
                    {
                        AZStd::shared_ptr<GFxFramework::IMaterial> origMat = matGroup.GetMaterial(matIndex);
                        origMat->SetName(mat->GetName());
                        origMat->EnableUseVertexColor(mat->UseVertexColor());
                        origMat->SetMaterialFlags(mat->GetMaterialFlags());
                        origMat->SetTexture(GFxFramework::TextureMapType::Diffuse, mat->GetTexture(GFxFramework::TextureMapType::Diffuse));
                        origMat->SetTexture(GFxFramework::TextureMapType::Specular, mat->GetTexture(GFxFramework::TextureMapType::Specular));
                        origMat->SetTexture(GFxFramework::TextureMapType::Bump, mat->GetTexture(GFxFramework::TextureMapType::Bump));
                        origMat->SetDccMaterialHash(mat->GetDccMaterialHash());
                    }
                    //This rule could change independently of an update material flag as it is set in the advanced rule.
                    else if (matIndex != GFxFramework::MaterialExport::g_materialNotFound)
                    {
                        AZStd::shared_ptr<GFxFramework::IMaterial> origMat = matGroup.GetMaterial(matIndex);
                        origMat->EnableUseVertexColor(mat->UseVertexColor());
                        origMat->SetDccMaterialHash(mat->GetDccMaterialHash());
                    }
                    else
                    {
                        matGroup.AddMaterial(mat);
                    }

                    if (cleanUp)
                    {
                        doNotRemoveGroup.AddMaterial(mat);
                    }
                }

                //Remove a physical material if one had been added previously. 
                if (!hasPhysicalMaterial)
                {
                    matGroup.RemoveMaterial(GFxFramework::MaterialExport::g_stringPhysicsNoDraw);
                }

                //Remove any unused materials
                if (materialGroup.m_removeMaterials)
                {
                    AZStd::vector<AZStd::string> removeNames;
                    for (size_t i = 0; i < matGroup.GetMaterialCount(); ++i)
                    {
                        if (doNotRemoveGroup.FindMaterialIndex(matGroup.GetMaterial(i)->GetName()) == GFxFramework::MaterialExport::g_materialNotFound)
                        {
                            removeNames.push_back(matGroup.GetMaterial(i)->GetName());
                        }
                    }

                    for (const auto& name : removeNames)
                    {
                        matGroup.RemoveMaterial(name);
                    }
                }

                matGroup.WriteMtlFile(filePath);

                return true;
            }

            bool MtlMaterialExporter::DoesMeshNodeHaveColorStreamChild(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex meshNode) const
            {
                const Containers::SceneGraph& graph = scene.GetGraph();

                auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, meshNode,
                    graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(view.begin(), view.end(), Containers::DerivedTypeFilter<DataTypes::IMeshVertexColorData>());
                return result != view.end();
            }
        } // namespace Export
    } // namespace SceneAPI
} // namespace AZ
