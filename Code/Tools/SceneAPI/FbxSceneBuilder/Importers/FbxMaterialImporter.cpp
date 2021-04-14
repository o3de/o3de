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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxMaterialImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMaterialWrapper.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxMaterialImporter::FbxMaterialImporter()
            {
                BindToCall(&FbxMaterialImporter::ImportMaterials);
            }

            void FbxMaterialImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxMaterialImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxMaterialImporter::ImportMaterials(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Material");

                if (!context.m_sourceNode.GetMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedMaterialImportResults;

                for (int materialIndex = 0; materialIndex < context.m_sourceNode.GetMaterialCount(); ++materialIndex)
                {
                    AZ_TraceContext("Material Index", materialIndex);

                    const std::shared_ptr<FbxSDKWrapper::FbxMaterialWrapper> fbxMaterial = 
                        context.m_sourceNode.GetMaterial(materialIndex);

                    if (!fbxMaterial)
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Invalid material data found, ignoring.");
                        continue;
                    }

                    AZStd::string materialName = fbxMaterial->GetName().c_str();
                    RenamedNodesMap::SanitizeNodeName(materialName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "Material");
                    AZ_TraceContext("Material Name", materialName);

                    AZStd::shared_ptr<SceneData::GraphData::MaterialData> materialData =
                        BuildMaterial(context.m_sourceNode, materialIndex);

                    AZ_Assert(materialData, "Failed to allocate scene material data.");
                    if (!materialData)
                    {
                        combinedMaterialImportResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Events::ProcessingResult materialResult;
                    Containers::SceneGraph::NodeIndex newIndex = 
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, materialName.c_str());

                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if(!newIndex.IsValid())
                    {
                        combinedMaterialImportResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    SceneAttributeDataPopulatedContext dataPopulated(context, materialData, newIndex, materialName);
                    materialResult = Events::Process(dataPopulated);

                    if (materialResult != Events::ProcessingResult::Failure)
                    {
                        materialResult = AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedMaterialImportResults += materialResult;
                }

                return combinedMaterialImportResults.GetResult();
            }

            AZStd::shared_ptr<SceneData::GraphData::MaterialData> FbxMaterialImporter::BuildMaterial(FbxSDKWrapper::FbxNodeWrapper& node, int materialIndex) const
            {
                AZ_Assert(materialIndex < node.GetMaterialCount(), "Invalid material index (%i)", materialIndex);
                const std::shared_ptr<FbxSDKWrapper::FbxMaterialWrapper> fbxMaterial = node.GetMaterial(materialIndex);
                if (!fbxMaterial)
                {
                    return nullptr;
                }

                AZStd::shared_ptr<SceneData::GraphData::MaterialData> material = AZStd::make_shared<SceneData::GraphData::MaterialData>();

                material->SetMaterialName(fbxMaterial->GetName());
                material->SetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse,
                    fbxMaterial->GetTextureFileName(FbxSDKWrapper::FbxMaterialWrapper::MaterialMapType::Diffuse).c_str());
                material->SetTexture(DataTypes::IMaterialData::TextureMapType::Specular,
                    fbxMaterial->GetTextureFileName(FbxSDKWrapper::FbxMaterialWrapper::MaterialMapType::Specular).c_str());
                material->SetTexture(DataTypes::IMaterialData::TextureMapType::Bump,
                    fbxMaterial->GetTextureFileName(FbxSDKWrapper::FbxMaterialWrapper::MaterialMapType::Bump).c_str());
                material->SetTexture(DataTypes::IMaterialData::TextureMapType::Normal,
                    fbxMaterial->GetTextureFileName(FbxSDKWrapper::FbxMaterialWrapper::MaterialMapType::Normal).c_str());
                material->SetDiffuseColor(fbxMaterial->GetDiffuseColor());
                material->SetSpecularColor(fbxMaterial->GetSpecularColor());
                material->SetEmissiveColor(fbxMaterial->GetEmissiveColor());
                material->SetShininess(fbxMaterial->GetShininess());

                float opacity = fbxMaterial->GetOpacity();
                if (opacity == 0.0f)
                {
                    opacity = 1.0f;
                    AZ_TracePrintf(Utilities::WarningWindow, "Opacity has been changed from 0 to full. Some DCC tools ignore the opacity and "
                        "write 0 to indicate opacity is not used. This causes meshes to turn invisible, which is often not the intention so "
                        "the opacity has been set to full automatically. If the intention was for a fully transparent mesh, please update "
                        "the opacity in Open 3D Engine's material editor.");
                }
                material->SetOpacity(opacity);

                // Due to the fact that fbxMaterial->GetUniqueId() will return a different ID
                // each time when the fbx is reprocessed, we need a more stable ID.
                // The current best candidate is probably the name of the material.
                // But this will also force the user to update the material component for overrides
                // if the fbx material name is changed outside from apps like dcc tools.
                // (Ideally, only changing the material properties in the dcc tool will force the user to update the material component)
                //
                // Using 32-bit CRC as it is mathematically stable and enough within the same fbx.
                uint64_t id = uint32_t(AZ::Crc32(fbxMaterial->GetName()));
                material->SetUniqueId(id);
                return material;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ 
