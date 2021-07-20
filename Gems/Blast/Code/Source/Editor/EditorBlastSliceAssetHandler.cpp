/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StdAfx.h"

#include <Editor/EditorBlastMeshDataComponent.h>
#include <Editor/EditorBlastSliceAssetHandler.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>

#include <GFxFramework/MaterialIO/Material.h>

namespace Blast
{
    // BlastSliceAssetStorageComponent

    void BlastSliceAssetStorageComponent::Reflect(AZ::ReflectContext* context)
    {
        using namespace AZ::Edit;

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BlastSliceAssetStorageComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("Mesh Data", &BlastSliceAssetStorageComponent::m_meshAssetIdList)
                ->Field("Mesh Path List", &BlastSliceAssetStorageComponent::m_meshAssetPathList);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<BlastSliceAssetStorageComponent>(
                      "Blast Slice Storage Component", "Used process blast slice data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Box.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Box.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastSliceAssetStorageComponent::m_meshAssetIdList, "Mesh Data",
                        "Slice data to fill out the mesh list")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastSliceAssetStorageComponent::m_meshAssetPathList,
                        "Mesh Paths", "The mesh path list");
            }
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<BlastSliceAssetStorageComponent>("BlastSliceAssetStorageComponent")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "blast")
                ->Method("GenerateAssetInfo", &BlastSliceAssetStorageComponent::GenerateAssetInfo)
                ->Method("WriteMaterialFile", &BlastSliceAssetStorageComponent::WriteMaterialFile);
        }
    }

    bool BlastSliceAssetStorageComponent::GenerateAssetInfo(
        const AZStd::vector<AZStd::string>& chunkNames, AZStd::string_view blastFilename,
        AZStd::string_view assetinfoFilename)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext == nullptr)
        {
            return false;
        }
        using namespace AZ::SceneAPI::Containers;
        using namespace AZ::SceneAPI::SceneData;

        AZStd::string filename;
        AZ::StringFunc::Path::Split(blastFilename.data(), nullptr, nullptr, &filename, nullptr);

        AZStd::any sceneManifestPointer(serializeContext->CreateAny(azrtti_typeid<SceneManifest>()));
        SceneManifest* sceneManifest = AZStd::any_cast<SceneManifest>(&sceneManifestPointer);

        AZStd::vector<AZStd::any> meshGroupData;
        meshGroupData.reserve(chunkNames.size());

        AZStd::vector<AZStd::any> materialRuleData;
        materialRuleData.reserve(chunkNames.size());

        for (const AZStd::string& chunkName : chunkNames)
        {
            meshGroupData.emplace_back(serializeContext->CreateAny(azrtti_typeid<MeshGroup>()));
            AZStd::any& meshGroupPointer = meshGroupData.back();
            MeshGroup* meshGroup = AZStd::any_cast<MeshGroup>(&meshGroupPointer);

            // make selection list
            meshGroup->GetSceneNodeSelectionList().RemoveSelectedNode("RootNode");
            for (const AZStd::string& node : chunkNames)
            {
                meshGroup->GetSceneNodeSelectionList().RemoveSelectedNode(
                    AZStd::string::format("RootNode.%s", node.c_str()));
            }
            meshGroup->GetSceneNodeSelectionList().AddSelectedNode(
                AZStd::string::format("RootNode.%s", chunkName.c_str()));

            // create a default material for the mesh group
            materialRuleData.emplace_back(serializeContext->CreateAny(azrtti_typeid<MaterialRule>()));
            AZStd::any& materialRulePointer = materialRuleData.back();
            MaterialRule* materialRule = AZStd::any_cast<MaterialRule>(&materialRulePointer);

            // override the deleter since the AZStd::any will clean up later on
            AZStd::shared_ptr<MaterialRule> materialRuleEntry = AZStd::shared_ptr<MaterialRule>(
                materialRule,
                [](auto)
                {
                });
            meshGroup->GetRuleContainer().AddRule(materialRuleEntry);

            // construct the asset name for the chunk's mesh group
            AZStd::string meshGroupName(filename);
            meshGroupName.append("-");
            meshGroupName.append(chunkName);
            // TODO: Uncomment lines below as part of SPEC-3542
            // meshGroup->OverrideId(AZ::Uuid::CreateName(meshGroupName.c_str()));
            // meshGroup->SetName(AZStd::move(meshGroupName));

            // override the deleter since the AZStd::any will clean up later on
            AZStd::shared_ptr<MeshGroup> meshGroupEntry = AZStd::shared_ptr<MeshGroup>(
                meshGroup,
                [](auto)
                {
                });
            sceneManifest->AddEntry(AZStd::move(meshGroupEntry));
        }

        return sceneManifest->SaveToFile(assetinfoFilename.data());
    }

    bool BlastSliceAssetStorageComponent::WriteMaterialFile(
        AZStd::string_view materialGroupName, const AZStd::vector<AZStd::string>& materialNames,
        AZStd::string_view materialFilename)
    {
        AZ::GFxFramework::MaterialGroup group;
        for (const auto& texture : materialNames)
        {
            auto mat = AZStd::make_shared<AZ::GFxFramework::Material>();
            mat->SetName(texture);
            mat->SetTexture(AZ::GFxFramework::TextureMapType::Diffuse, "EngineAssets/Textures/white.dds");
            group.AddMaterial(mat);
        }
        group.SetMtlName(materialGroupName);
        return group.WriteMtlFile(materialFilename.data());
    }

    //
    // EditorBlastSliceAssetHandler
    //

    EditorBlastSliceAssetHandler::~EditorBlastSliceAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr EditorBlastSliceAssetHandler::CreateAsset(
        const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type != GetAssetType())
        {
            AZ_Error("Blast", type == GetAssetType(), "Invalid asset type! We only handle 'BlastAsset'");
            return {};
        }

        if (!CanHandleAsset(id))
        {
            return nullptr;
        }

        return aznew BlastSliceAsset;
    }

    AZ::Data::AssetHandler::LoadResult EditorBlastSliceAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        BlastSliceAsset* blastSliceAssetData = asset.GetAs<BlastSliceAsset>();
        AZ_Error(
            "blast", blastSliceAssetData,
            "This should be a BlastSliceAsset type, as this is the only type we process!");
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (blastSliceAssetData && serializeContext)
        {
            AZ::ObjectStream::FilterDescriptor filter(assetLoadFilterCB);
            AZStd::unique_ptr<AZ::Entity> baseEntity(
                AZ::Utils::LoadObjectFromStream<AZ::Entity>(*stream, serializeContext, filter));
            AZ_Error("Blast", baseEntity, "Could not load slice root entity {asset id}");
            if (!baseEntity)
            {
                return LoadResult::Error;
            }

            auto&& sliceComponent = baseEntity->FindComponent<AZ::SliceComponent>();
            AZ_Error("Blast", sliceComponent, "blast_slice entity missing SliceComponent!");
            if (sliceComponent == nullptr)
            {
                return LoadResult::Error;
            }

            AZStd::vector<AZ::Entity*> enityList;
            sliceComponent->GetEntities(enityList);
            for (auto&& entity : enityList)
            {
                // the base element type to store Blast mesh data is the BlastSliceAssetStorageComponent
                auto&& blastSliceAssetStorage = entity->FindComponent<BlastSliceAssetStorageComponent>();
                if (blastSliceAssetStorage)
                {
                    if (blastSliceAssetStorage->GetMeshData().empty() == false)
                    {
                        blastSliceAssetData->SetMeshIdList(blastSliceAssetStorage->GetMeshData());
                        return LoadResult::LoadComplete;
                    }
                    else if (blastSliceAssetStorage->GetMeshPathList().empty() == false)
                    {
                        AZStd::vector<AZ::Data::AssetId> meshAssetIdList;
                        meshAssetIdList.reserve(blastSliceAssetStorage->GetMeshPathList().size());

                        for (auto&& assetPath : blastSliceAssetStorage->GetMeshPathList())
                        {
                            AZ::Data::AssetId meshAssetId;
                            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                                meshAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                                assetPath.c_str(), AZ::Data::s_invalidAssetType, false);

                            if (meshAssetId.IsValid())
                            {
                                meshAssetIdList.emplace_back(meshAssetId);
                            }
                        }
                        blastSliceAssetData->SetMeshIdList(meshAssetIdList);
                        return LoadResult::LoadComplete;
                    }
                }

                // back up logic to load blast data for the EditorBlastMeshDataComponent
                auto&& meshDataComponent = entity->FindComponent<EditorBlastMeshDataComponent>();
                if (meshDataComponent)
                {
                    auto&& innerBlastSliceAsset = meshDataComponent->GetBlastSliceAsset();
                    if (innerBlastSliceAsset.IsReady())
                    {
                        blastSliceAssetData->SetMeshIdList(innerBlastSliceAsset.Get()->GetMeshIdList());
                        blastSliceAssetData->SetMaterialId(innerBlastSliceAsset.Get()->GetMaterialId());
                        return LoadResult::LoadComplete;
                    }
                    else
                    {
                        auto&& meshDataList = meshDataComponent->GetMeshAssets();
                        AZStd::vector<AZ::Data::AssetId> meshAssetIdList;
                        meshAssetIdList.reserve(meshDataList.size());
                        for (auto&& meshData : meshDataList)
                        {
                            AZ::RPI::ModelAsset* meshAsset = meshData.Get();
                            if (meshAsset)
                            {
                                meshAssetIdList.push_back(meshAsset->GetId());
                            }
                        }
                        blastSliceAssetData->SetMeshIdList(meshAssetIdList);
                        return LoadResult::LoadComplete;
                    }
                }
            }
            AZ_Error(
                "Blast", false, "blast_slice assetId:%s missing EditorBlastMeshDataComponent!",
                asset->GetId().ToString<AZStd::string>().c_str());
        }
        return LoadResult::Error;
    }

    void EditorBlastSliceAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void EditorBlastSliceAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(azrtti_typeid<BlastSliceAsset>());
    }

    void EditorBlastSliceAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, azrtti_typeid<BlastSliceAsset>());
        AZ::AssetTypeInfoBus::Handler::BusConnect(azrtti_typeid<BlastSliceAsset>());
    }

    void EditorBlastSliceAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(azrtti_typeid<BlastSliceAsset>());
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType EditorBlastSliceAssetHandler::GetAssetType() const
    {
        return azrtti_typeid<BlastSliceAsset>();
    }

    const char* EditorBlastSliceAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Blast Slice Asset";
    }

    const char* EditorBlastSliceAssetHandler::GetGroup() const
    {
        return "Blast";
    }

    const char* EditorBlastSliceAssetHandler::GetBrowserIcon() const
    {
        return "Icons/Components/Box.png";
    }

    void EditorBlastSliceAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("blast_slice");
    }

} // namespace Blast
