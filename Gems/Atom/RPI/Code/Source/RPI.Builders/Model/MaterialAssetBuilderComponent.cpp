/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Model/MaterialAssetBuilderComponent.h>
#include <Material/MaterialBuilderUtils.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Color.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialConverterBus.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Settings/SettingsRegistry.h>


namespace AZ
{
    namespace RPI
    {
        [[maybe_unused]] static const char* MaterialExporterName = "Scene Material Builder";

        void MaterialAssetDependenciesComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialAssetDependenciesComponent, Component>()
                    ->Version(5) // <<<<< If you have made changes to material code and need to force scene files to be reprocessed, this probably is
                                 // NOT the version number you want to bump . What you're looking for is MaterialAssetBuilderComponent::Reflect below.
                    ->Attribute(Edit::Attributes::SystemComponentTags, AZStd::vector<Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
            }
        }

        void MaterialAssetDependenciesComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("MaterialAssetDependenciesService"));
        }

        void MaterialAssetDependenciesComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("MaterialAssetDependenciesService"));
        }

        void MaterialAssetDependenciesComponent::Activate()
        {
            SceneAPI::SceneBuilderDependencyBus::Handler::BusConnect();
        }

        void MaterialAssetDependenciesComponent::Deactivate()
        {
            SceneAPI::SceneBuilderDependencyBus::Handler::BusDisconnect();
        }

        void MaterialAssetDependenciesComponent::ReportJobDependencies(SceneAPI::JobDependencyList& jobDependencyList, const char* platformIdentifier)
        {
            bool conversionEnabled = false;
            RPI::MaterialConverterBus::BroadcastResult(conversionEnabled, &RPI::MaterialConverterBus::Events::IsEnabled);
            
            // Right now, scene file importing only supports a single material type, once that changes, this will have to be re-designed, see ATOM-3554
            AZStd::string materialTypePath;
            RPI::MaterialConverterBus::BroadcastResult(materialTypePath, &RPI::MaterialConverterBus::Events::GetMaterialTypePath);

            if (conversionEnabled && !materialTypePath.empty())
            {
                AssetBuilderSDK::SourceFileDependency materialTypeSource;
                materialTypeSource.m_sourceFileDependencyPath = materialTypePath;

                AssetBuilderSDK::JobDependency jobDependency;
                jobDependency.m_jobKey = "Material Type Builder (Final Stage)";
                jobDependency.m_sourceFile = materialTypeSource;
                jobDependency.m_platformIdentifier = platformIdentifier;
                jobDependency.m_productSubIds.push_back(0);
                jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;

                jobDependencyList.push_back(jobDependency);
            }
        }
        
        void MaterialAssetDependenciesComponent::AddFingerprintInfo(AZStd::set<AZStd::string>& fingerprintInfo)
        {
            // This will cause scene files to be reprocessed whenever the global MaterialConverter settings change.

            AZStd::string conversionInfo = "[Material conversion info missing]";
            RPI::MaterialConverterBus::BroadcastResult(conversionInfo, &RPI::MaterialConverterBus::Events::GetFingerprintInfo);
            fingerprintInfo.insert(conversionInfo);
        }

        void MaterialAssetBuilderComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialAssetBuilderComponent, SceneAPI::SceneCore::ExportingComponent>()
                    ->Version(26);  // Add productSubId dependency for materialtype
            }
        }
        
        Data::Asset<MaterialAsset> MaterialAssetBuilderComponent::GetDefaultMaterialAsset() const
        {
            AZStd::string defaultMaterialPath;
            RPI::MaterialConverterBus::BroadcastResult(defaultMaterialPath, &RPI::MaterialConverterBus::Events::GetDefaultMaterialPath);

            if (defaultMaterialPath.empty())
            {
                return {};
            }
            else
            {
                auto defaultMaterialAssetId = RPI::AssetUtils::MakeAssetId(defaultMaterialPath, 0);
                if (!defaultMaterialAssetId.IsSuccess())
                {
                    AZ_Error("MaterialAssetBuilderComponent", false, "Could not find asset '%s'", defaultMaterialPath.c_str());
                    return {};
                }
                else
                {
                    return Data::AssetManager::Instance().CreateAsset<RPI::MaterialAsset>(defaultMaterialAssetId.GetValue(), Data::AssetLoadBehaviorNamespace::PreLoad);
                }
            }
        }

        uint32_t MaterialAssetBuilderComponent::GetMaterialAssetSubId(uint64_t materialUid)
        {
            // [GFX TODO] I am suggesting we use the first two 16bits for different kind of assets generated from a Scene
            // For example, 0x10000 for mesh, 0x20000 for material, 0x30000 for animation, 0x40000 for scene graph and etc. 
            // so the subid can be evaluated for reference across different assets generate within this scene file. 
            /*const uint32_t materialPrefix = 0x20000;
            AZ_Assert(materialPrefix > materialId, "materialId should be smaller than materialPrefix");
            return materialPrefix + materialId;*/
            return static_cast<uint32_t>(materialUid);
        }

        MaterialAssetBuilderComponent::MaterialAssetBuilderComponent()
        {
            // This setting disables material output (for automated testing purposes) to allow an FBX file to be processed without including
            // the dozens of dependencies required to process a material.  
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            bool skipAtomOutput = false;
            if (settingsRegistry && settingsRegistry->Get(skipAtomOutput, "/O3DE/SceneAPI/AssetImporter/SkipAtomOutput") && skipAtomOutput)
            {
                return;
            }

            BindToCall(&MaterialAssetBuilderComponent::BuildMaterials);
        }
        
        SceneAPI::Events::ProcessingResult MaterialAssetBuilderComponent::ConvertMaterials(MaterialAssetBuilderContext& context) const
        {
            const auto& scene = context.m_scene;
            const Uuid sourceSceneUuid = scene.GetSourceGuid();
            const auto& sceneGraph = scene.GetGraph();

            auto names = sceneGraph.GetNameStorage();
            auto content = sceneGraph.GetContentStorage();
            auto pairView = SceneAPI::Containers::Views::MakePairView(names, content);

            auto view = SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<
                SceneAPI::Containers::Views::BreadthFirst>(
                    sceneGraph, sceneGraph.GetRoot(), pairView.cbegin(), true);

            struct NamedMaterialSourceData
            {
                MaterialSourceData m_data;
                AZStd::string m_name;
            };
            AZStd::unordered_map<uint64_t, NamedMaterialSourceData> materialSourceDataByUid;

            for (const auto& viewIt : view)
            {
                if (viewIt.second == nullptr)
                {
                    continue;
                }

                if (azrtti_istypeof<SceneAPI::DataTypes::IMaterialData>(viewIt.second.get()))
                {
                    // Convert MaterialData to MaterialSourceData and add it to materialSourceDataByUid

                    auto materialData = AZStd::static_pointer_cast<const SceneAPI::DataTypes::IMaterialData>(viewIt.second);
                    uint64_t materialUid = materialData->GetUniqueId();
                    if (materialSourceDataByUid.find(materialUid) != materialSourceDataByUid.end())
                    {
                        continue;
                    }

                    // The source data for generating material asset
                    MaterialSourceData sourceData;

                    // User hook to create their materials based on the data from the scene pipeline
                    bool result = false;
                    RPI::MaterialConverterBus::BroadcastResult(result, &RPI::MaterialConverterBus::Events::ConvertMaterial, *materialData, sourceData);
                    if (result)
                    {
                        materialSourceDataByUid[materialUid] = { AZStd::move(sourceData), materialData->GetMaterialName() };
                    }
                }
            }
            
            // Build material assets. 
            for (auto& itr : materialSourceDataByUid)
            {
                const uint64_t materialUid = itr.first;
                Data::AssetId assetId(sourceSceneUuid, GetMaterialAssetSubId(materialUid));

                auto materialSourceData = itr.second;
                Outcome<Data::Asset<MaterialAsset>> result = materialSourceData.m_data.CreateMaterialAsset(assetId, "", false);
                if (result.IsSuccess())
                {
                    context.m_outputMaterialsByUid[materialUid] = { result.GetValue(), materialSourceData.m_name };
                }
                else
                {
                    AZ_Error(MaterialExporterName, false, "Create material failed");
                    return SceneAPI::Events::ProcessingResult::Failure;
                }
            }

            return SceneAPI::Events::ProcessingResult::Success;
        }

        SceneAPI::Events::ProcessingResult MaterialAssetBuilderComponent::AssignDefaultMaterials(MaterialAssetBuilderContext& context) const
        {
            Data::Asset<MaterialAsset> defaultMaterialAsset = GetDefaultMaterialAsset();

            if (!defaultMaterialAsset.GetId().IsValid())
            {
                AZ_Warning("MaterialAssetBuilderComponent", false, "Material conversion is disabled but no default material was provided. The model will likely be invisible by default.");
                // Return success because it's just a warning.
                return SceneAPI::Events::ProcessingResult::Success;
            }

            const auto& scene = context.m_scene;
            const auto& sceneGraph = scene.GetGraph();

            auto names = sceneGraph.GetNameStorage();
            auto content = sceneGraph.GetContentStorage();
            auto pairView = SceneAPI::Containers::Views::MakePairView(names, content);

            auto view = SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<
                SceneAPI::Containers::Views::BreadthFirst>(
                    sceneGraph, sceneGraph.GetRoot(), pairView.cbegin(), true);

            for (const auto& viewIt : view)
            {
                if (viewIt.second == nullptr)
                {
                    continue;
                }

                if (azrtti_istypeof<SceneAPI::DataTypes::IMaterialData>(viewIt.second.get()))
                {
                    auto materialData = AZStd::static_pointer_cast<const SceneAPI::DataTypes::IMaterialData>(viewIt.second);
                    uint64_t materialUid = materialData->GetUniqueId();

                    context.m_outputMaterialsByUid[materialUid] = { defaultMaterialAsset, materialData->GetMaterialName() };
                }
            }

            return SceneAPI::Events::ProcessingResult::Success;
        }

        SceneAPI::Events::ProcessingResult MaterialAssetBuilderComponent::BuildMaterials(MaterialAssetBuilderContext& context) const
        {
            bool conversionEnabled = false;
            RPI::MaterialConverterBus::BroadcastResult(conversionEnabled, &RPI::MaterialConverterBus::Events::IsEnabled);

            if (conversionEnabled)
            {
                return ConvertMaterials(context);
            }
            else
            {
                return AssignDefaultMaterials(context);
            }

        }
    } // namespace RPI
} // namespace AZ

