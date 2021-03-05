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

#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMaterialImporter.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpMaterialWrapper.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <assimp/scene.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            AssImpMaterialImporter::AssImpMaterialImporter()
            {
                BindToCall(&AssImpMaterialImporter::ImportMaterials);
            }

            void AssImpMaterialImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpMaterialImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult AssImpMaterialImporter::ImportMaterials(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Material");
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedMaterialImportResults;
                for (int idx = 0; idx < context.m_sourceNode.m_assImpNode->mNumMeshes; ++idx)
                {
                    int meshIndex = context.m_sourceNode.m_assImpNode->mMeshes[idx];
                    aiMesh* assImpMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[meshIndex];
                    AZ_Assert(assImpMesh, "Asset Importer Mesh should not be null.");
                    int materialIndex = assImpMesh->mMaterialIndex;
                    AZ_TraceContext("Material Index", materialIndex);

                    AZStd::unordered_map<int, AZStd::shared_ptr<SceneData::GraphData::MaterialData>> materialMap;

                    auto matFound = materialMap.find(materialIndex);

                    AZStd::shared_ptr<SceneData::GraphData::MaterialData> material;
                    AZStd::string materialName;

                    if (matFound == materialMap.end())
                    {
                        std::shared_ptr<AssImpSDKWrapper::AssImpMaterialWrapper> assImpMaterial =
                            std::shared_ptr<AssImpSDKWrapper::AssImpMaterialWrapper>(new AssImpSDKWrapper::AssImpMaterialWrapper(context.m_sourceScene.GetAssImpScene()->mMaterials[materialIndex]));

                        materialName = assImpMaterial->GetName().c_str();
                        RenamedNodesMap::SanitizeNodeName(materialName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "Material");
                        AZ_TraceContext("Material Name", materialName);

                        material = AZStd::make_shared<SceneData::GraphData::MaterialData>();

                        material->SetMaterialName(assImpMaterial->GetName());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse,
                            assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Diffuse).c_str());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Specular,
                            assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Specular).c_str());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Bump,
                            assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Bump).c_str());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Normal,
                            assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Normal).c_str());
                        material->SetUniqueId(assImpMaterial->GetUniqueId());
                        material->SetDiffuseColor(assImpMaterial->GetDiffuseColor());
                        material->SetSpecularColor(assImpMaterial->GetSpecularColor());
                        material->SetEmissiveColor(assImpMaterial->GetEmissiveColor());
                        material->SetShininess(assImpMaterial->GetShininess());


                        AZ_Assert(material, "Failed to allocate scene material data.");
                        if (!material)
                        {
                            combinedMaterialImportResults += Events::ProcessingResult::Failure;
                            continue;
                        }

                        materialMap[materialIndex] = material;
                    }
                    else
                    {
                        material = matFound->second;
                        materialName = material.get()->GetMaterialName();
                    }

                    Events::ProcessingResult materialResult;
                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, materialName.c_str());

                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedMaterialImportResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    AssImpSceneAttributeDataPopulatedContext dataPopulated(context, material, newIndex, materialName);
                    materialResult = Events::Process(dataPopulated);

                    if (materialResult != Events::ProcessingResult::Failure)
                    {
                        materialResult = SceneAPI::FbxSceneBuilder::AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedMaterialImportResults += materialResult;
                }

                return combinedMaterialImportResults.GetResult();
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
