/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <API/EditorAssetSystemAPI.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMaterialImporter.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
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
        namespace SceneBuilder
        {
            static AZStd::string ResolveTexturePath([[maybe_unused]] const AZStd::string& materialName, const AssImpSDKWrapper::AssImpSceneWrapper& scene, const AZStd::string& textureFilePath);

            AssImpMaterialImporter::AssImpMaterialImporter()
            {
                BindToCall(&AssImpMaterialImporter::ImportMaterials);
            }

            void AssImpMaterialImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpMaterialImporter, SceneCore::LoadingComponent>()->Version(3);
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

                AZStd::unordered_map<int, AZStd::shared_ptr<SceneData::GraphData::MaterialData>> materialMap;
                for (unsigned int idx = 0; idx < context.m_sourceNode.GetAssImpNode()->mNumMeshes; ++idx)
                {
                    int meshIndex = context.m_sourceNode.GetAssImpNode()->mMeshes[idx];
                    const aiMesh* assImpMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[meshIndex];
                    AZ_Assert(assImpMesh, "Asset Importer Mesh should not be null.");
                    int materialIndex = assImpMesh->mMaterialIndex;
                    AZ_TraceContext("Material Index", materialIndex);

                    auto matFound = materialMap.find(materialIndex);

                    AZStd::shared_ptr<SceneData::GraphData::MaterialData> material;
                    AZStd::string materialName;

                    if (matFound == materialMap.end())
                    {
                        AZStd::shared_ptr<AssImpSDKWrapper::AssImpMaterialWrapper> assImpMaterial =
                            AZStd::shared_ptr<AssImpSDKWrapper::AssImpMaterialWrapper>(new AssImpSDKWrapper::AssImpMaterialWrapper(context.m_sourceScene.GetAssImpScene()->mMaterials[materialIndex]));

                        materialName = assImpMaterial->GetName().c_str();
                        RenamedNodesMap::SanitizeNodeName(materialName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "Material");
                        AZ_TraceContext("Material Name", materialName);

                        material = AZStd::make_shared<SceneData::GraphData::MaterialData>();

                        material->SetMaterialName(assImpMaterial->GetName());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Diffuse)).c_str());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Specular,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Specular)).c_str());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Bump,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Bump)).c_str());
                        material->SetTexture(DataTypes::IMaterialData::TextureMapType::Normal,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Normal)).c_str());
                        material->SetUniqueId(assImpMaterial->GetUniqueId());
                        material->SetDiffuseColor(assImpMaterial->GetDiffuseColor());
                        material->SetSpecularColor(assImpMaterial->GetSpecularColor());
                        material->SetEmissiveColor(assImpMaterial->GetEmissiveColor());
                        material->SetShininess(assImpMaterial->GetShininess());
                        material->SetOpacity(assImpMaterial->GetOpacity());

                        material->SetUseColorMap(assImpMaterial->GetUseColorMap());
                        material->SetBaseColor(assImpMaterial->GetBaseColor());

                        material->SetUseMetallicMap(assImpMaterial->GetUseMetallicMap());
                        material->SetMetallicFactor(assImpMaterial->GetMetallicFactor());
                        material->SetTexture(
                            DataTypes::IMaterialData::TextureMapType::Metallic,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Metallic).c_str()));

                        material->SetUseRoughnessMap(assImpMaterial->GetUseRoughnessMap());
                        material->SetRoughnessFactor(assImpMaterial->GetRoughnessFactor());
                        material->SetTexture(
                            DataTypes::IMaterialData::TextureMapType::Roughness,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Roughness).c_str()));

                        material->SetUseEmissiveMap(assImpMaterial->GetUseEmissiveMap());
                        material->SetEmissiveIntensity(assImpMaterial->GetEmissiveIntensity());

                        material->SetUseAOMap(assImpMaterial->GetUseAOMap());
                        material->SetTexture(
                            DataTypes::IMaterialData::TextureMapType::AmbientOcclusion,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::AmbientOcclusion).c_str()));

                        material->SetTexture(
                            DataTypes::IMaterialData::TextureMapType::Emissive,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::Emissive).c_str()));

                        material->SetTexture(
                            DataTypes::IMaterialData::TextureMapType::BaseColor,
                            ResolveTexturePath(materialName, context.m_sourceScene,
                                assImpMaterial->GetTextureFileName(SDKMaterial::MaterialWrapper::MaterialMapType::BaseColor).c_str()));

                        AZ_Assert(material, "Failed to allocate scene material data.");
                        if (!material)
                        {
                            combinedMaterialImportResults += Events::ProcessingResult::Failure;
                            continue;
                        }

                        materialMap[materialIndex] = material;

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
                            materialResult = SceneAPI::SceneBuilder::AddAttributeDataNodeWithContexts(dataPopulated);
                        }

                        combinedMaterialImportResults += materialResult;
                    }
                    else
                    {
                        material = matFound->second;
                        materialName = material->GetMaterialName();
                        AZ_Info(AZ::SceneAPI::Utilities::LogWindow, "Duplicate material references to %s from node %s",
                            materialName.c_str(), context.m_sourceNode.GetName());
                    }
                }

                return combinedMaterialImportResults.GetResult();
            }

            AZStd::string ResolveTexturePath([[maybe_unused]] const AZStd::string& materialName, const AssImpSDKWrapper::AssImpSceneWrapper& scene, const AZStd::string& textureFilePath)
            {
                if (textureFilePath.empty())
                {
                    AZ_Info(AZ::SceneAPI::Utilities::LogWindow, "Material %.*s has no associated texture.", AZ_STRING_ARG(materialName));
                    return textureFilePath;
                }
                const AZStd::string& sceneFilePath = scene.GetSceneFileName();
                const aiTexture* embeddedTexture = scene.GetAssImpScene()->GetEmbeddedTexture(textureFilePath.c_str());
                if (scene.GetExtractEmbeddedTextures() && embeddedTexture != nullptr)
                {
                    if (embeddedTexture->mHeight == 0)
                    {
                        AZ_Info(
                            AZ::SceneAPI::Utilities::LogWindow,
                            "Material %.*s has an embedded texture compressed as %s format",
                            AZ_STRING_ARG(materialName), embeddedTexture->achFormatHint);
                        AZStd::string relativeTexturePath, rootPath;
                        bool generatedRelativeSourcePath = false;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                            generatedRelativeSourcePath, &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                            sceneFilePath.c_str(), relativeTexturePath, rootPath);

                        AZStd::string textureFileName;
                        if (embeddedTexture->mFilename.length == 0)
                        {
                            // Set texture path as ${relative scene folder}/${scene filename}_${embedded texture index}
                            // for embedded texture, path starts with asterisk (like *1, *2, ...).
                            const char* embeddedTextureIndex = &(textureFilePath.c_str()[1]);
                            AZ::StringFunc::Path::GetFileName(sceneFilePath.c_str(), textureFileName);
                            textureFileName = AZStd::string::format("%s_%s_%s", textureFileName.c_str(), materialName.c_str(), embeddedTextureIndex);
                        }
                        else
                        {
                            AZ::StringFunc::Path::GetFileName(embeddedTexture->mFilename.C_Str(), textureFileName);
                        }
                        AZ::StringFunc::Path::ReplaceFullName(relativeTexturePath, textureFileName.c_str(), embeddedTexture->achFormatHint);
                        // Save the texture
                        IO::FileIOBase* fileIO = IO::FileIOBase::GetInstance();
                        AZStd::string fullTexturePath;
                        AZ::StringFunc::Path::Join(fileIO->GetAlias("@projectroot@"), relativeTexturePath.c_str(), fullTexturePath);

                        if (fileIO->Exists(fullTexturePath.c_str()))
                        {
                            // don't overwrite if the file already exists.
                            return relativeTexturePath;
                        }

                        IO::HandleType handle;
                        if (fileIO->Open(fullTexturePath.c_str(), IO::OpenMode::ModeWrite | IO::OpenMode::ModeBinary | IO::OpenMode::ModeCreatePath, handle))
                        {
                            u64 bytesWritten = 0;
                            fileIO->Write(handle, embeddedTexture->pcData, embeddedTexture->mWidth, &bytesWritten);
                            if (bytesWritten < embeddedTexture->mWidth)
                            {
                                AZ_Warning(
                                    "AtomFeatureCommon",
                                    false,
                                    "Failed to write all bytes to file '%s'",
                                    fullTexturePath.c_str());
                            }
                            fileIO->Close(handle);
                        }
                        else
                        {
                            AZ_Warning(
                                "AtomFeatureCommon",
                                false,
                                "Can not open path %s", fullTexturePath.c_str());
                        }
                        return relativeTexturePath;
                    }
                    else
                    {
                        AZ_Warning(
                            "AtomFeatureCommon",
                            false,
                            "Material %.*s has a uncompressed texture with absolute path '%.*s', which is not supported",
                            AZ_STRING_ARG(materialName),
                            AZ_STRING_ARG(textureFilePath));
                    }
                    return textureFilePath;
                }

                AZStd::string cleanedUpSceneFilePath(sceneFilePath);
                AZ::StringFunc::Path::StripFullName(cleanedUpSceneFilePath);
                
                AZ::IO::PathView textureFilePathView(textureFilePath.c_str());

                if (textureFilePathView.IsAbsolute())
                {
                    // Don't try to resolve the absolute path of the texture relative to the scene file,
                    // because it may resolve differently on different machines if the path happens to be
                    // correct on one person's machine but not another.
                    // Example: Texture is at C:\path\to\MyProject\assets\MyTexture.tga.
                    // On one person's machine, if their scene file is in that folder, example C:\path\to\MyProject\assets\MyFBX.fbx
                    // then comparing the path of the scene file and texture might properly resolve here.
                    // However, if a second person on the team's machine has the project at D:\path2\to2\MyProject,
                    // the same logic won't resolve.
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow,
                        "Material %.*s has a texture with absolute path '%.*s'. This path will not be resolved, this texture will not be found or used by this material.",
                        AZ_STRING_ARG(materialName), AZ_STRING_ARG(textureFilePath));
                    return textureFilePath;
                }

                AZStd::string texturePathRelativeToScene;
                AZ::StringFunc::Path::Join(cleanedUpSceneFilePath.c_str(), textureFilePath.c_str(), texturePathRelativeToScene);

                // If the texture did start with marker to change directories upward, then it's relative to the scene file,
                // and that path needs to be resolved. It can't be resolved later. This was handled internally by FBX SDK,
                // but is not handled by AssImp.
                if (textureFilePath.starts_with(".."))
                {
                    // Not checking for the file existing because it may not be there yet.
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow,
                        "Material %.*s has a texture '%.*s' with a directory change marker. This may not resolve correctly, the texture may not be found or used by this material.",
                        AZ_STRING_ARG(materialName), AZ_STRING_ARG(textureFilePath));
                    return texturePathRelativeToScene;
                }

                bool generatedRelativeSourcePath = false;
                AZStd::string relativePath, rootPath;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    generatedRelativeSourcePath, &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                    texturePathRelativeToScene.c_str(), relativePath, rootPath);

                // The engine only supports relative paths to scan folders.
                // Scene files may have paths to textures, relative to the scene file.
                // Try to use a scanfolder relative path instead
                if (generatedRelativeSourcePath)
                {
                    return relativePath;
                }


                return textureFilePath;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
