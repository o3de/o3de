/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Viewport/MaterialCanvasViewportComponent.h>
#include <Viewport/MaterialCanvasViewportNotificationBus.h>
#include <Viewport/MaterialCanvasViewportSettings.h>

namespace MaterialCanvas
{
    MaterialCanvasViewportComponent::MaterialCanvasViewportComponent()
    {
    }

    void MaterialCanvasViewportComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialCanvasViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialCanvasViewportComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<MaterialCanvasViewportComponent>("MaterialCanvasViewport", "Manages configurations for lighting and models displayed in the viewport")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialCanvasViewportRequestBus>("MaterialCanvasViewportRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Event("ReloadContent", &MaterialCanvasViewportRequestBus::Events::ReloadContent)
                ->Event("AddLightingPreset", &MaterialCanvasViewportRequestBus::Events::AddLightingPreset)
                ->Event("SaveLightingPreset", &MaterialCanvasViewportRequestBus::Events::SaveLightingPreset)
                ->Event("GetLightingPresets", &MaterialCanvasViewportRequestBus::Events::GetLightingPresets)
                ->Event("GetLightingPresetByName", &MaterialCanvasViewportRequestBus::Events::GetLightingPresetByName)
                ->Event("GetLightingPresetSelection", &MaterialCanvasViewportRequestBus::Events::GetLightingPresetSelection)
                ->Event("SelectLightingPreset", &MaterialCanvasViewportRequestBus::Events::SelectLightingPreset)
                ->Event("SelectLightingPresetByName", &MaterialCanvasViewportRequestBus::Events::SelectLightingPresetByName)
                ->Event("GetLightingPresetNames", &MaterialCanvasViewportRequestBus::Events::GetLightingPresetNames)
                ->Event("GetLightingPresetLastSavePath", &MaterialCanvasViewportRequestBus::Events::GetLightingPresetLastSavePath)
                ->Event("AddModelPreset", &MaterialCanvasViewportRequestBus::Events::AddModelPreset)
                ->Event("SaveModelPreset", &MaterialCanvasViewportRequestBus::Events::SaveModelPreset)
                ->Event("GetModelPresets", &MaterialCanvasViewportRequestBus::Events::GetModelPresets)
                ->Event("GetModelPresetByName", &MaterialCanvasViewportRequestBus::Events::GetModelPresetByName)
                ->Event("GetModelPresetSelection", &MaterialCanvasViewportRequestBus::Events::GetModelPresetSelection)
                ->Event("SelectModelPreset", &MaterialCanvasViewportRequestBus::Events::SelectModelPreset)
                ->Event("SelectModelPresetByName", &MaterialCanvasViewportRequestBus::Events::SelectModelPresetByName)
                ->Event("GetModelPresetNames", &MaterialCanvasViewportRequestBus::Events::GetModelPresetNames)
                ->Event("GetModelPresetLastSavePath", &MaterialCanvasViewportRequestBus::Events::GetModelPresetLastSavePath)
                ->Event("SetShadowCatcherEnabled", &MaterialCanvasViewportRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &MaterialCanvasViewportRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &MaterialCanvasViewportRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &MaterialCanvasViewportRequestBus::Events::GetGridEnabled)
                ->Event("SetAlternateSkyboxEnabled", &MaterialCanvasViewportRequestBus::Events::SetAlternateSkyboxEnabled)
                ->Event("GetAlternateSkyboxEnabled", &MaterialCanvasViewportRequestBus::Events::GetAlternateSkyboxEnabled)
                ->Event("SetFieldOfView", &MaterialCanvasViewportRequestBus::Events::SetFieldOfView)
                ->Event("GetFieldOfView", &MaterialCanvasViewportRequestBus::Events::GetFieldOfView)
                ;

            behaviorContext->EBus<MaterialCanvasViewportNotificationBus>("MaterialCanvasViewportNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Event("OnLightingPresetAdded", &MaterialCanvasViewportNotificationBus::Events::OnLightingPresetAdded)
                ->Event("OnLightingPresetSelected", &MaterialCanvasViewportNotificationBus::Events::OnLightingPresetSelected)
                ->Event("OnLightingPresetChanged", &MaterialCanvasViewportNotificationBus::Events::OnLightingPresetChanged)
                ->Event("OnModelPresetAdded", &MaterialCanvasViewportNotificationBus::Events::OnModelPresetAdded)
                ->Event("OnModelPresetSelected", &MaterialCanvasViewportNotificationBus::Events::OnModelPresetSelected)
                ->Event("OnModelPresetChanged", &MaterialCanvasViewportNotificationBus::Events::OnModelPresetChanged)
                ->Event("OnShadowCatcherEnabledChanged", &MaterialCanvasViewportNotificationBus::Events::OnShadowCatcherEnabledChanged)
                ->Event("OnGridEnabledChanged", &MaterialCanvasViewportNotificationBus::Events::OnGridEnabledChanged)
                ->Event("OnAlternateSkyboxEnabledChanged", &MaterialCanvasViewportNotificationBus::Events::OnAlternateSkyboxEnabledChanged)
                ->Event("OnFieldOfViewChanged", &MaterialCanvasViewportNotificationBus::Events::OnFieldOfViewChanged)
                ;
        }
    }

