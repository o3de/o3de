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
#include <Viewport/MaterialViewportSettings.h>
#include <Viewport/MaterialViewportSettingsNotificationBus.h>
#include <Viewport/MaterialViewportSettingsSystem.h>

namespace MaterialEditor
{
    void MaterialViewportSettingsSystem::Reflect(AZ::ReflectContext* context)
    {
        MaterialViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialViewportSettingsSystem>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<MaterialViewportSettingsSystem>("MaterialViewportSettingsSystem", "Manages and serializes settings for the application viewport")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialViewportSettingsRequestBus>("MaterialViewportSettingsRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("SetLightingPreset", &MaterialViewportSettingsRequestBus::Events::SetLightingPreset)
                ->Event("GetLightingPreset", &MaterialViewportSettingsRequestBus::Events::GetLightingPreset)
                ->Event("SaveLightingPreset", &MaterialViewportSettingsRequestBus::Events::SaveLightingPreset)
                ->Event("LoadLightingPreset", &MaterialViewportSettingsRequestBus::Events::LoadLightingPreset)
                ->Event("LoadLightingPresetByAssetId", &MaterialViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId)
                ->Event("GetLastLightingPresetPath", &MaterialViewportSettingsRequestBus::Events::GetLastLightingPresetPath)
                ->Event("GetLastLightingPresetAssetId", &MaterialViewportSettingsRequestBus::Events::GetLastLightingPresetAssetId)
                ->Event("SetModelPreset", &MaterialViewportSettingsRequestBus::Events::SetModelPreset)
                ->Event("GetModelPreset", &MaterialViewportSettingsRequestBus::Events::GetModelPreset)
                ->Event("SaveModelPreset", &MaterialViewportSettingsRequestBus::Events::SaveModelPreset)
                ->Event("LoadModelPreset", &MaterialViewportSettingsRequestBus::Events::LoadModelPreset)
                ->Event("LoadModelPresetByAssetId", &MaterialViewportSettingsRequestBus::Events::LoadModelPresetByAssetId)
                ->Event("GetLastModelPresetPath", &MaterialViewportSettingsRequestBus::Events::GetLastModelPresetPath)
                ->Event("GetLastModelPresetAssetId", &MaterialViewportSettingsRequestBus::Events::GetLastModelPresetAssetId)
                ->Event("SetShadowCatcherEnabled", &MaterialViewportSettingsRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &MaterialViewportSettingsRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &MaterialViewportSettingsRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &MaterialViewportSettingsRequestBus::Events::GetGridEnabled)
                ->Event("SetAlternateSkyboxEnabled", &MaterialViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled)
                ->Event("GetAlternateSkyboxEnabled", &MaterialViewportSettingsRequestBus::Events::GetAlternateSkyboxEnabled)
                ->Event("SetFieldOfView", &MaterialViewportSettingsRequestBus::Events::SetFieldOfView)
                ->Event("GetFieldOfView", &MaterialViewportSettingsRequestBus::Events::GetFieldOfView)
                ;

            behaviorContext->EBus<MaterialViewportSettingsNotificationBus>("MaterialViewportSettingsNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("OnViewportSettingsChanged", &MaterialViewportSettingsNotificationBus::Events::OnViewportSettingsChanged)
                ;
        }
    }

    MaterialViewportSettingsSystem::MaterialViewportSettingsSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        MaterialViewportSettingsRequestBus::Handler::BusConnect(m_toolId);
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    MaterialViewportSettingsSystem::~MaterialViewportSettingsSystem()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialViewportSettingsRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void MaterialViewportSettingsSystem::ClearContent()
    {
        m_lightingPresetCache.clear();
        m_lightingPreset = {};

        m_modelPresetCache.clear();
        m_modelPreset = {};

        m_settingsNotificationPending = {};
    }

    void MaterialViewportSettingsSystem::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_settingsNotificationPending)
        {
            m_settingsNotificationPending = false;
            MaterialViewportSettingsNotificationBus::Event(
                m_toolId, &MaterialViewportSettingsNotificationBus::Events::OnViewportSettingsChanged);
        }
    }

    void MaterialViewportSettingsSystem::SetLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::LightingPreset& MaterialViewportSettingsSystem::GetLightingPreset() const
    {
        return m_lightingPreset;
    }

    bool MaterialViewportSettingsSystem::SaveLightingPreset(const AZStd::string& path) const
    {
        if (!path.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_lightingPreset, path).IsSuccess())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPresetCache[path] = m_lightingPreset;
            return true;
        }
        return false;
    }

    bool MaterialViewportSettingsSystem::LoadLightingPreset(const AZStd::string& path)
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

        if (!path.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
            {
                AtomToolsFramework::SetSettingsObject(
                    "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId",
                    AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
                m_lightingPresetCache[path] = m_lightingPreset = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
                m_settingsNotificationPending = true;
                return true;
            }
        }
        return false;
    }

    bool MaterialViewportSettingsSystem::LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadLightingPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string MaterialViewportSettingsSystem::GetLastLightingPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastLightingPresetAssetId());
    }

    AZ::Data::AssetId MaterialViewportSettingsSystem::GetLastLightingPresetAssetId() const
    {
        return AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/LightingPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/lightingpresets/neutral_urban.lightingpreset.azasset"));
    }

    void MaterialViewportSettingsSystem::SetModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::ModelPreset& MaterialViewportSettingsSystem::GetModelPreset() const
    {
        return m_modelPreset;
    }

    bool MaterialViewportSettingsSystem::SaveModelPreset(const AZStd::string& path) const
    {
        if (!path.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_modelPreset, path).IsSuccess())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPresetCache[path] = m_modelPreset;
            return true;
        }
        return false;
    }

    bool MaterialViewportSettingsSystem::LoadModelPreset(const AZStd::string& path)
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

        if (!path.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::Render::ModelPreset>())
            {
                AtomToolsFramework::SetSettingsObject(
                    "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId",
                    AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
                m_modelPresetCache[path] = m_modelPreset = AZStd::any_cast<AZ::Render::ModelPreset>(loadResult.GetValue());
                m_settingsNotificationPending = true;
                return true;
            }
        }
        return false;
    }

    bool MaterialViewportSettingsSystem::LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadModelPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string MaterialViewportSettingsSystem::GetLastModelPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastModelPresetAssetId());
    }

    AZ::Data::AssetId MaterialViewportSettingsSystem::GetLastModelPresetAssetId() const
    {
        return AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/ModelPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/viewportmodels/shaderball.modelpreset.azasset"));
    }

    void MaterialViewportSettingsSystem::SetShadowCatcherEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableShadowCatcher", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialViewportSettingsSystem::GetShadowCatcherEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableShadowCatcher", true);
    }

    void MaterialViewportSettingsSystem::SetGridEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableGrid", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialViewportSettingsSystem::GetGridEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableGrid", true);
    }

    void MaterialViewportSettingsSystem::SetAlternateSkyboxEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableAlternateSkybox", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialViewportSettingsSystem::GetAlternateSkyboxEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialEditor/ViewportSettings/EnableAlternateSkybox", false);
    }

    void MaterialViewportSettingsSystem::SetFieldOfView(float fieldOfView)
    {
        AtomToolsFramework::SetSettingsValue<double>(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/FieldOfView", aznumeric_cast<double>(fieldOfView));
        m_settingsNotificationPending = true;
    }

    float MaterialViewportSettingsSystem::GetFieldOfView() const
    {
        return aznumeric_cast<float>(
            AtomToolsFramework::GetSettingsValue<double>("/O3DE/Atom/MaterialEditor/ViewportSettings/FieldOfView", 90.0));
    }

    void MaterialViewportSettingsSystem::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        AtomToolsFramework::SetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/DisplayMapperOperationType", aznumeric_cast<AZ::u64>(operationType));
        m_settingsNotificationPending = true;
    }

    AZ::Render::DisplayMapperOperationType MaterialViewportSettingsSystem::GetDisplayMapperOperationType() const
    {
        return aznumeric_cast<AZ::Render::DisplayMapperOperationType>(AtomToolsFramework::GetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialEditor/ViewportSettings/DisplayMapperOperationType",
            aznumeric_cast<AZ::u64>(AZ::Render::DisplayMapperOperationType::Aces)));
    }

    void MaterialViewportSettingsSystem::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        ClearContent();
        LoadModelPreset(GetLastModelPresetPath());
        LoadLightingPreset(GetLastLightingPresetPath());
    }

    void MaterialViewportSettingsSystem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void MaterialViewportSettingsSystem::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void MaterialViewportSettingsSystem::QueueLoadPresetCache(const AZ::Data::AssetInfo& assetInfo)
    {
        if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".lightingpreset.azasset"))
        {
            AZ::TickBus::QueueFunction([=]() {
                const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                if (!path.empty())
                {
                    auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
                    if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
                    {
                        m_lightingPresetCache[path] = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
                        m_settingsNotificationPending = true;
                    }
                }
            });
            return;
        }

        if (AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".modelpreset.azasset"))
        {
            AZ::TickBus::QueueFunction([=]() {
                const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                if (!path.empty())
                {
                    auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
                    if (loadResult && loadResult.GetValue().is<AZ::Render::ModelPreset>())
                    {
                        m_modelPresetCache[path] = AZStd::any_cast<AZ::Render::ModelPreset>(loadResult.GetValue());
                        m_settingsNotificationPending = true;
                    }
                }
            });
            return;
        }
    }
} // namespace MaterialEditor
