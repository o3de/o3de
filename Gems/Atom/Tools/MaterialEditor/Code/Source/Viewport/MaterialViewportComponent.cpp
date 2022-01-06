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
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#include <Atom/Viewport/MaterialViewportSettings.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Viewport/MaterialViewportComponent.h>

namespace MaterialEditor
{
    MaterialViewportComponent::MaterialViewportComponent()
    {
    }

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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
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
                ->Event("ReloadContent", &MaterialViewportRequestBus::Events::ReloadContent)
                ->Event("AddLightingPreset", &MaterialViewportRequestBus::Events::AddLightingPreset)
                ->Event("SaveLightingPreset", &MaterialViewportRequestBus::Events::SaveLightingPreset)
                ->Event("GetLightingPresets", &MaterialViewportRequestBus::Events::GetLightingPresets)
                ->Event("GetLightingPresetByName", &MaterialViewportRequestBus::Events::GetLightingPresetByName)
                ->Event("GetLightingPresetSelection", &MaterialViewportRequestBus::Events::GetLightingPresetSelection)
                ->Event("SelectLightingPreset", &MaterialViewportRequestBus::Events::SelectLightingPreset)
                ->Event("SelectLightingPresetByName", &MaterialViewportRequestBus::Events::SelectLightingPresetByName)
                ->Event("GetLightingPresetNames", &MaterialViewportRequestBus::Events::GetLightingPresetNames)
                ->Event("GetLightingPresetLastSavePath", &MaterialViewportRequestBus::Events::GetLightingPresetLastSavePath)
                ->Event("AddModelPreset", &MaterialViewportRequestBus::Events::AddModelPreset)
                ->Event("SaveModelPreset", &MaterialViewportRequestBus::Events::SaveModelPreset)
                ->Event("GetModelPresets", &MaterialViewportRequestBus::Events::GetModelPresets)
                ->Event("GetModelPresetByName", &MaterialViewportRequestBus::Events::GetModelPresetByName)
                ->Event("GetModelPresetSelection", &MaterialViewportRequestBus::Events::GetModelPresetSelection)
                ->Event("SelectModelPreset", &MaterialViewportRequestBus::Events::SelectModelPreset)
                ->Event("SelectModelPresetByName", &MaterialViewportRequestBus::Events::SelectModelPresetByName)
                ->Event("GetModelPresetNames", &MaterialViewportRequestBus::Events::GetModelPresetNames)
                ->Event("GetModelPresetLastSavePath", &MaterialViewportRequestBus::Events::GetModelPresetLastSavePath)
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
                ->Event("OnShadowCatcherEnabledChanged", &MaterialViewportNotificationBus::Events::OnShadowCatcherEnabledChanged)
                ->Event("OnGridEnabledChanged", &MaterialViewportNotificationBus::Events::OnGridEnabledChanged)
                ->Event("OnAlternateSkyboxEnabledChanged", &MaterialViewportNotificationBus::Events::OnAlternateSkyboxEnabledChanged)
                ->Event("OnFieldOfViewChanged", &MaterialViewportNotificationBus::Events::OnFieldOfViewChanged)
                ;
        }
    }

    void MaterialViewportComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PerformanceMonitorService", 0x6a44241a));
        required.push_back(AZ_CRC("AtomImageBuilderService", 0x76ded592));
    }

    void MaterialViewportComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MaterialViewportService", 0xed9b44d7));
    }

    void MaterialViewportComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MaterialViewportService", 0xed9b44d7));
    }

    void MaterialViewportComponent::Init()
    {
    }

    void MaterialViewportComponent::Activate()
    {
        m_viewportSettings =
            AZ::UserSettings::CreateFind<MaterialViewportSettings>(AZ::Crc32("MaterialViewportSettings"), AZ::UserSettings::CT_GLOBAL);

        MaterialViewportRequestBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void MaterialViewportComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialViewportRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void MaterialViewportComponent::ClearContent()
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

    void MaterialViewportComponent::ReloadContent()
    {
        AZ_TracePrintf("Material Editor", "Started loading viewport configurations.\n");

        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnBeginReloadContent);

        ClearContent();

        // Enumerate and load all the relevant preset files in the project.
        // (The files are stored in a temporary list instead of processed in the callback because deep operations inside
        // AssetCatalogRequestBus::EnumerateAssets can lead to deadlocked)
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB = [this]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), ".lightingpreset.azasset"))
            {
                m_lightingPresetAssets[info.m_assetId] = { info.m_assetId, info.m_assetType };
                AZ::Data::AssetBus::MultiHandler::BusConnect(info.m_assetId);
            }
            else if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), ".modelpreset.azasset"))
            {
                m_modelPresetAssets[info.m_assetId] = { info.m_assetId, info.m_assetType };
                AZ::Data::AssetBus::MultiHandler::BusConnect(info.m_assetId);
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

    AZ::Render::LightingPresetPtr MaterialViewportComponent::AddLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPresetVector.push_back(AZStd::make_shared<AZ::Render::LightingPreset>(preset));
        auto presetPtr = m_lightingPresetVector.back();

        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnLightingPresetAdded, presetPtr);
        return presetPtr;
    }

    AZ::Render::LightingPresetPtr MaterialViewportComponent::GetLightingPresetByName(const AZStd::string& name) const
    {
        const auto presetItr = AZStd::find_if(m_lightingPresetVector.begin(), m_lightingPresetVector.end(), [&name](const auto& preset) {
            return preset && preset->m_displayName == name; });
        return presetItr != m_lightingPresetVector.end() ? *presetItr : nullptr;
    }

    AZ::Render::LightingPresetPtrVector MaterialViewportComponent::GetLightingPresets() const
    {
        return m_lightingPresetVector;
    }

    bool MaterialViewportComponent::SaveLightingPreset(AZ::Render::LightingPresetPtr preset, const AZStd::string& path) const
    {
        if (preset && AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::LightingPreset>(preset.get(), path).IsSuccess())
        {
            m_lightingPresetLastSavePathMap[preset] = path;
            return true;
        }

        return false;
    }

    AZ::Render::LightingPresetPtr MaterialViewportComponent::GetLightingPresetSelection() const
    {
        return m_lightingPresetSelection;
    }

    void MaterialViewportComponent::SelectLightingPreset(AZ::Render::LightingPresetPtr preset)
    {
        if (preset)
        {
            m_lightingPresetSelection = preset;
            m_viewportSettings->m_selectedLightingPresetName = preset->m_displayName;
            MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnLightingPresetSelected, m_lightingPresetSelection);
        }
    }

    void MaterialViewportComponent::SelectLightingPresetByName(const AZStd::string& name)
    {
        SelectLightingPreset(GetLightingPresetByName(name));
    }

    MaterialViewportPresetNameSet MaterialViewportComponent::GetLightingPresetNames() const
    {
        MaterialViewportPresetNameSet names;
        for (const auto& preset : m_lightingPresetVector)
        {
            if (preset)
            {
                names.insert(preset->m_displayName);
            }
        }
        return names;
    }

    AZStd::string MaterialViewportComponent::GetLightingPresetLastSavePath(AZ::Render::LightingPresetPtr preset) const
    {
        auto pathItr = m_lightingPresetLastSavePathMap.find(preset);
        return pathItr != m_lightingPresetLastSavePathMap.end() ? pathItr->second : AZStd::string();
    }

    AZ::Render::ModelPresetPtr MaterialViewportComponent::AddModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPresetVector.push_back(AZStd::make_shared<AZ::Render::ModelPreset>(preset));
        auto presetPtr = m_modelPresetVector.back();

        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnModelPresetAdded, presetPtr);
        return presetPtr;
    }

    AZ::Render::ModelPresetPtr MaterialViewportComponent::GetModelPresetByName(const AZStd::string& name) const
    {
        const auto presetItr = AZStd::find_if(m_modelPresetVector.begin(), m_modelPresetVector.end(), [&name](const auto& preset) {
            return preset && preset->m_displayName == name; });
        return presetItr != m_modelPresetVector.end() ? *presetItr : nullptr;
    }

    AZ::Render::ModelPresetPtrVector MaterialViewportComponent::GetModelPresets() const
    {
        return m_modelPresetVector;
    }

    bool MaterialViewportComponent::SaveModelPreset(AZ::Render::ModelPresetPtr preset, const AZStd::string& path) const
    {
        if (preset && AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::ModelPreset>(preset.get(), path).IsSuccess())
        {
            m_modelPresetLastSavePathMap[preset] = path;
            return true;
        }

        return false;
    }

    AZ::Render::ModelPresetPtr MaterialViewportComponent::GetModelPresetSelection() const
    {
        return m_modelPresetSelection;
    }

    void MaterialViewportComponent::SelectModelPreset(AZ::Render::ModelPresetPtr preset)
    {
        if (preset)
        {
            m_modelPresetSelection = preset;
            m_viewportSettings->m_selectedModelPresetName = preset->m_displayName;
            MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnModelPresetSelected, m_modelPresetSelection);
        }
    }

    void MaterialViewportComponent::SelectModelPresetByName(const AZStd::string& name)
    {
        SelectModelPreset(GetModelPresetByName(name));
    }

    MaterialViewportPresetNameSet MaterialViewportComponent::GetModelPresetNames() const
    {
        MaterialViewportPresetNameSet names;
        for (const auto& preset : m_modelPresetVector)
        {
            if (preset)
            {
                names.insert(preset->m_displayName);
            }
        }
        return names;
    }

    AZStd::string MaterialViewportComponent::GetModelPresetLastSavePath(AZ::Render::ModelPresetPtr preset) const
    {
        auto pathItr = m_modelPresetLastSavePathMap.find(preset);
        return pathItr != m_modelPresetLastSavePathMap.end() ? pathItr->second : AZStd::string();
    }

    void MaterialViewportComponent::SetShadowCatcherEnabled(bool enable)
    {
        m_viewportSettings->m_enableShadowCatcher = enable;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnShadowCatcherEnabledChanged, enable);
    }

    bool MaterialViewportComponent::GetShadowCatcherEnabled() const
    {
        return m_viewportSettings->m_enableShadowCatcher;
    }

    void MaterialViewportComponent::SetGridEnabled(bool enable)
    {
        m_viewportSettings->m_enableGrid = enable;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnGridEnabledChanged, enable);
    }


    bool MaterialViewportComponent::GetGridEnabled() const
    {
        return m_viewportSettings->m_enableGrid;
    }

    void MaterialViewportComponent::SetAlternateSkyboxEnabled(bool enable)
    {
        m_viewportSettings->m_enableAlternateSkybox = enable;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnAlternateSkyboxEnabledChanged, enable);
    }


    bool MaterialViewportComponent::GetAlternateSkyboxEnabled() const
    {
        return m_viewportSettings->m_enableAlternateSkybox;
    }

    void MaterialViewportComponent::SetFieldOfView(float fieldOfView)
    {
        m_viewportSettings->m_fieldOfView = fieldOfView;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnFieldOfViewChanged, fieldOfView);
    }


    float MaterialViewportComponent::GetFieldOfView() const
    {
        return m_viewportSettings->m_fieldOfView;
    }

    void MaterialViewportComponent::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        m_viewportSettings->m_displayMapperOperationType = operationType;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnDisplayMapperOperationTypeChanged, operationType);
    }

    AZ::Render::DisplayMapperOperationType MaterialViewportComponent::GetDisplayMapperOperationType() const
    {
        return m_viewportSettings->m_displayMapperOperationType;
    }

    inline void MaterialViewportComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (AZ::Data::Asset<AZ::RPI::AnyAsset> anyAsset = asset)
        {
            if (const auto lightingPreset = anyAsset->GetDataAs<AZ::Render::LightingPreset>())
            {
                auto presetPtr = AddLightingPreset(*lightingPreset);
                const auto& presetPath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(anyAsset.GetId());
                m_lightingPresetAssets[anyAsset.GetId()] = anyAsset;
                m_lightingPresetLastSavePathMap[presetPtr] = presetPath;
                AZ_TracePrintf("Material Editor", "Loaded Preset: %s\n", presetPath.c_str());
            }

            if (const auto modelPreset = anyAsset->GetDataAs<AZ::Render::ModelPreset>())
            {
                auto presetPtr = AddModelPreset(*modelPreset);
                const auto& presetPath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(anyAsset.GetId());
                m_modelPresetAssets[anyAsset.GetId()] = anyAsset;
                m_modelPresetLastSavePathMap[presetPtr] = presetPath;
                AZ_TracePrintf("Material Editor", "Loaded Preset: %s\n", presetPath.c_str());
            }
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
        if (!AZ::Data::AssetBus::MultiHandler::BusIsConnected())
        {
            SelectLightingPresetByName(m_viewportSettings->m_selectedLightingPresetName);
            SelectModelPresetByName(m_viewportSettings->m_selectedModelPresetName);
            MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnEndReloadContent);
            AZ_TracePrintf("Material Editor", "Finished loading viewport configurations.\n");
        }
    }

    void MaterialViewportComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        AZ::TickBus::QueueFunction([this]() {
            ReloadContent();
        });
    }
}