    void MaterialCanvasViewportComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
        required.push_back(AZ_CRC_CE("PerformanceMonitorService"));
    }

    void MaterialCanvasViewportComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialCanvasViewportService"));
    }

    void MaterialCanvasViewportComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialCanvasViewportService"));
    }

    void MaterialCanvasViewportComponent::Init()
    {
    }

    void MaterialCanvasViewportComponent::Activate()
    {
        m_viewportSettings =
            AZ::UserSettings::CreateFind<MaterialCanvasViewportSettings>(AZ::Crc32("MaterialCanvasViewportSettings"), AZ::UserSettings::CT_GLOBAL);

        MaterialCanvasViewportRequestBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void MaterialCanvasViewportComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialCanvasViewportRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void MaterialCanvasViewportComponent::ClearContent()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        m_lightingPresetAssets.clear();
        m_lightingPresetVector.clear();
        m_lightingPresetLastSavePathMap.clear();
        m_lightingPresetSelection.reset();

        m_modelPresetAssets.clear();
        m_modelPresetVector.clear();
        m_modelPresetLastSavePathMap.clear();
        m_modelPresetSelection.reset();
    }

    void MaterialCanvasViewportComponent::ReloadContent()
    {
        AZ_TracePrintf("Material Canvas", "Started loading viewport configurations.\n");

        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnBeginReloadContent);

        ClearContent();

        // Enumerate and load all the relevant preset files in the project.
        // (The files are stored in a temporary list instead of processed in the callback because deep operations inside
        // AssetCatalogRequestBus::EnumerateAssets can lead to deadlocked)
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB = [this]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AZ::StringFunc::EndsWith(info.m_relativePath.c_str(), ".lightingpreset.azasset"))
            {
                m_lightingPresetAssets[info.m_assetId] = { info.m_assetId, info.m_assetType };
                AZ::Data::AssetBus::MultiHandler::BusConnect(info.m_assetId);
                return;
            }

            if (AZ::StringFunc::EndsWith(info.m_relativePath.c_str(), ".modelpreset.azasset"))
            {
                m_modelPresetAssets[info.m_assetId] = { info.m_assetId, info.m_assetType };
                AZ::Data::AssetBus::MultiHandler::BusConnect(info.m_assetId);
                return;
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        for (auto& assetPair : m_lightingPresetAssets)
        {
            assetPair.second.QueueLoad();
        }

        for (auto& assetPair : m_modelPresetAssets)
        {
            assetPair.second.QueueLoad();
        }
    }

    AZ::Render::LightingPresetPtr MaterialCanvasViewportComponent::AddLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPresetVector.push_back(AZStd::make_shared<AZ::Render::LightingPreset>(preset));
        auto presetPtr = m_lightingPresetVector.back();

        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnLightingPresetAdded, presetPtr);
        return presetPtr;
    }

    AZ::Render::LightingPresetPtr MaterialCanvasViewportComponent::GetLightingPresetByName(const AZStd::string& name) const
    {
        const auto presetItr = AZStd::find_if(m_lightingPresetVector.begin(), m_lightingPresetVector.end(), [&name](const auto& preset) {
            return preset && preset->m_displayName == name; });
        return presetItr != m_lightingPresetVector.end() ? *presetItr : nullptr;
    }

    AZ::Render::LightingPresetPtrVector MaterialCanvasViewportComponent::GetLightingPresets() const
    {
        return m_lightingPresetVector;
    }

    bool MaterialCanvasViewportComponent::SaveLightingPreset(AZ::Render::LightingPresetPtr preset, const AZStd::string& path) const
    {
        if (preset && AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::LightingPreset>(preset.get(), path).IsSuccess())
        {
            m_lightingPresetLastSavePathMap[preset] = path;
            return true;
        }

        return false;
    }

    AZ::Render::LightingPresetPtr MaterialCanvasViewportComponent::GetLightingPresetSelection() const
    {
        return m_lightingPresetSelection;
    }

    void MaterialCanvasViewportComponent::SelectLightingPreset(AZ::Render::LightingPresetPtr preset)
    {
        if (preset)
        {
            m_lightingPresetSelection = preset;
            m_viewportSettings->m_selectedLightingPresetName = preset->m_displayName;
            MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnLightingPresetSelected, m_lightingPresetSelection);
        }
    }

    void MaterialCanvasViewportComponent::SelectLightingPresetByName(const AZStd::string& name)
    {
        SelectLightingPreset(GetLightingPresetByName(name));
    }

    MaterialCanvasViewportPresetNameSet MaterialCanvasViewportComponent::GetLightingPresetNames() const
    {
        MaterialCanvasViewportPresetNameSet names;
        for (const auto& preset : m_lightingPresetVector)
        {
            if (preset)
            {
                names.insert(preset->m_displayName);
            }
        }
        return names;
    }

    AZStd::string MaterialCanvasViewportComponent::GetLightingPresetLastSavePath(AZ::Render::LightingPresetPtr preset) const
    {
        auto pathItr = m_lightingPresetLastSavePathMap.find(preset);
        return pathItr != m_lightingPresetLastSavePathMap.end() ? pathItr->second : AZStd::string();
    }

    AZ::Render::ModelPresetPtr MaterialCanvasViewportComponent::AddModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPresetVector.push_back(AZStd::make_shared<AZ::Render::ModelPreset>(preset));
        auto presetPtr = m_modelPresetVector.back();

        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnModelPresetAdded, presetPtr);
        return presetPtr;
    }

    AZ::Render::ModelPresetPtr MaterialCanvasViewportComponent::GetModelPresetByName(const AZStd::string& name) const
    {
        const auto presetItr = AZStd::find_if(m_modelPresetVector.begin(), m_modelPresetVector.end(), [&name](const auto& preset) {
            return preset && preset->m_displayName == name; });
        return presetItr != m_modelPresetVector.end() ? *presetItr : nullptr;
    }

    AZ::Render::ModelPresetPtrVector MaterialCanvasViewportComponent::GetModelPresets() const
    {
        return m_modelPresetVector;
    }

    bool MaterialCanvasViewportComponent::SaveModelPreset(AZ::Render::ModelPresetPtr preset, const AZStd::string& path) const
    {
        if (preset && AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::ModelPreset>(preset.get(), path).IsSuccess())
        {
            m_modelPresetLastSavePathMap[preset] = path;
            return true;
        }

        return false;
    }

    AZ::Render::ModelPresetPtr MaterialCanvasViewportComponent::GetModelPresetSelection() const
    {
        return m_modelPresetSelection;
    }

    void MaterialCanvasViewportComponent::SelectModelPreset(AZ::Render::ModelPresetPtr preset)
    {
        if (preset)
        {
            m_modelPresetSelection = preset;
            m_viewportSettings->m_selectedModelPresetName = preset->m_displayName;
            MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnModelPresetSelected, m_modelPresetSelection);
        }
    }

    void MaterialCanvasViewportComponent::SelectModelPresetByName(const AZStd::string& name)
    {
        SelectModelPreset(GetModelPresetByName(name));
    }

    MaterialCanvasViewportPresetNameSet MaterialCanvasViewportComponent::GetModelPresetNames() const
    {
        MaterialCanvasViewportPresetNameSet names;
        for (const auto& preset : m_modelPresetVector)
        {
            if (preset)
            {
                names.insert(preset->m_displayName);
            }
        }
        return names;
    }

    AZStd::string MaterialCanvasViewportComponent::GetModelPresetLastSavePath(AZ::Render::ModelPresetPtr preset) const
    {
        auto pathItr = m_modelPresetLastSavePathMap.find(preset);
        return pathItr != m_modelPresetLastSavePathMap.end() ? pathItr->second : AZStd::string();
    }

    void MaterialCanvasViewportComponent::SetShadowCatcherEnabled(bool enable)
    {
        m_viewportSettings->m_enableShadowCatcher = enable;
        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnShadowCatcherEnabledChanged, enable);
    }

    bool MaterialCanvasViewportComponent::GetShadowCatcherEnabled() const
    {
        return m_viewportSettings->m_enableShadowCatcher;
    }

    void MaterialCanvasViewportComponent::SetGridEnabled(bool enable)
    {
        m_viewportSettings->m_enableGrid = enable;
        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnGridEnabledChanged, enable);
    }


    bool MaterialCanvasViewportComponent::GetGridEnabled() const
    {
        return m_viewportSettings->m_enableGrid;
    }

    void MaterialCanvasViewportComponent::SetAlternateSkyboxEnabled(bool enable)
    {
        m_viewportSettings->m_enableAlternateSkybox = enable;
        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnAlternateSkyboxEnabledChanged, enable);
    }


    bool MaterialCanvasViewportComponent::GetAlternateSkyboxEnabled() const
    {
        return m_viewportSettings->m_enableAlternateSkybox;
    }

    void MaterialCanvasViewportComponent::SetFieldOfView(float fieldOfView)
    {
        m_viewportSettings->m_fieldOfView = fieldOfView;
        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnFieldOfViewChanged, fieldOfView);
    }


    float MaterialCanvasViewportComponent::GetFieldOfView() const
    {
        return m_viewportSettings->m_fieldOfView;
    }

    void MaterialCanvasViewportComponent::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        m_viewportSettings->m_displayMapperOperationType = operationType;
        MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnDisplayMapperOperationTypeChanged, operationType);
    }

    AZ::Render::DisplayMapperOperationType MaterialCanvasViewportComponent::GetDisplayMapperOperationType() const
    {
        return m_viewportSettings->m_displayMapperOperationType;
    }

    inline void MaterialCanvasViewportComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (AZ::Data::Asset<AZ::RPI::AnyAsset> anyAsset = asset)
        {
            if (const auto lightingPreset = anyAsset->GetDataAs<AZ::Render::LightingPreset>())
            {
                auto presetPtr = AddLightingPreset(*lightingPreset);
                const auto& presetPath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(anyAsset.GetId());
                m_lightingPresetAssets[anyAsset.GetId()] = anyAsset;
                m_lightingPresetLastSavePathMap[presetPtr] = presetPath;
                AZ_TracePrintf("Material Canvas", "Loaded Preset: %s\n", presetPath.c_str());
            }

            if (const auto modelPreset = anyAsset->GetDataAs<AZ::Render::ModelPreset>())
            {
                auto presetPtr = AddModelPreset(*modelPreset);
                const auto& presetPath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(anyAsset.GetId());
                m_modelPresetAssets[anyAsset.GetId()] = anyAsset;
                m_modelPresetLastSavePathMap[presetPtr] = presetPath;
                AZ_TracePrintf("Material Canvas", "Loaded Preset: %s\n", presetPath.c_str());
            }
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
        if (!AZ::Data::AssetBus::MultiHandler::BusIsConnected())
        {
            SelectLightingPresetByName(m_viewportSettings->m_selectedLightingPresetName);
            SelectModelPresetByName(m_viewportSettings->m_selectedModelPresetName);
            MaterialCanvasViewportNotificationBus::Broadcast(&MaterialCanvasViewportNotificationBus::Events::OnEndReloadContent);
            AZ_TracePrintf("Material Canvas", "Finished loading viewport configurations.\n");
        }
    }

    void MaterialCanvasViewportComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        AZ::TickBus::QueueFunction([this]() {
            ReloadContent();
        });
    }

    void MaterialCanvasViewportComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        auto ReloadLightingAndModelPresets = [this, &assetId](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
        {
            AZ::Data::AssetInfo assetInfo = assetCatalogRequests->GetAssetInfoById(assetId);
            if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".lightingpreset.azasset"))
            {
                m_lightingPresetAssets[assetInfo.m_assetId] = { assetInfo.m_assetId, assetInfo.m_assetType };
                m_lightingPresetAssets[assetInfo.m_assetId].QueueLoad();
                AZ::Data::AssetBus::MultiHandler::BusConnect(assetInfo.m_assetId);
                return;
            }

            if (AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".modelpreset.azasset"))
            {
                m_modelPresetAssets[assetInfo.m_assetId] = { assetInfo.m_assetId, assetInfo.m_assetType };
                m_modelPresetAssets[assetInfo.m_assetId].QueueLoad();
                AZ::Data::AssetBus::MultiHandler::BusConnect(assetInfo.m_assetId);
                return;
            }
        };
        AZ::Data::AssetCatalogRequestBus::Broadcast(AZStd::move(ReloadLightingAndModelPresets));
    }

    void MaterialCanvasViewportComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        OnCatalogAssetChanged(assetId);
    }

    void MaterialCanvasViewportComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo)
    {
        if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".lightingpreset.azasset"))
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetInfo.m_assetId);
            m_lightingPresetAssets.erase(assetId);
            return;
        }

        if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".modelpreset.azasset"))
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetInfo.m_assetId);
            m_modelPresetAssets.erase(assetId);
            return;
        }
    }
}
