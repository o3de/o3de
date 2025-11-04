/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetCollectionAsyncLoaderTestComponent.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

// Included so we can deduce the asset type from asset paths.
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace AtomBridge
    {
        [[maybe_unused]] static constexpr char AssetCollectionAsyncLoaderTestComponentName[] = " AssetCollectionAsyncLoaderTestComponent";

        void AssetCollectionAsyncLoaderTestComponent::Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AssetCollectionAsyncLoaderTestComponent, EditorComponentBase>()
                    ->Version(1)
                    ->Field("AssetListPathJson", &AssetCollectionAsyncLoaderTestComponent::m_pathToAssetListJson)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<AssetCollectionAsyncLoaderTestComponent>(
                        "AssetCollectionAsyncLoaderTest", "The AssetCollectionAsyncLoaderTest component allows you to test the API provided by AssetCollectionAsyncLoader")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Test")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Comment.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Comment.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level"), AZ_CRC_CE("Game"), AZ_CRC_CE("Layer") }))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::LineEdit, &AssetCollectionAsyncLoaderTestComponent::m_pathToAssetListJson, "", "Path To Asset List")
                            ->Attribute(AZ::Edit::Attributes::PlaceholderText, "Path to a JSON file")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", "Starts/Stop the test")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AssetCollectionAsyncLoaderTestComponent::OnStartCancelButtonClicked)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, &AssetCollectionAsyncLoaderTestComponent::GetStartCancelButtonText);
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<AssetCollectionAsyncLoaderTestBus>("AssetCollectionAsyncLoaderTestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Test")
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Event("StartLoadingAssetsFromJsonFile", &AssetCollectionAsyncLoaderTestBus::Events::StartLoadingAssetsFromJsonFile)
                    ->Event("StartLoadingAssetsFromAssetList", &AssetCollectionAsyncLoaderTestBus::Events::StartLoadingAssetsFromAssetList)
                    ->Event("CancelLoadingAssets", &AssetCollectionAsyncLoaderTestBus::Events::CancelLoadingAssets)
                    ->Event("GetPendingAssetsList", &AssetCollectionAsyncLoaderTestBus::Events::GetPendingAssetsList)
                    ->Event("GetCountOfPendingAssets", &AssetCollectionAsyncLoaderTestBus::Events::GetCountOfPendingAssets)
                    ->Event("ValidateAssetWasLoaded", &AssetCollectionAsyncLoaderTestBus::Events::ValidateAssetWasLoaded)
                    ;
            }

        }

        void AssetCollectionAsyncLoaderTestComponent::Activate()
        {
            m_assetCollectionAsyncLoader = AZStd::make_unique<AZ::AssetCollectionAsyncLoader>();
            AssetCollectionAsyncLoaderTestBus::Handler::BusConnect(GetEntityId());
        }

        void AssetCollectionAsyncLoaderTestComponent::Deactivate()
        {
            m_assetCollectionAsyncLoader = nullptr;
            AssetCollectionAsyncLoaderTestBus::Handler::BusDisconnect();
        }

        AZ::Crc32 AssetCollectionAsyncLoaderTestComponent::OnStartCancelButtonClicked()
        {
            switch (m_state)
            {
            case State::LoadingAssets:
                CancelLoadingAssets();
                break;
            default:
                StartLoadingAssetsFromJsonFile(m_pathToAssetListJson);
                break;
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZStd::string AssetCollectionAsyncLoaderTestComponent::GetStartCancelButtonText() const
        {
            switch (m_state)
            {
            case State::LoadingAssets:
                return "Cancel Loading Assets";
            }
            return "Start Loading Assets";
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCollectionAsyncLoaderTestBus overrides
        bool AssetCollectionAsyncLoaderTestComponent::StartLoadingAssetsFromJsonFile(const AZStd::string& pathToAssetListJson)
        {
            rapidjson::Document jsonDoc;

            auto readJsonResult = JsonSerializationUtils::ReadJsonFile(pathToAssetListJson, AZ::RPI::JsonUtils::DefaultMaxFileSize);

            if (!readJsonResult.IsSuccess())
            {
                AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "Failed to parse asset list json file %s", pathToAssetListJson.c_str());
                return false;
            }

            jsonDoc = readJsonResult.TakeValue();

            AZStd::vector<AZStd::string> assetList;
            for (rapidjson::Value::ConstValueIterator itr = jsonDoc.Begin(); itr != jsonDoc.End(); ++itr)
            {
                AZStd::string_view assetPath = itr->GetString();
                AZ_TracePrintf(AssetCollectionAsyncLoaderTestComponentName, "Asset path: %s\n", assetPath.data());
                assetList.push_back(assetPath);
            }

            return StartLoadingAssetsFromAssetList(assetList);
        }

        //! Helper method
        static Data::AssetType GetAssetTypeFromAssetPath(const AZStd::string& assetPath)
        {
            AZStd::string extension;
            if (!AzFramework::StringFunc::Path::GetExtension(assetPath.c_str(), extension, false /*include dot*/))
            {
                AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "Failed to get extension from path: %s", assetPath.c_str());
                return {};
            }

            if (extension == "azshader")
            {
                return azrtti_typeid<RPI::ShaderAsset>();
            }
            else if (extension == "azmodel")
            {
                return azrtti_typeid<RPI::ModelAsset>();
            }
            else if (extension == "streamingimage")
            {
                return azrtti_typeid<RPI::StreamingImageAsset>();
            }

            AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "Do not know the asset type for file: %s", assetPath.c_str());
            return {};
        }

        bool AssetCollectionAsyncLoaderTestComponent::StartLoadingAssetsFromAssetList(const AZStd::vector<AZStd::string>& assetList)
        {
            if (assetList.empty())
            {
                AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "Input asset list is empty");
                return false;
            }

            // Build the list with asset types deduced from the file extensions.
            AZStd::vector<AssetCollectionAsyncLoader::AssetToLoadInfo> assetListWithType;
            assetListWithType.reserve(assetList.size());
            for (const auto& assetPath : assetList)
            {
                const auto assetType = GetAssetTypeFromAssetPath(assetPath);
                assetListWithType.push_back({ assetPath, assetType });
                m_pendingAssets.insert(assetPath);
            }

            m_assetCollectionAsyncLoader->LoadAssetsAsync(assetListWithType,
                [&](AZStd::string_view assetPath, [[maybe_unused]] bool success, [[maybe_unused]] size_t pendingAssetCount)
                {
                    AZ_TracePrintf(AssetCollectionAsyncLoaderTestComponentName, "Got asset load [%s] for asset [%s]. Pending asset count [%zu]\n"
                        , success ? "SUCCESS" : "ERROR", assetPath.data(), pendingAssetCount);
                    switch (m_state)
                    {
                    case State::LoadingAssets:
                    {
                        if (m_pendingAssets.count(assetPath))
                        {
                            m_pendingAssets.erase(assetPath);
                            if (!m_pendingAssets.size())
                            {
                                AZ_TracePrintf(AssetCollectionAsyncLoaderTestComponentName, "Asset Loading Is Successfully Complete\n");
                                m_state = State::Idle;
                            }
                        }
                        else
                        {
                            AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "While loading assets, got asset update from an unexpected asset with path: %s", assetPath.data());
                            m_assetCollectionAsyncLoader->Cancel();
                            m_state = State::FatalError;
                        }
                    }
                    break;
                    default:
                        AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "Got asset update from an unexpected asset with path: %s", assetPath.data());
                        m_state = State::FatalError;
                    break;
                    }
                });

            m_state = State::LoadingAssets;
            return true;
        }

        void AssetCollectionAsyncLoaderTestComponent::CancelLoadingAssets()
        {
            m_assetCollectionAsyncLoader->Cancel();
            m_pendingAssets.clear();
            m_state = State::Idle;
        }

        AZStd::vector<AZStd::string> AssetCollectionAsyncLoaderTestComponent::GetPendingAssetsList() const
        {
            AZStd::vector<AZStd::string> retList;
            retList.reserve(m_pendingAssets.size());
            AZStd::for_each(m_pendingAssets.begin(), m_pendingAssets.end(),
                [&](const AZStd::string& assetPath)
                {
                    retList.push_back(assetPath);
                });
            return retList;
        }

        uint32_t AssetCollectionAsyncLoaderTestComponent::GetCountOfPendingAssets() const
        {
            return static_cast<uint32_t>(m_pendingAssets.size());
        }

        bool AssetCollectionAsyncLoaderTestComponent::ValidateAssetWasLoaded(const AZStd::string& assetPath) const
        {
            Data::AssetType assetType = GetAssetTypeFromAssetPath(assetPath);
            if (assetType == azrtti_typeid<RPI::ShaderAsset>())
            {
                auto asset = m_assetCollectionAsyncLoader->GetAsset<RPI::ShaderAsset>(assetPath);
                return (bool)asset && asset.GetId().IsValid() && asset.IsReady() && !asset->GetName().IsEmpty();

            }
            else if (assetType == azrtti_typeid<RPI::ModelAsset>())
            {
                auto asset = m_assetCollectionAsyncLoader->GetAsset<RPI::ModelAsset>(assetPath);
                return (bool)asset && asset.GetId().IsValid() && asset.IsReady() && asset->GetLodCount();
            }
            else if (assetType == azrtti_typeid<RPI::StreamingImageAsset>())
            {
                auto asset = m_assetCollectionAsyncLoader->GetAsset<RPI::StreamingImageAsset>(assetPath);
                return (bool)asset && asset.GetId().IsValid() && asset.IsReady() && asset->GetTotalImageDataSize();
            }

            AZ_Error(AssetCollectionAsyncLoaderTestComponentName, false, "Can not handle asset type for assetPath: %s", assetPath.c_str());
            return false;
        }

        //////////////////////////////////////////////////////////////////////////


    } //namespace AtomBridge
} // namespace AZ
