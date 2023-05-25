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
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobFunction.h>
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
                ->Event("GetLastLightingPresetPathWithoutAlias", &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPathWithoutAlias)
                ->Event("RegisterLightingPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::RegisterLightingPresetPath)
                ->Event("UnregisterLightingPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::UnregisterLightingPresetPath)
                ->Event("GetRegisteredLightingPresetPaths", &EntityPreviewViewportSettingsRequestBus::Events::GetRegisteredLightingPresetPaths)
                ->Event("SetModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset)
                ->Event("GetModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::GetModelPreset)
                ->Event("SaveModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset)
                ->Event("LoadModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset)
                ->Event("LoadModelPresetByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPresetByAssetId)
                ->Event("GetLastModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPath)
                ->Event("GetLastModelPresetPathWithoutAlias", &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPathWithoutAlias)
                ->Event("RegisterModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::RegisterModelPresetPath)
                ->Event("UnregisterModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::UnregisterModelPresetPath)
                ->Event("GetRegisteredModelPresetPaths", &EntityPreviewViewportSettingsRequestBus::Events::GetRegisteredModelPresetPaths)
                ->Event("LoadRenderPipeline", &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipeline)
                ->Event("LoadRenderPipelineByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipelineByAssetId)
                ->Event("GetLastRenderPipelinePath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastRenderPipelinePath)
                ->Event("GetLastRenderPipelinePathWithoutAlias", &EntityPreviewViewportSettingsRequestBus::Events::GetLastRenderPipelinePathWithoutAlias)
                ->Event("RegisterRenderPipelinePath", &EntityPreviewViewportSettingsRequestBus::Events::RegisterRenderPipelinePath)
                ->Event("UnregisterRenderPipelinePath", &EntityPreviewViewportSettingsRequestBus::Events::UnregisterRenderPipelinePath)
                ->Event("GetRegisteredRenderPipelinePaths", &EntityPreviewViewportSettingsRequestBus::Events::GetRegisteredRenderPipelinePaths)
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
        PreloadPresets();
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

    bool EntityPreviewViewportSettingsSystem::SaveLightingPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(path);
        if (!pathWithoutAlias.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_lightingPreset, pathWithoutAlias).IsSuccess())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", pathWithAlias);
            RegisterLightingPreset(pathWithAlias, m_lightingPreset);
            return true;
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadLightingPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(path);
        auto cacheItr = m_lightingPresetCache.find(pathWithAlias);
        if (cacheItr != m_lightingPresetCache.end())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", pathWithAlias);
            m_lightingPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!pathWithoutAlias.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
            {
                SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", pathWithAlias);
                m_lightingPreset = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
                RegisterLightingPreset(pathWithAlias, m_lightingPreset);
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
        return GetSettingsValue<AZStd::string>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath",
            "@gemroot:MaterialEditor@/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset");
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastLightingPresetPathWithoutAlias() const
    {
        return GetPathWithoutAlias(GetLastLightingPresetPath());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastLightingPresetAssetId() const
    {
        const auto& result = AZ::RPI::AssetUtils::MakeAssetId(GetLastLightingPresetPath(), 0);
        return result.IsSuccess() ? result.GetValue() : AZ::Data::AssetId();
    }

    void EntityPreviewViewportSettingsSystem::RegisterLightingPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(AZ::Render::LightingPreset::Extension))
        {
            auto paths = GetRegisteredLightingPresetPaths();
            paths.insert(GetPathWithAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths", paths);
        }
    }

    void EntityPreviewViewportSettingsSystem::UnregisterLightingPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(AZ::Render::LightingPreset::Extension))
        {
            auto paths = GetRegisteredLightingPresetPaths();
            paths.erase(GetPathWithAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths", paths);
        }
    }

    AZStd::set<AZStd::string> EntityPreviewViewportSettingsSystem::GetRegisteredLightingPresetPaths() const
    {
        return GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths",
            AZStd::set<AZStd::string>{
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/LightingPresets/lythwood_room.lightingpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset" });
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

    bool EntityPreviewViewportSettingsSystem::SaveModelPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(path);
        if (!pathWithoutAlias.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_modelPreset, pathWithoutAlias).IsSuccess())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", pathWithAlias);
            RegisterModelPreset(pathWithAlias, m_modelPreset);
            return true;
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadModelPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(path);
        auto cacheItr = m_modelPresetCache.find(pathWithAlias);
        if (cacheItr != m_modelPresetCache.end())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", pathWithAlias);
            m_modelPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!pathWithoutAlias.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<AZ::Render::ModelPreset>())
            {
                SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", pathWithAlias);
                m_modelPreset = AZStd::any_cast<AZ::Render::ModelPreset>(loadResult.GetValue());
                RegisterModelPreset(pathWithAlias, m_modelPreset);
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
        return GetSettingsValue<AZStd::string>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath",
            "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Shaderball.modelpreset.azasset");
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastModelPresetPathWithoutAlias() const
    {
        return GetPathWithoutAlias(GetLastModelPresetPath());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastModelPresetAssetId() const
    {
        const auto& result = AZ::RPI::AssetUtils::MakeAssetId(GetLastModelPresetPath(), 0);
        return result.IsSuccess() ? result.GetValue() : AZ::Data::AssetId();
    }

    void EntityPreviewViewportSettingsSystem::RegisterModelPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(AZ::Render::ModelPreset::Extension))
        {
            auto paths = GetRegisteredModelPresetPaths();
            paths.insert(GetPathWithAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths", paths);
        }
    }

    void EntityPreviewViewportSettingsSystem::UnregisterModelPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(AZ::Render::ModelPreset::Extension))
        {
            auto paths = GetRegisteredModelPresetPaths();
            paths.erase(GetPathWithAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths", paths);
        }
    }

    AZStd::set<AZStd::string> EntityPreviewViewportSettingsSystem::GetRegisteredModelPresetPaths() const
    {
        return GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths",
            AZStd::set<AZStd::string>{
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/BeveledCone.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/BeveledCube.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/BeveledCylinder.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Caduceus.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Cone.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Cube.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Cylinder.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Hermanubis.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Plane_1x1.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Plane_3x3.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/PlatonicSphere.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/PolarSphere.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/QuadSphere.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Shaderball.modelpreset.azasset",
                "@gemroot:MaterialEditor@/Assets/MaterialEditor/ViewportModels/Torus.modelpreset.azasset" });
    }

    bool EntityPreviewViewportSettingsSystem::LoadRenderPipeline(const AZStd::string& path)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(path);
        auto cacheItr = m_renderPipelineDescriptorCache.find(pathWithAlias);
        if (cacheItr != m_renderPipelineDescriptorCache.end())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath", pathWithAlias);
            m_renderPipelineDescriptor = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!pathWithoutAlias.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<AZ::RPI::RenderPipelineDescriptor>())
            {
                SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath", pathWithAlias);
                m_renderPipelineDescriptor = AZStd::any_cast<AZ::RPI::RenderPipelineDescriptor>(loadResult.GetValue());
                RegisterRenderPipeline(pathWithAlias, m_renderPipelineDescriptor);
                return true;
            }
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadRenderPipelineByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadRenderPipeline(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastRenderPipelinePath() const
    {
        return GetPathWithoutAlias(GetSettingsValue<AZStd::string>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath",
            "@gemroot:Atom_Feature_Common@/Assets/Passes/MainRenderPipeline.azasset"));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastRenderPipelinePathWithoutAlias() const
    {
        return GetPathWithoutAlias(GetLastRenderPipelinePath());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastRenderPipelineAssetId() const
    {
        const auto& result = AZ::RPI::AssetUtils::MakeAssetId(GetLastRenderPipelinePath(), 0);
        return result.IsSuccess() ? result.GetValue() : AZ::Data::AssetId();
    }

    void EntityPreviewViewportSettingsSystem::RegisterRenderPipelinePath(const AZStd::string& path)
    {
        if (path.ends_with(AZ::RPI::RenderPipelineDescriptor::Extension))
        {
            auto paths = GetRegisteredRenderPipelinePaths();
            paths.insert(GetPathWithAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", paths);
        }
    }

    void EntityPreviewViewportSettingsSystem::UnregisterRenderPipelinePath(const AZStd::string& path)
    {
        if (path.ends_with(AZ::RPI::RenderPipelineDescriptor::Extension))
        {
            auto paths = GetRegisteredRenderPipelinePaths();
            paths.erase(GetPathWithAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", paths);
        }
    }

    AZStd::set<AZStd::string> EntityPreviewViewportSettingsSystem::GetRegisteredRenderPipelinePaths() const
    {
        return GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths",
            AZStd::set<AZStd::string>{
                "@gemroot:Atom_Feature_Common@/Assets/Passes/MainRenderPipeline.azasset" });
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

    void EntityPreviewViewportSettingsSystem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        if (assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type())
        {
            PreloadPreset(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId));
        }
    }

    void EntityPreviewViewportSettingsSystem::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        OnCatalogAssetChanged(assetId);
    }

    void EntityPreviewViewportSettingsSystem::PreloadPreset(const AZStd::string& path)
    {
        if (path.ends_with(AZ::Render::LightingPreset::Extension))
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
            {
                RegisterLightingPreset(path, AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue()));
            }
            return;
        }

        if (path.ends_with(AZ::Render::ModelPreset::Extension))
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::Render::ModelPreset>())
            {
                RegisterModelPreset(path, AZStd::any_cast<AZ::Render::ModelPreset>(loadResult.GetValue()));
            }
            return;
        }

        if (path.ends_with(AZ::RPI::RenderPipelineDescriptor::Extension))
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(path);
            if (loadResult && loadResult.GetValue().is<AZ::RPI::RenderPipelineDescriptor>())
            {
                RegisterRenderPipeline(path, AZStd::any_cast<AZ::RPI::RenderPipelineDescriptor>(loadResult.GetValue()));
            }
            return;
        }
    }

    void EntityPreviewViewportSettingsSystem::RegisterLightingPreset(const AZStd::string& path, const AZ::Render::LightingPreset& preset)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        m_lightingPresetCache[pathWithAlias] = preset;
        m_settingsNotificationPending = true;
        RegisterLightingPresetPath(pathWithAlias);
        EntityPreviewViewportSettingsNotificationBus::Event(
            m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnLightingPresetAdded, pathWithAlias);
    }

    void EntityPreviewViewportSettingsSystem::RegisterModelPreset(const AZStd::string& path, const AZ::Render::ModelPreset& preset)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        m_modelPresetCache[pathWithAlias] = preset;
        m_settingsNotificationPending = true;
        RegisterModelPresetPath(pathWithAlias);
        EntityPreviewViewportSettingsNotificationBus::Event(
            m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnModelPresetAdded, pathWithAlias);
    }

    void EntityPreviewViewportSettingsSystem::RegisterRenderPipeline(const AZStd::string& path, const AZ::RPI::RenderPipelineDescriptor& preset)
    {
        const auto& pathWithAlias = GetPathWithAlias(path);
        m_renderPipelineDescriptorCache[pathWithAlias] = preset;
        m_settingsNotificationPending = true;
        RegisterRenderPipelinePath(pathWithAlias);
        EntityPreviewViewportSettingsNotificationBus::Event(
            m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnRenderPipelineAdded, pathWithAlias);
    }

    void EntityPreviewViewportSettingsSystem::PreloadPresets()
    {
        // Preload the last active lighting and model presets so they are available for the viewport and selection controls.
        LoadLightingPreset(GetLastLightingPresetPath());
        LoadModelPreset(GetLastModelPresetPath());

        // This job will do background enumeration of lighting and preset files available in the project. Once the files have been
        // enumerated, the work will be passed along to the tick bus on the main thread to register all of the presets that were discovered.
        // It should be safe to fire and forget because the bus for this class is being addressed by ID. If the class is destroyed and
        // disconnected then the notifications will be ignored.
        AZ::Job* job = AZ::CreateJobFunction(
            [toolId = m_toolId]()
            {
                AZ_TracePrintf("EntityPreviewViewportSettingsSystem", "Enumerating presets started.");
                auto filterFn = [](const AZStd::string& path)
                {
                    return
                        path.ends_with(AZ::Render::LightingPreset::Extension) ||
                        path.ends_with(AZ::Render::ModelPreset::Extension) ||
                        path.ends_with(AZ::RPI::RenderPipelineDescriptor::Extension);
                };

                const auto& paths = GetPathsInSourceFoldersMatchingFilter(filterFn);
                AZ_TracePrintf("EntityPreviewViewportSettingsSystem", "Enumerating presets finished.");

                AZ::TickBus::QueueFunction(
                    [toolId, paths]()
                    {
                        EntityPreviewViewportSettingsRequestBus::Event(
                            toolId,
                            [paths](EntityPreviewViewportSettingsRequests* viewportRequests)
                            {
                                for (const auto& path : paths)
                                {
                                    viewportRequests->PreloadPreset(path);
                                }
                            });
                    });
            },
            true);
        job->Start();
    }
} // namespace AtomToolsFramework
