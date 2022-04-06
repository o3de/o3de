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
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettings.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
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

namespace AtomToolsFramework
{
    void EntityPreviewViewportSettingsSystem::Reflect(AZ::ReflectContext* context)
    {
        EntityPreviewViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EntityPreviewViewportSettingsSystem>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EntityPreviewViewportSettingsSystem>("EntityPreviewViewportSettingsSystem", "Manages and serializes settings for the application viewport")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EntityPreviewViewportSettingsRequestBus>("EntityPreviewViewportSettingsRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("SetLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::SetLightingPreset)
                ->Event("GetLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::GetLightingPreset)
                ->Event("SaveLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::SaveLightingPreset)
                ->Event("LoadLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset)
                ->Event("LoadLightingPresetByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId)
                ->Event("GetLastLightingPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPath)
                ->Event("GetLastLightingPresetAssetId", &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetAssetId)
                ->Event("SetModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset)
                ->Event("GetModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::GetModelPreset)
                ->Event("SaveModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset)
                ->Event("LoadModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset)
                ->Event("LoadModelPresetByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPresetByAssetId)
                ->Event("GetLastModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPath)
                ->Event("GetLastModelPresetAssetId", &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetAssetId)
                ->Event("SetShadowCatcherEnabled", &EntityPreviewViewportSettingsRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &EntityPreviewViewportSettingsRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &EntityPreviewViewportSettingsRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &EntityPreviewViewportSettingsRequestBus::Events::GetGridEnabled)
                ->Event("SetAlternateSkyboxEnabled", &EntityPreviewViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled)
                ->Event("GetAlternateSkyboxEnabled", &EntityPreviewViewportSettingsRequestBus::Events::GetAlternateSkyboxEnabled)
                ->Event("SetFieldOfView", &EntityPreviewViewportSettingsRequestBus::Events::SetFieldOfView)
                ->Event("GetFieldOfView", &EntityPreviewViewportSettingsRequestBus::Events::GetFieldOfView)
                ;

            behaviorContext->EBus<EntityPreviewViewportSettingsNotificationBus>("EntityPreviewViewportSettingsNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("OnViewportSettingsChanged", &EntityPreviewViewportSettingsNotificationBus::Events::OnViewportSettingsChanged)
                ;
        }
    }

    EntityPreviewViewportSettingsSystem::EntityPreviewViewportSettingsSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        EntityPreviewViewportSettingsRequestBus::Handler::BusConnect(m_toolId);
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    EntityPreviewViewportSettingsSystem::~EntityPreviewViewportSettingsSystem()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        EntityPreviewViewportSettingsRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void EntityPreviewViewportSettingsSystem::ClearContent()
    {
        m_lightingPresetCache.clear();
        m_lightingPreset = {};

        m_modelPresetCache.clear();
        m_modelPreset = {};

        m_settingsNotificationPending = {};
    }

    void EntityPreviewViewportSettingsSystem::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_settingsNotificationPending)
        {
            m_settingsNotificationPending = false;
            EntityPreviewViewportSettingsNotificationBus::Event(
                m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnViewportSettingsChanged);
        }
    }

    void EntityPreviewViewportSettingsSystem::SetLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::LightingPreset& EntityPreviewViewportSettingsSystem::GetLightingPreset() const
    {
        return m_lightingPreset;
    }

    bool EntityPreviewViewportSettingsSystem::SaveLightingPreset(const AZStd::string& path) const
    {
        if (!path.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_lightingPreset, path).IsSuccess())
        {
            SetSettingsObject(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetAssetId",
                AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPresetCache[path] = m_lightingPreset;
            return true;
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadLightingPreset(const AZStd::string& path)
    {
        auto cacheItr = m_lightingPresetCache.find(path);
        if (cacheItr != m_lightingPresetCache.end())
        {
            SetSettingsObject(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetAssetId",
                AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_lightingPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!path.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
            {
                SetSettingsObject(
                    "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetAssetId",
                    AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
                m_lightingPresetCache[path] = m_lightingPreset = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
                m_settingsNotificationPending = true;
                return true;
            }
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadLightingPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastLightingPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastLightingPresetAssetId());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastLightingPresetAssetId() const
    {
        return GetSettingsObject(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/lightingpresets/neutral_urban.lightingpreset.azasset"));
    }

    void EntityPreviewViewportSettingsSystem::SetModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPreset = preset;
        m_settingsNotificationPending = true;
    }

    const AZ::Render::ModelPreset& EntityPreviewViewportSettingsSystem::GetModelPreset() const
    {
        return m_modelPreset;
    }

    bool EntityPreviewViewportSettingsSystem::SaveModelPreset(const AZStd::string& path) const
    {
        if (!path.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_modelPreset, path).IsSuccess())
        {
            SetSettingsObject(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetAssetId",
                AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPresetCache[path] = m_modelPreset;
            return true;
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadModelPreset(const AZStd::string& path)
    {
        auto cacheItr = m_modelPresetCache.find(path);
        if (cacheItr != m_modelPresetCache.end())
        {
            SetSettingsObject(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetAssetId",
                AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
            m_modelPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!path.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::Render::ModelPreset>())
            {
                SetSettingsObject(
                    "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetAssetId",
                    AZ::RPI::AssetUtils::MakeAssetId(path, 0).TakeValue());
                m_modelPresetCache[path] = m_modelPreset = AZStd::any_cast<AZ::Render::ModelPreset>(loadResult.GetValue());
                m_settingsNotificationPending = true;
                return true;
            }
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadModelPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastModelPresetPath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetLastModelPresetAssetId());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastModelPresetAssetId() const
    {
        return GetSettingsObject(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetAssetId",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/viewportmodels/shaderball.modelpreset.azasset"));
    }

    void EntityPreviewViewportSettingsSystem::SetShadowCatcherEnabled(bool enable)
    {
        SetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableShadowCatcher", enable);
        m_settingsNotificationPending = true;
    }

    bool EntityPreviewViewportSettingsSystem::GetShadowCatcherEnabled() const
    {
        return GetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableShadowCatcher", true);
    }

    void EntityPreviewViewportSettingsSystem::SetGridEnabled(bool enable)
    {
        SetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableGrid", enable);
        m_settingsNotificationPending = true;
    }

    bool EntityPreviewViewportSettingsSystem::GetGridEnabled() const
    {
        return GetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableGrid", true);
    }

    void EntityPreviewViewportSettingsSystem::SetAlternateSkyboxEnabled(bool enable)
    {
        SetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableAlternateSkybox", enable);
        m_settingsNotificationPending = true;
    }

    bool EntityPreviewViewportSettingsSystem::GetAlternateSkyboxEnabled() const
    {
        return GetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableAlternateSkybox", false);
    }

    void EntityPreviewViewportSettingsSystem::SetFieldOfView(float fieldOfView)
    {
        SetSettingsValue<double>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/FieldOfView", aznumeric_cast<double>(fieldOfView));
        m_settingsNotificationPending = true;
    }

    float EntityPreviewViewportSettingsSystem::GetFieldOfView() const
    {
        return aznumeric_cast<float>(GetSettingsValue<double>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/FieldOfView", 90.0));
    }

    void EntityPreviewViewportSettingsSystem::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        SetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/DisplayMapperOperationType", aznumeric_cast<AZ::u64>(operationType));
        m_settingsNotificationPending = true;
    }

    AZ::Render::DisplayMapperOperationType EntityPreviewViewportSettingsSystem::GetDisplayMapperOperationType() const
    {
        return aznumeric_cast<AZ::Render::DisplayMapperOperationType>(GetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/DisplayMapperOperationType",
            aznumeric_cast<AZ::u64>(AZ::Render::DisplayMapperOperationType::Aces)));
    }

    void EntityPreviewViewportSettingsSystem::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        ClearContent();
        LoadModelPreset(GetLastModelPresetPath());
        LoadLightingPreset(GetLastLightingPresetPath());
    }

    void EntityPreviewViewportSettingsSystem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void EntityPreviewViewportSettingsSystem::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        QueueLoadPresetCache(assetInfo);
    }

    void EntityPreviewViewportSettingsSystem::QueueLoadPresetCache(const AZ::Data::AssetInfo& assetInfo)
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
} // namespace AtomToolsFramework
