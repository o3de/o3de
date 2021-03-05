/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include <Viewport/MaterialViewportComponent.h>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>

namespace MaterialEditor
{
    MaterialViewportComponent::MaterialViewportComponent()
    {
    }

    void MaterialViewportComponent::Reflect(AZ::ReflectContext* context)
    {
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
                ->Event("SaveLightingPresetSelection", &MaterialViewportRequestBus::Events::SaveLightingPresetSelection)
                ->Event("GetLightingPresets", &MaterialViewportRequestBus::Events::GetLightingPresets)
                ->Event("GetLightingPresetByName", &MaterialViewportRequestBus::Events::GetLightingPresetByName)
                ->Event("GetLightingPresetSelection", &MaterialViewportRequestBus::Events::GetLightingPresetSelection)
                ->Event("SelectLightingPreset", &MaterialViewportRequestBus::Events::SelectLightingPreset)
                ->Event("SelectLightingPresetByName", &MaterialViewportRequestBus::Events::SelectLightingPresetByName)
                ->Event("GetLightingPresetNames", &MaterialViewportRequestBus::Events::GetLightingPresetNames)
                ->Event("AddModelPreset", &MaterialViewportRequestBus::Events::AddModelPreset)
                ->Event("SaveModelPresetSelection", &MaterialViewportRequestBus::Events::SaveModelPresetSelection)
                ->Event("GetModelPresets", &MaterialViewportRequestBus::Events::GetModelPresets)
                ->Event("GetModelPresetByName", &MaterialViewportRequestBus::Events::GetModelPresetByName)
                ->Event("GetModelPresetSelection", &MaterialViewportRequestBus::Events::GetModelPresetSelection)
                ->Event("SelectModelPreset", &MaterialViewportRequestBus::Events::SelectModelPreset)
                ->Event("SelectModelPresetByName", &MaterialViewportRequestBus::Events::SelectModelPresetByName)
                ->Event("GetModelPresetNames", &MaterialViewportRequestBus::Events::GetModelPresetNames)
                ->Event("SetShadowCatcherEnabled", &MaterialViewportRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &MaterialViewportRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &MaterialViewportRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &MaterialViewportRequestBus::Events::GetGridEnabled)
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
                ;
        }
    }

    void MaterialViewportComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PerformanceMonitorService", 0x6a44241a));
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
        MaterialViewportRequestBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void MaterialViewportComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialViewportRequestBus::Handler::BusDisconnect();

        m_lightingPresetVector.clear();
        m_lightingPresetSelection.reset();

        m_modelPresetVector.clear();
        m_modelPresetSelection.reset();
    }

    void MaterialViewportComponent::ReloadContent()
    {
        AZ_TracePrintf("Material Editor", "Started loading viewport configurtions.\n");

        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnBeginReloadContent);

        const AZStd::string prevLightingPresetSelectionName = m_lightingPresetSelection ? m_lightingPresetSelection->m_displayName : "";
        const AZStd::string prevModelPresetSelectionName = m_modelPresetSelection ? m_modelPresetSelection->m_displayName : "";

        m_lightingPresetVector.clear();
        m_lightingPresetSelection.reset();

        m_modelPresetVector.clear();
        m_modelPresetSelection.reset();

        AZStd::vector<AZ::Data::AssetInfo> lightingAssetInfoVector;
        AZStd::vector<AZ::Data::AssetInfo> modelAssetInfoVector;

        // Enumerate and load all the relevant preset files in the project.
        // (The files are stored in a temporary list instead of processed in the callback because deep operations inside
        // AssetCatalogRequestBus::EnumerateAssets can lead to deadlocked)
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB = [&lightingAssetInfoVector, &modelAssetInfoVector]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), ".lightingpreset.azasset"))
            {
                lightingAssetInfoVector.push_back(info);
            }
            else if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), ".modelpreset.azasset"))
            {
                modelAssetInfoVector.push_back(info);
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        for (const auto& info : lightingAssetInfoVector)
        {
            if (info.m_assetId.IsValid())
            {
                AZ::Data::Asset<AZ::RPI::AnyAsset> asset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::AnyAsset>(
                    info.m_assetId, AZ::RPI::AssetUtils::TraceLevel::Warning);
                if (asset)
                {
                    const AZ::Render::LightingPreset* preset = asset->GetDataAs<AZ::Render::LightingPreset>();
                    if (preset)
                    {
                        AddLightingPreset(*preset);
                        AZ_TracePrintf("Material Editor", "Loaded viewport configurtion: %s.\n", info.m_relativePath.c_str());
                    }
                }
            }
        }

        for (const auto& info : modelAssetInfoVector)
        {
            if (info.m_assetId.IsValid())
            {
                AZ::Data::Asset<AZ::RPI::AnyAsset> asset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::AnyAsset>(
                    info.m_assetId, AZ::RPI::AssetUtils::TraceLevel::Warning);
                if (asset)
                {
                    const AZ::Render::ModelPreset* preset = asset->GetDataAs<AZ::Render::ModelPreset>();
                    if (preset)
                    {
                        AddModelPreset(*preset);
                        AZ_TracePrintf("Material Editor", "Loaded viewport configurtion: %s.\n", info.m_relativePath.c_str());
                    }
                }
            }
        }

        // If there was a prior selection, this will keep the same configuration selected.
        // Otherwise, these strings are empty and the operation will be ignored.
        SelectLightingPresetByName(prevLightingPresetSelectionName);
        SelectModelPresetByName(prevModelPresetSelectionName);

        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnEndReloadContent);

        AZ_TracePrintf("Material Editor", "Finished loading viewport configurtions.\n");
    }

    AZ::Render::LightingPresetPtr MaterialViewportComponent::AddLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPresetVector.push_back(AZStd::make_shared<AZ::Render::LightingPreset>(preset));
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnLightingPresetAdded,
            m_lightingPresetVector.back());

        if (preset.m_autoSelect || m_lightingPresetVector.size() == 1)
        {
            SelectLightingPreset(m_lightingPresetVector.back());
        }

        return m_lightingPresetVector.back();
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

    bool MaterialViewportComponent::SaveLightingPresetSelection(const AZStd::string& path) const
    {
        if (m_lightingPresetSelection)
        {
            return AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::LightingPreset>(
                m_lightingPresetSelection.get(), path).IsSuccess();
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

    AZ::Render::ModelPresetPtr MaterialViewportComponent::AddModelPreset(const AZ::Render::ModelPreset& preset)
    {
        m_modelPresetVector.push_back(AZStd::make_shared<AZ::Render::ModelPreset>(preset));
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnModelPresetAdded,
            m_modelPresetVector.back());

        if (preset.m_autoSelect || m_modelPresetVector.size() == 1)
        {
            SelectModelPreset(m_modelPresetVector.back());
        }

        return m_modelPresetVector.back();
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

    bool MaterialViewportComponent::SaveModelPresetSelection(const AZStd::string& path) const
    {
        if (m_modelPresetSelection)
        {
            return AZ::JsonSerializationUtils::SaveObjectToFile<AZ::Render::ModelPreset>(
                m_modelPresetSelection.get(), path).IsSuccess();
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

    void MaterialViewportComponent::SetShadowCatcherEnabled(bool enable)
    {
        m_shadowCatcherEnabled = enable;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnShadowCatcherEnabledChanged, enable);
    }

    bool MaterialViewportComponent::GetShadowCatcherEnabled() const
    {
        return m_shadowCatcherEnabled;
    }

    void MaterialViewportComponent::SetGridEnabled(bool enable)
    {
        m_gridEnabled = enable;
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnGridEnabledChanged, enable);
    }


    bool MaterialViewportComponent::GetGridEnabled() const
    {
        return m_gridEnabled;
    }

    void MaterialViewportComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        AZ::TickBus::QueueFunction([this]() { ReloadContent(); });
    }
}
