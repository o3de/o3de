/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Model/MaterialAssetBuilderComponent.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Color.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

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

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

namespace AZ
{
    namespace RPI
    {
        static const char* MaterialExporterName = "Scene Material Builder";

        void MaterialAssetDependenciesComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialAssetDependenciesComponent, Component>()
                    ->Version(4)
                    ->Attribute(Edit::Attributes::SystemComponentTags, AZStd::vector<Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
            }
        }

        void MaterialAssetDependenciesComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MaterialAssetDependenciesService", 0x28bbd0f3));
        }

        void MaterialAssetDependenciesComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("MaterialAssetDependenciesService", 0x28bbd0f3));
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
            AssetBuilderSDK::SourceFileDependency materialTypeSource;
            // Right now, scene file importing only supports a single material type, once that changes, this will have to be re-designed, see ATOM-3554
            RPI::MaterialConverterBus::BroadcastResult(materialTypeSource.m_sourceFileDependencyPath, &RPI::MaterialConverterBus::Events::GetMaterialTypePath);

            AssetBuilderSDK::JobDependency jobDependency;
            jobDependency.m_jobKey = "Atom Material Builder";
            jobDependency.m_sourceFile = materialTypeSource;
            jobDependency.m_platformIdentifier = platformIdentifier;
            jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;

            jobDependencyList.push_back(jobDependency);
        }

        void MaterialAssetBuilderComponent::Reflect(ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialAssetBuilderComponent, SceneAPI::SceneCore::ExportingComponent>()
                    ->Version(14);  // [ATOM-13410]
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
            BindToCall(&MaterialAssetBuilderComponent::BuildMaterials);
        }

        SceneAPI::Events::ProcessingResult MaterialAssetBuilderComponent::BuildMaterials(MaterialAssetBuilderContext& context) const
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
    } // namespace RPI
} // namespace AZ

