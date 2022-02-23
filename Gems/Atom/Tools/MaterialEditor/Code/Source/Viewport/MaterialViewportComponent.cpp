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
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Viewport/MaterialViewportComponent.h>
#include <Viewport/MaterialViewportNotificationBus.h>
#include <Viewport/MaterialViewportSettings.h>

namespace MaterialEditor
{
    void MaterialViewportComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialViewportComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<MaterialViewportComponent>("MaterialViewport", "Manages configurations for lighting and models displayed in the viewport")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialViewportRequestBus>("MaterialViewportRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("SetLightingPreset", &MaterialViewportRequestBus::Events::SetLightingPreset)
                ->Event("GetLightingPreset", &MaterialViewportRequestBus::Events::GetLightingPreset)
                ->Event("SaveLightingPreset", &MaterialViewportRequestBus::Events::SaveLightingPreset)
                ->Event("LoadLightingPreset", &MaterialViewportRequestBus::Events::LoadLightingPreset)
                ->Event("LoadLightingPresetByAssetId", &MaterialViewportRequestBus::Events::LoadLightingPresetByAssetId)
                ->Event("GetLastLightingPresetPath", &MaterialViewportRequestBus::Events::GetLastLightingPresetPath)
                ->Event("GetLastLightingPresetAssetId", &MaterialViewportRequestBus::Events::GetLastLightingPresetAssetId)
                ->Event("SetModelPreset", &MaterialViewportRequestBus::Events::SetModelPreset)
                ->Event("GetModelPreset", &MaterialViewportRequestBus::Events::GetModelPreset)
                ->Event("SaveModelPreset", &MaterialViewportRequestBus::Events::SaveModelPreset)
                ->Event("LoadModelPreset", &MaterialViewportRequestBus::Events::LoadModelPreset)
                ->Event("LoadModelPresetByAssetId", &MaterialViewportRequestBus::Events::LoadModelPresetByAssetId)
                ->Event("GetLastModelPresetPath", &MaterialViewportRequestBus::Events::GetLastModelPresetPath)
                ->Event("GetLastModelPresetAssetId", &MaterialViewportRequestBus::Events::GetLastModelPresetAssetId)
                ->Event("SetShadowCatcherEnabled", &MaterialViewportRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &MaterialViewportRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &MaterialViewportRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &MaterialViewportRequestBus::Events::GetGridEnabled)
                ->Event("SetAlternateSkyboxEnabled", &MaterialViewportRequestBus::Events::SetAlternateSkyboxEnabled)
                ->Event("GetAlternateSkyboxEnabled", &MaterialViewportRequestBus::Events::GetAlternateSkyboxEnabled)
                ->Event("SetFieldOfView", &MaterialViewportRequestBus::Events::SetFieldOfView)
                ->Event("GetFieldOfView", &MaterialViewportRequestBus::Events::GetFieldOfView)
                ;

            behaviorContext->EBus<MaterialViewportNotificationBus>("MaterialViewportNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("OnLightingPresetAdded", &MaterialViewportNotificationBus::Events::OnLightingPresetAdded)
                ->Event("OnLightingPresetSelected", &MaterialViewportNotificationBus::Events::OnLightingPresetSelected)
                ->Event("OnLightingPresetChanged", &MaterialViewportNotificationBus::Events::OnLightingPresetChanged)
                ->Event("OnModelPresetAdded", &MaterialViewportNotificationBus::Events::OnModelPresetAdded)
                ->Event("OnModelPresetSelected", &MaterialViewportNotificationBus::Events::OnModelPresetSelected)
                ->Event("OnModelPresetChanged", &MaterialViewportNotificationBus::Events::OnModelPresetChanged)
                ;
        }
    }

    MaterialViewportComponent::MaterialViewportComponent()
    {
    }

