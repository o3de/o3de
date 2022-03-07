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
#include <Atom/RPI.Reflect/System/AnyAsset.h>
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
#include <Viewport/MaterialCanvasViewportSettings.h>
#include <Viewport/MaterialCanvasViewportSettingsNotificationBus.h>
#include <Viewport/MaterialCanvasViewportSettingsSystem.h>

namespace MaterialCanvas
{
    void MaterialCanvasViewportSettingsSystem::Reflect(AZ::ReflectContext* context)
    {
        MaterialCanvasViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialCanvasViewportSettingsSystem>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<MaterialCanvasViewportSettingsSystem>("MaterialCanvasViewportSettingsSystem", "Manages and serializes settings for the application viewport")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialCanvasViewportSettingsRequestBus>("MaterialCanvasViewportSettingsRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Event("SetLightingPreset", &MaterialCanvasViewportSettingsRequestBus::Events::SetLightingPreset)
                ->Event("GetLightingPreset", &MaterialCanvasViewportSettingsRequestBus::Events::GetLightingPreset)
                ->Event("SaveLightingPreset", &MaterialCanvasViewportSettingsRequestBus::Events::SaveLightingPreset)
                ->Event("LoadLightingPreset", &MaterialCanvasViewportSettingsRequestBus::Events::LoadLightingPreset)
                ->Event("LoadLightingPresetByAssetId", &MaterialCanvasViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId)
                ->Event("GetLastLightingPresetPath", &MaterialCanvasViewportSettingsRequestBus::Events::GetLastLightingPresetPath)
                ->Event("GetLastLightingPresetAssetId", &MaterialCanvasViewportSettingsRequestBus::Events::GetLastLightingPresetAssetId)
                ->Event("SetModelPreset", &MaterialCanvasViewportSettingsRequestBus::Events::SetModelPreset)
                ->Event("GetModelPreset", &MaterialCanvasViewportSettingsRequestBus::Events::GetModelPreset)
                ->Event("SaveModelPreset", &MaterialCanvasViewportSettingsRequestBus::Events::SaveModelPreset)
                ->Event("LoadModelPreset", &MaterialCanvasViewportSettingsRequestBus::Events::LoadModelPreset)
                ->Event("LoadModelPresetByAssetId", &MaterialCanvasViewportSettingsRequestBus::Events::LoadModelPresetByAssetId)
                ->Event("GetLastModelPresetPath", &MaterialCanvasViewportSettingsRequestBus::Events::GetLastModelPresetPath)
                ->Event("GetLastModelPresetAssetId", &MaterialCanvasViewportSettingsRequestBus::Events::GetLastModelPresetAssetId)
                ->Event("SetShadowCatcherEnabled", &MaterialCanvasViewportSettingsRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &MaterialCanvasViewportSettingsRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &MaterialCanvasViewportSettingsRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &MaterialCanvasViewportSettingsRequestBus::Events::GetGridEnabled)
                ->Event("SetAlternateSkyboxEnabled", &MaterialCanvasViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled)
                ->Event("GetAlternateSkyboxEnabled", &MaterialCanvasViewportSettingsRequestBus::Events::GetAlternateSkyboxEnabled)
                ->Event("SetFieldOfView", &MaterialCanvasViewportSettingsRequestBus::Events::SetFieldOfView)
                ->Event("GetFieldOfView", &MaterialCanvasViewportSettingsRequestBus::Events::GetFieldOfView)
                ;

            behaviorContext->EBus<MaterialCanvasViewportSettingsNotificationBus>("MaterialCanvasViewportSettingsNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Event("OnViewportSettingsChanged", &MaterialCanvasViewportSettingsNotificationBus::Events::OnViewportSettingsChanged)
                ;
        }
    }

    MaterialCanvasViewportSettingsSystem::MaterialCanvasViewportSettingsSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        MaterialCanvasViewportSettingsRequestBus::Handler::BusConnect(m_toolId);
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    MaterialCanvasViewportSettingsSystem::~MaterialCanvasViewportSettingsSystem()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialCanvasViewportSettingsRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void MaterialCanvasViewportSettingsSystem::ClearContent()
    {
        m_lightingPresetCache.clear();
        m_lightingPreset = {};

        m_modelPresetCache.clear();
        m_modelPreset = {};

        m_settingsNotificationPending = {};
    }

    void MaterialCanvasViewportSettingsSystem::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_settingsNotificationPending)
        {
            m_settingsNotificationPending = false;
            MaterialCanvasViewportSettingsNotificationBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsNotificationBus::Events::OnViewportSettingsChanged);
        }
    }

    void MaterialCanvasViewportSettingsSystem::SetLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::LightingPreset& MaterialCanvasViewportSettingsSystem::GetLightingPreset() const
    {
        return m_lightingPreset;
    }

    bool MaterialCanvasViewportSettingsSystem::SaveLightingPreset(const AZStd::string& path) const
    {
        if (!path.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_lightingPreset, path).IsSuccess())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialCanvas/ViewportSettings/LightingPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPresetCache[path] = m_lightingPreset;
            return true;
        }
        return false;
    }

    bool MaterialCanvasViewportSettingsSystem::LoadLightingPreset(const AZStd::string& path)
    {
        auto cacheItr = m_lightingPresetCache.find(path);
        if (cacheItr != m_lightingPresetCache.end())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialCanvas/ViewportSettings/LightingPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
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
                    "/O3DE/Atom/MaterialCanvas/ViewportSettings/LightingPresetAssetId",
                    AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
                m_lightingPresetCache[path] = m_lightingPreset = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
                m_settingsNotificationPending = true;
                return true;
            }
        }
        return false;
    }

    bool MaterialCanvasViewportSettingsSystem::LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadLightingPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string MaterialCanvasViewportSettingsSystem::GetLastLightingPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastLightingPresetAssetId());
    }

    AZ::Data::AssetId MaterialCanvasViewportSettingsSystem::GetLastLightingPresetAssetId() const
    {
        return AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/MaterialCanvas/ViewportSettings/LightingPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/lightingpresets/neutral_urban.lightingpreset.azasset"));
    }

    void MaterialCanvasViewportSettingsSystem::SetModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::ModelPreset& MaterialCanvasViewportSettingsSystem::GetModelPreset() const
    {
        return m_modelPreset;
    }

    bool MaterialCanvasViewportSettingsSystem::SaveModelPreset(const AZStd::string& path) const
    {
        if (!path.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_modelPreset, path).IsSuccess())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialCanvas/ViewportSettings/ModelPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPresetCache[path] = m_modelPreset;
            return true;
        }
        return false;
    }

    bool MaterialCanvasViewportSettingsSystem::LoadModelPreset(const AZStd::string& path)
    {
        auto cacheItr = m_modelPresetCache.find(path);
        if (cacheItr != m_modelPresetCache.end())
        {
            AtomToolsFramework::SetSettingsObject(
                "/O3DE/Atom/MaterialCanvas/ViewportSettings/ModelPresetAssetId", AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
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
                    "/O3DE/Atom/MaterialCanvas/ViewportSettings/ModelPresetAssetId",
                    AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
                m_modelPresetCache[path] = m_modelPreset = AZStd::any_cast<AZ::Render::ModelPreset>(loadResult.GetValue());
                m_settingsNotificationPending = true;
                return true;
            }
        }
        return false;
    }

    bool MaterialCanvasViewportSettingsSystem::LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadModelPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string MaterialCanvasViewportSettingsSystem::GetLastModelPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastModelPresetAssetId());
    }

    AZ::Data::AssetId MaterialCanvasViewportSettingsSystem::GetLastModelPresetAssetId() const
    {
        return AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/MaterialCanvas/ViewportSettings/ModelPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/viewportmodels/shaderball.modelpreset.azasset"));
    }

    void MaterialCanvasViewportSettingsSystem::SetShadowCatcherEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/ViewportSettings/EnableShadowCatcher", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialCanvasViewportSettingsSystem::GetShadowCatcherEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/ViewportSettings/EnableShadowCatcher", true);
    }

    void MaterialCanvasViewportSettingsSystem::SetGridEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/ViewportSettings/EnableGrid", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialCanvasViewportSettingsSystem::GetGridEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/ViewportSettings/EnableGrid", true);
    }

    void MaterialCanvasViewportSettingsSystem::SetAlternateSkyboxEnabled(bool enable)
    {
        AtomToolsFramework::SetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/ViewportSettings/EnableAlternateSkybox", enable);
        m_settingsNotificationPending = true;
    }

    bool MaterialCanvasViewportSettingsSystem::GetAlternateSkyboxEnabled() const
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/ViewportSettings/EnableAlternateSkybox", false);
    }

    void MaterialCanvasViewportSettingsSystem::SetFieldOfView(float fieldOfView)
    {
        AtomToolsFramework::SetSettingsValue<double>(
            "/O3DE/Atom/MaterialCanvas/ViewportSettings/FieldOfView", aznumeric_cast<double>(fieldOfView));
        m_settingsNotificationPending = true;
    }

    float MaterialCanvasViewportSettingsSystem::GetFieldOfView() const
    {
        return aznumeric_cast<float>(
            AtomToolsFramework::GetSettingsValue<double>("/O3DE/Atom/MaterialCanvas/ViewportSettings/FieldOfView", 90.0));
    }

    void MaterialCanvasViewportSettingsSystem::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        AtomToolsFramework::SetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialCanvas/ViewportSettings/DisplayMapperOperationType", aznumeric_cast<AZ::u64>(operationType));
        m_settingsNotificationPending = true;
    }

    AZ::Render::DisplayMapperOperationType MaterialCanvasViewportSettingsSystem::GetDisplayMapperOperationType() const
    {
        return aznumeric_cast<AZ::Render::DisplayMapperOperationType>(AtomToolsFramework::GetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialCanvas/ViewportSettings/DisplayMapperOperationType",
            aznumeric_cast<AZ::u64>(AZ::Render::DisplayMapperOperationType::Aces)));
    }

    void MaterialCanvasViewportSettingsSystem::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        ClearContent();
        LoadModelPreset(GetLastModelPresetPath());
        LoadLightingPreset(GetLastLightingPresetPath());
    }

    void MaterialCanvasViewportSettingsSystem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void MaterialCanvasViewportSettingsSystem::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void MaterialCanvasViewportSettingsSystem::QueueLoadPresetCache(const AZ::Data::AssetInfo& assetInfo)
    {
        if (assetInfo.m_assetType != AZ::RPI::AnyAsset::RTTI_Type())
        {
            return;
        }

        if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::LightingPreset::Extension))
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

        if (AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::ModelPreset::Extension))
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
} // namespace MaterialCanvas
