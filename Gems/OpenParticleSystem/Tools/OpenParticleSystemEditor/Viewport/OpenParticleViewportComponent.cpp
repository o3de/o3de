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
#include <AtomToolsFramework/Util/Util.h>

#include <OpenParticleViewportComponent.h>
#include <OpenParticleViewportNotificationBus.h>
#include <OpenParticleViewportSettings.h>

namespace OpenParticleSystemEditor
{
    OpenParticleViewportComponent::OpenParticleViewportComponent()
    {
    }

    void OpenParticleViewportComponent::Reflect(AZ::ReflectContext* context)
    {
        OpenParticleViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OpenParticleViewportComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext
                    ->Class<OpenParticleViewportComponent>(
                        "OpenParticleViewport", "Manages configurations for lighting and models displayed in the viewport")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void OpenParticleViewportComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void OpenParticleViewportComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OpenParticleViewportService"));
    }

    void OpenParticleViewportComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OpenParticleViewportService"));
    }

    void OpenParticleViewportComponent::Init()
    {
    }

    void OpenParticleViewportComponent::Activate()
    {
        m_viewportSettings =
            AZ::UserSettings::CreateFind<OpenParticleViewportSettings>(AZ::Crc32("OpenParticleViewportSettings"), AZ::UserSettings::CT_GLOBAL);

        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        OpenParticleViewportRequestBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void OpenParticleViewportComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        OpenParticleViewportRequestBus::Handler::BusDisconnect();
    }

    void OpenParticleViewportComponent::ReloadContent()
    {
        AZ_TracePrintf("OpenParticle Editor", "Started loading viewport configurations.\n");

        OpenParticleViewportNotificationBus::Broadcast(&OpenParticleViewportNotificationBus::Events::OnBeginReloadContent);

        const AZStd::string selectedLightingPresetNameOld = m_viewportSettings->m_selectedLightingPresetName;

        m_lightingPresetVector.clear();
        m_lightingPresetNameSet.clear();
        m_lightingPresetDisplayNameMap.clear();
        m_lightingPresetSelection.reset();

        m_modelPresetVector.clear();
        m_modelPresetSelection.reset();

        AZStd::vector<AZ::Data::AssetInfo> lightingAssetInfoVector;

        // Enumerate and load all the relevant preset files in the project.
        // (The files are stored in a temporary list instead of processed in the callback because deep operations inside
        // AssetCatalogRequestBus::EnumerateAssets can lead to deadlocked)
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB = [&lightingAssetInfoVector]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), ".lightingpreset.azasset"))
            {
                lightingAssetInfoVector.push_back(info);
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
                        auto presetPtr = AddLightingPreset(*preset);
                        auto sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(info.m_assetId);
                        const AZStd::string displayName = AtomToolsFramework::GetDisplayNameFromPath(sourcePath);
                        m_lightingPresetDisplayNameMap[displayName] = presetPtr;
                        m_lightingPresetNameSet.insert(displayName);
                        AZ_TracePrintf("OpenParticle Editor", "Loaded viewport configuration: %s.\n", info.m_relativePath.c_str());
                    }
                }
            }
        }

        // If there was a prior selection, this will keep the same configuration selected.
        // Otherwise, these strings are empty and the operation will be ignored.
        SelectLightingPresetByName(selectedLightingPresetNameOld);

        OpenParticleViewportNotificationBus::Broadcast(&OpenParticleViewportNotificationBus::Events::OnEndReloadContent);

        AZ_TracePrintf("OpenParticle Editor", "Finished loading viewport configurations.\n");
    }

    AZ::Render::LightingPresetPtr OpenParticleViewportComponent::AddLightingPreset(const AZ::Render::LightingPreset& preset)
    {
        m_lightingPresetVector.push_back(AZStd::make_shared<AZ::Render::LightingPreset>(preset));
        auto presetPtr = m_lightingPresetVector.back();

        OpenParticleViewportNotificationBus::Broadcast(&OpenParticleViewportNotificationBus::Events::OnLightingPresetAdded, presetPtr);

        if (m_lightingPresetVector.size() == 1)
        {
            SelectLightingPreset(presetPtr);
        }

        return presetPtr;
    }

    AZ::Render::LightingPresetPtr OpenParticleViewportComponent::GetLightingPresetByName(const AZStd::string& name) const
    {
        return m_lightingPresetDisplayNameMap[name];
    }

    OpenParticleViewportPresetNameSet OpenParticleViewportComponent::GetLightingPresets() const
    {
        return m_lightingPresetNameSet;
    }

    AZStd::string OpenParticleViewportComponent::GetLightingPresetSelection() const
    {
        return m_lightingPresetNameSelection;
    }

    void OpenParticleViewportComponent::SelectLightingPreset(AZ::Render::LightingPresetPtr preset)
    {
        if (preset)
        {
            auto it = AZStd::find(m_lightingPresetVector.begin(), m_lightingPresetVector.end(), preset);
            auto index = it - m_lightingPresetVector.begin();
            AZStd::string name;
            int i = 0;
            for (auto iter : m_lightingPresetNameSet)
            {
                if (i == index)
                {
                    name = iter;
                    break;
                }
                i++;
            }
            SelectLightingPresetByName(name);
        }
    }

    void OpenParticleViewportComponent::SelectLightingPresetByName(const AZStd::string& name)
    {
        m_lightingPresetNameSelection = name;
        OpenParticleViewportNotificationBus::Broadcast(
            &OpenParticleViewportNotificationBus::Events::OnLightingPresetSelected, m_lightingPresetNameSelection);
    }

    void OpenParticleViewportComponent::SetGridEnabled(bool enable)
    {
        m_viewportSettings->m_enableGrid = enable;
        OpenParticleViewportNotificationBus::Broadcast(&OpenParticleViewportNotificationBus::Events::OnGridEnabledChanged, enable);
    }

    void OpenParticleViewportComponent::SetAlternateSkyboxEnabled(bool enable)
    {
        m_viewportSettings->m_enableAlternateSkybox = enable;
        OpenParticleViewportNotificationBus::Broadcast(&OpenParticleViewportNotificationBus::Events::OnAlternateSkyboxEnabledChanged, enable);
    }


    bool OpenParticleViewportComponent::GetAlternateSkyboxEnabled() const
    {
        return m_viewportSettings->m_enableAlternateSkybox;
    }

    void OpenParticleViewportComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        AZ::TickBus::QueueFunction([this]() {
            ReloadContent();
        });
    }

    void OpenParticleViewportComponent::OnApplicationAboutToStop()
    {
        m_lightingPresetVector.clear();
        m_lightingPresetNameSet.clear();
        m_lightingPresetDisplayNameMap.clear();
        m_lightingPresetSelection.reset();

        m_modelPresetVector.clear();
        m_modelPresetSelection.reset();
    }
}