    void MaterialViewportComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
        required.push_back(AZ_CRC_CE("PerformanceMonitorService"));
    }

    void MaterialViewportComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialViewportService"));
    }

    void MaterialViewportComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialViewportService"));
    }

    void MaterialViewportComponent::Init()
    {
    }

    void MaterialViewportComponent::Activate()
    {
        ClearContent();
        MaterialViewportRequestBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void MaterialViewportComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialViewportRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void MaterialViewportComponent::ClearContent()
    {
        m_lightingPresetCache.clear();
        m_lightingPreset = {};

        m_modelPresetCache.clear();
        m_modelPreset = {};

        m_settingsNotificationPending = {};
    }

    void MaterialViewportComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_settingsNotificationPending)
        {
            m_settingsNotificationPending = false;
            MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnViewportSettingsChanged);
        }
    }

    void MaterialViewportComponent::SetLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::LightingPreset& MaterialViewportComponent::GetLightingPreset() const
    {
        return m_lightingPreset;
    }

    bool MaterialViewportComponent::SaveLightingPreset(const AZStd::string& path) const
    {
        if (AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::LightingPreset>(&m_lightingPreset, path))
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPresetCache[path] = m_lightingPreset;
            return true;
        }
        return false;
    }

    bool MaterialViewportComponent::LoadLightingPreset(const AZStd::string& path)
    {
        auto cacheItr = m_lightingPresetCache.find(path);
        if (cacheItr != m_lightingPresetCache.end())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (AZ::JsonSerializationUtils::LoadObjectFromFile<AZ::Render::LightingPreset>(m_lightingPreset, path))
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPresetCache[path] = m_lightingPreset;
            m_settingsNotificationPending = true;
            return true;
        }
        return false;
    }

    bool MaterialViewportComponent::LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadLightingPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string MaterialViewportComponent::GetLastLightingPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastLightingPresetAssetId());
    }

    AZ::Data::AssetId MaterialViewportComponent::GetLastLightingPresetAssetId() const
    {
        return AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/lightingpresets/neutral_urban.lightingpreset.azasset"));
    }

    void MaterialViewportComponent::SetModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::ModelPreset& MaterialViewportComponent::GetModelPreset() const
    {
        return m_modelPreset;
    }

    bool MaterialViewportComponent::SaveModelPreset(const AZStd::string& path) const
    {
        if (AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::ModelPreset>(&m_modelPreset, path))
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPresetCache[path] = m_modelPreset;
            return true;
        }
        return false;
    }

    bool MaterialViewportComponent::LoadModelPreset(const AZStd::string& path)
    {
        auto cacheItr = m_modelPresetCache.find(path);
        if (cacheItr != m_modelPresetCache.end())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (AZ::JsonSerializationUtils::LoadObjectFromFile<AZ::Render::ModelPreset>(m_modelPreset, path))
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPresetCache[path] = m_modelPreset;
            m_settingsNotificationPending = true;
            return true;
        }
        return false;
    }

    bool MaterialViewportComponent::LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadModelPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string MaterialViewportComponent::GetLastModelPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastModelPresetAssetId());
    }

    AZ::Data::AssetId MaterialViewportComponent::GetLastModelPresetAssetId() const
    {
        return AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/viewportmodels/shaderball.modelpreset.azasset"));
    }

    void MaterialViewportComponent::SetShadowCatcherEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableShadowCatcher", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialViewportComponent::GetShadowCatcherEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableShadowCatcher", true);
    }

    void MaterialViewportComponent::SetGridEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableGrid", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialViewportComponent::GetGridEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableGrid", true);
    }

    void MaterialViewportComponent::SetAlternateSkyboxEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableAlternateSkybox", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialViewportComponent::GetAlternateSkyboxEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableAlternateSkybox", false);
    }

    void MaterialViewportComponent::SetFieldOfView(float fieldOfView)
    {
        AtomToolsFramework::SetSettingsValue<double>(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/FieldOfView", aznumeric_cast<double>(fieldOfView));
        m_settingsNotificationPending = true;
    }

    float MaterialViewportComponent::GetFieldOfView() const
    {
        return aznumeric_cast<float>(
            AtomToolsFramework::GetSettingsValue<double>("/O3DE/Atom/MaterialEditor/ViewportSettings/FieldOfView", 90.0));
    }

    void MaterialViewportComponent::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        AtomToolsFramework::SetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/DisplayMapperOperationType", aznumeric_cast<AZ::u64>(operationType));
        m_settingsNotificationPending = true;
    }

    AZ::Render::DisplayMapperOperationType MaterialViewportComponent::GetDisplayMapperOperationType() const
    {
        return aznumeric_cast<AZ::Render::DisplayMapperOperationType>(AtomToolsFramework::GetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/DisplayMapperOperationType",
            aznumeric_cast<AZ::u64>(AZ::Render::DisplayMapperOperationType::Aces)));
    }

    void MaterialViewportComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        ClearContent();
        LoadModelPreset(GetLastModelPresetPath());
        LoadLightingPreset(GetLastLightingPresetPath());
    }

    void MaterialViewportComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void MaterialViewportComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void MaterialViewportComponent::QueueLoadPresetCache(const AZ::Data::AssetInfo& assetInfo)
    {
        if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".lightingpreset.azasset"))
        {
            AZ::TickBus::QueueFunction([=]() {
                const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                AZ::JsonSerializationUtils::LoadObjectFromFile(m_lightingPresetCache[path], path);
            });
            return;
        }

        if (AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".modelpreset.azasset"))
        {
            AZ::TickBus::QueueFunction([=]() {
                const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                AZ::JsonSerializationUtils::LoadObjectFromFile(m_modelPresetCache[path], path);
            });
            return;
        }
    }
} // namespace MaterialEditor
