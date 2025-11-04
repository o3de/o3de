/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Model/ModelExporterComponent.h>

#include <AzCore/Asset/AssetCommon.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>

#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

#include <cinttypes>

#include <Atom/RPI.Builders/Model/ModelExporterContexts.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace RPI
    {
        [[maybe_unused]] static const char* s_exporterName = "Atom Model Builder";

        ModelExporterComponent::ModelExporterComponent()
        {
            // This setting disables model output (for automated testing purposes) to allow an FBX file to be processed without including
            // all the dependencies required to process a model.  
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            bool skipAtomOutput = false;
            if (settingsRegistry && settingsRegistry->Get(skipAtomOutput, "/O3DE/SceneAPI/AssetImporter/SkipAtomOutput") && skipAtomOutput)
            {
                return;
            }

            BindToCall(&ModelExporterComponent::ExportModel);
        }

        SceneAPI::Events::ProcessingResult
            ModelExporterComponent::ExportModel(SceneAPI::Events::ExportEventContext& exportEventContext) const
        {
            const SceneAPI::Containers::SceneManifest& manifest =
                exportEventContext.GetScene().GetManifest();

            const Uuid sourceSceneUuid = exportEventContext.GetScene().GetSourceGuid();

            auto valueStorage = manifest.GetValueStorage();
            auto view = SceneAPI::Containers::MakeDerivedFilterView<
                SceneAPI::DataTypes::IMeshGroup>(valueStorage);
            
            SceneAPI::Events::ProcessingResultCombiner combinerResult;

            MaterialAssetsByUid materialsByUid;
            MaterialAssetBuilderContext materialContext(exportEventContext.GetScene(), materialsByUid);
            combinerResult = SceneAPI::Events::Process<MaterialAssetBuilderContext>(materialContext);
            if (combinerResult.GetResult() == SceneAPI::Events::ProcessingResult::Failure)
            {
                return SceneAPI::Events::ProcessingResult::Failure;
            }

            //Export MaterialAssets
            for (auto& materialPair : materialsByUid)
            {
                const Data::Asset<MaterialAsset>& asset = materialPair.second.m_asset;
                
                // MaterialAssetBuilderContext could attach an independent material asset rather than
                // generate one using the scene data, so we must skip the export step in that case.
                if (asset.GetId().m_guid != exportEventContext.GetScene().GetSourceGuid())
                {
                    continue;
                }

                uint64_t materialUid = materialPair.first;
                const AZStd::string& sceneName = exportEventContext.GetScene().GetName();

                // escape the material name acceptable for a filename
                AZStd::string materialName = materialPair.second.m_name;
                for (char& item : materialName)
                {
                    if (!isalpha(item) && !isdigit(item))
                    {
                        item = '_';
                    }
                }

                const AZStd::string relativeMaterialFileName = AZStd::string::format("%s_%s_%" PRIu64, sceneName.c_str(), materialName.data(), materialUid);

                AssetExportContext materialExportContext =
                {
                    relativeMaterialFileName,
                    MaterialAsset::Extension,
                    sourceSceneUuid,
                    DataStream::ST_BINARY
                };

                if (!ExportAsset(asset, materialExportContext, exportEventContext, "Material"))
                {
                    return SceneAPI::Events::ProcessingResult::Failure;
                }
            }

            AZStd::set<AZStd::string> groupNames;

            for (const SceneAPI::DataTypes::IMeshGroup& meshGroup : view)
            {
                const AZStd::string& meshGroupName = meshGroup.GetName();

                //Check for duplicate group names
                if(groupNames.find(meshGroupName) != groupNames.end())
                {
                    AZ_Warning(s_exporterName, false, "Multiple mesh groups with duplicate name: \"%s\". Skipping export...", meshGroupName.c_str());
                    continue;
                }

                groupNames.insert(meshGroupName);

                AZ_TraceContext("Mesh group", meshGroupName.c_str());

                // Get the coordinate system conversion rule.
                AZ::SceneAPI::CoordinateSystemConverter coordSysConverter;
                AZStd::shared_ptr<AZ::SceneAPI::SceneData::CoordinateSystemRule> coordinateSystemRule = meshGroup.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::CoordinateSystemRule>();
                if (coordinateSystemRule)
                {
                    coordinateSystemRule->UpdateCoordinateSystemConverter();
                    coordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
                }

                Data::Asset<ModelAsset> modelAsset;
                Data::Asset<SkinMetaAsset> skinMetaAsset;
                Data::Asset<MorphTargetMetaAsset> morphTargetMetaAsset;
                ModelAssetBuilderContext modelContext(exportEventContext.GetScene(), meshGroup, coordSysConverter, materialsByUid, modelAsset, skinMetaAsset, morphTargetMetaAsset);
                combinerResult = SceneAPI::Events::Process<ModelAssetBuilderContext>(modelContext);
                if (combinerResult.GetResult() != SceneAPI::Events::ProcessingResult::Success)
                {
                    return combinerResult.GetResult();
                }
                ModelAssetPostBuildContext modelAssetPostBuildContext(
                    exportEventContext.GetScene(),
                    exportEventContext.GetOutputDirectory(),
                    exportEventContext.GetProductList(),
                    meshGroup,
                    modelAsset);
                combinerResult = SceneAPI::Events::Process<ModelAssetPostBuildContext>(modelAssetPostBuildContext);
                if (combinerResult.GetResult() != SceneAPI::Events::ProcessingResult::Success)
                {
                    return combinerResult.GetResult();
                }

                //Retrieve source asset info so we can get a string with the relative path to the asset
                bool assetInfoResult;
                Data::AssetInfo info;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetInfoResult, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, exportEventContext.GetScene().GetSourceFilename().c_str(), info, watchFolder);

                AZ_Assert(assetInfoResult, "Failed to retrieve source asset info. Can't reason about product asset paths");

                for (const Data::Asset<ModelLodAsset>& lodAsset : modelAsset->GetLodAssets())
                {
                    AZStd::set<uint32_t> exportedSubAssets;

                    for (const ModelLodAsset::Mesh& mesh : lodAsset->GetMeshes())
                    {
                        //Export all BufferAssets for this Lod

                        //Export Index Buffer
                        {
                            const Data::Asset<BufferAsset>& indexBufferAsset = mesh.GetIndexBufferAssetView().GetBufferAsset();
                            if (exportedSubAssets.find(indexBufferAsset.GetId().m_subId) == exportedSubAssets.end())
                            {
                                AssetExportContext bufferExportContext =
                                {
                                    indexBufferAsset.GetHint(),
                                    BufferAsset::Extension,
                                    sourceSceneUuid,
                                    DataStream::ST_BINARY
                                };

                                if (!ExportAsset(indexBufferAsset, bufferExportContext, exportEventContext, "Buffer"))
                                {
                                    return SceneAPI::Events::ProcessingResult::Failure;
                                }

                                exportedSubAssets.insert(indexBufferAsset.GetId().m_subId);
                            }
                        }

                        //Export Stream Buffers
                        for (const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : mesh.GetStreamBufferInfoList())
                        {
                            const Data::Asset<BufferAsset>& bufferAsset = streamBufferInfo.m_bufferAssetView.GetBufferAsset();
                            if (exportedSubAssets.find(bufferAsset.GetId().m_subId) == exportedSubAssets.end())
                            {
                                AssetExportContext bufferExportContext =
                                {
                                    bufferAsset.GetHint(),
                                    BufferAsset::Extension,
                                    sourceSceneUuid,
                                    DataStream::ST_BINARY
                                };

                                if (!ExportAsset(bufferAsset, bufferExportContext, exportEventContext, "Buffer"))
                                {
                                    return SceneAPI::Events::ProcessingResult::Failure;
                                }

                                exportedSubAssets.insert(bufferAsset.GetId().m_subId);
                            }
                        }
                    }

                    //Export ModelLodAsset
                    AssetExportContext lodExportContext =
                    {
                        lodAsset.GetHint(),
                        ModelLodAsset::Extension,
                        sourceSceneUuid,
                        DataStream::ST_BINARY
                    };

                    if (!ExportAsset(lodAsset, lodExportContext, exportEventContext, "Model LOD"))
                    {
                        return SceneAPI::Events::ProcessingResult::Failure;
                    }

                } //foreach lodAsset                

                //Export ModelAsset
                AssetExportContext modelExportContext =
                {
                    modelAsset.GetHint(),
                    ModelAsset::Extension,
                    sourceSceneUuid,
                    DataStream::ST_BINARY
                };

                if (!ExportAsset(modelAsset, modelExportContext, exportEventContext, "Model"))
                {
                    return SceneAPI::Events::ProcessingResult::Failure;
                }

                // Export skin meta data
                if (skinMetaAsset.IsReady())
                {
                    AssetExportContext skinMetaExportContext =
                    {
                        /*relativeFilename=*/meshGroupName,
                        SkinMetaAsset::Extension,
                        sourceSceneUuid,
                        DataStream::ST_JSON
                    };

                    if (!ExportAsset(skinMetaAsset, skinMetaExportContext, exportEventContext, "SkinMeta"))
                    {
                        return SceneAPI::Events::ProcessingResult::Failure;
                    }
                }

                // Export morph target meta data
                if (morphTargetMetaAsset.IsReady())
                {
                    AssetExportContext morphTargetMetaExportContext =
                    {
                        /*relativeFilename=*/meshGroupName,
                        MorphTargetMetaAsset::Extension,
                        sourceSceneUuid,
                        DataStream::ST_JSON
                    };

                    if (!ExportAsset(morphTargetMetaAsset, morphTargetMetaExportContext, exportEventContext, "MorphTargetMeta"))
                    {
                        return SceneAPI::Events::ProcessingResult::Failure;
                    }
                }
            } //foreach meshGroup

            return SceneAPI::Events::ProcessingResult::Success;
        }

        template<class T>
        bool ModelExporterComponent::ExportAsset(
            const Data::Asset<T>& asset,
            const AssetExportContext& assetContext,
            SceneAPI::Events::ExportEventContext& context,
            [[maybe_unused]] const char* assetTypeDebugName) const
        {
            const AZStd::string assetFileName = SceneAPI::Utilities::FileUtilities::CreateOutputFileName(
                assetContext.m_relativeFileName, context.GetOutputDirectory(), assetContext.m_extension,
                context.GetScene().GetSourceExtension());

            if (!Utils::SaveObjectToFile(assetFileName, assetContext.m_dataStreamType, asset.Get()))
            {
                AZ_Error(s_exporterName, false, "Failed to save %s to file %s", assetTypeDebugName, assetFileName.c_str());
                return false;
            }

            const Uuid assetUuid = asset.GetId().m_guid;
            if (assetUuid != assetContext.m_sourceUuid)
            {
                AZ_Assert(false, "All product UUIDs should be the same as the scene source UUID");
                return false;
            }

            const uint32_t assetSubId = asset.GetId().m_subId;

            // Add product to output list
            // Otherwise the asset won't be copied from temp folders to cache
            SceneAPI::Events::ExportProduct& product = context.GetProductList().AddProduct(
                assetFileName, assetUuid, asset->GetType(), AZStd::nullopt, assetSubId);

            AssetBuilderSDK::JobProduct jobProduct;
            if (!AssetBuilderSDK::OutputObject(asset.Get(), assetFileName, asset->GetType(), assetSubId, jobProduct))
            {
                AZ_Assert(false, "Failed to output product dependencies.");
                return false;
            }
            for (auto& dependency : jobProduct.m_dependencies)
            {
                product.m_productDependencies.push_back(SceneAPI::Events::ExportProduct{
                    {},
                    dependency.m_dependencyId.m_guid,
                    {},
                    AZStd::nullopt,
                    dependency.m_dependencyId.m_subId,
                    dependency.m_flags
                    });
            }
            return true;
        }

        void ModelExporterComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<ModelExporterComponent, SceneAPI::SceneCore::ExportingComponent>()
                    ->Version(4);
            }
        }

        ModelExporterComponent::AssetExportContext::AssetExportContext(
            AZStd::string_view relativeFileName,
            AZStd::string_view extension,
            Uuid sourceUuid,
            DataStream::StreamType dataStreamType)
            : m_relativeFileName{relativeFileName}
            , m_extension{extension}
            , m_sourceUuid{sourceUuid}
            , m_dataStreamType{dataStreamType}
        {}
    } // namespace RPI
} // namespace AZ
