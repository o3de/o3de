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

#include "LmbrCentral_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <CryCommon/IFlares.h>

#include <LmbrCentral/Rendering/EditorLightComponentBus.h>

#include "EditorLensFlareComponent.h"
#include "LightComponent.h"

namespace LmbrCentral
{
    void EditorLensFlareComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        EditorLensFlareConfiguration::Reflect(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorLensFlareComponent, EditorComponentBase>()->
                Version(1)->
                Field("Visible", &EditorLensFlareComponent::m_visible)->
                Field("LensFlareLibrary", &EditorLensFlareComponent::m_asset)->
                Field("SelectedLensFlare", &EditorLensFlareComponent::m_selectedLensFlareName)->
                Field("EditorLensFlareConfiguration", &EditorLensFlareComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorLensFlareComponent>(
                    "Lens Flare", "The Lens Flare component allows the placement of a lens flare on an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/LensFlare.svg")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<LmbrCentral::LensFlareAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/LensFlare.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-lens-flare.html")

                    ->DataElement(0, &EditorLensFlareComponent::m_configuration, "Settings", "Lens flare configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorLensFlareComponent::m_visible, "Visible", "The current visibility status of this lens flare")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLensFlareComponent::OnVisibleChanged)

                    ->DataElement(0, &EditorLensFlareComponent::m_asset, "Library", "The selected library of lens flares.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLensFlareComponent::OnAssetChanged)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorLensFlareComponent::m_selectedLensFlareName, "Lens flare", "The selected lens flare in this library.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLensFlareComponent::OnLensFlareSelected)
                        ->Attribute(AZ::Edit::Attributes::StringList, &EditorLensFlareComponent::GetLensFlarePaths)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorLensFlareConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorLensFlareConfiguration, LensFlareConfiguration>()->
                Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<LensFlareConfiguration>(
                    "Configuration", "Lens Flare configuration")->

                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                        Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))->

                    ClassElement(AZ::Edit::ClassElements::Group, "Flare Settings")->

                    DataElement(AZ::Edit::UIHandlers::ComboBox, &LensFlareConfiguration::m_minSpec, "Minimum spec", "Min spec for light to be active.")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        EnumAttribute(EngineSpec::Never, "Never")->
                        EnumAttribute(EngineSpec::VeryHigh, "Very high")->
                        EnumAttribute(EngineSpec::High, "High")->
                        EnumAttribute(EngineSpec::Medium, "Medium")->
                        EnumAttribute(EngineSpec::Low, "Low")->

                    DataElement(0, &LensFlareConfiguration::m_lensFlareFrustumAngle, "FOV", "The lens flare FOV angle")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Min, 0.f)->
                        Attribute(AZ::Edit::Attributes::Step, 1.f)->
                        Attribute(AZ::Edit::Attributes::Max, 360.f)->
                        Attribute(AZ::Edit::Attributes::Suffix, " degrees")->

                    DataElement(AZ::Edit::UIHandlers::Default, &LensFlareConfiguration::m_size, "Size", "The size of the lens flare")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Min, 0.f)->

                    DataElement(0, &LensFlareConfiguration::m_attachToSun, "Attach to sun", "Attach this flare to the sun")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::AttachToSunChanged)->

                    DataElement(0, &LensFlareConfiguration::m_useVisAreas, "Use VisAreas", "Lens Flares is affected by VisAreas")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->

                    DataElement(0, &LensFlareConfiguration::m_indoorOnly, "Indoor only", "Indoor only")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->

                    DataElement(0, &LensFlareConfiguration::m_onInitially, "On initially", "The lens flare is initially turned on.")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->

                    DataElement(0, &LensFlareConfiguration::m_viewDistMultiplier, "View distance multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        // Will be visible if attach to sun is false.
                        Attribute(AZ::Edit::Attributes::Visibility, &LensFlareConfiguration::ShouldViewDistanceMultiplier)->
                        Attribute(AZ::Edit::Attributes::Suffix, "x")->
                        Attribute(AZ::Edit::Attributes::Min, 0.f)->

                    ClassElement(AZ::Edit::ClassElements::Group, "Color Settings")->

                    DataElement(AZ::Edit::UIHandlers::Color, &LensFlareConfiguration::m_tint, "Tint", "Lens flare color tint")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->

                    DataElement(AZ::Edit::UIHandlers::Color, &LensFlareConfiguration::m_tintAlpha, "Tint [alpha]", "Lens flare alpha tint")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Min, 0)->
                        Attribute(AZ::Edit::Attributes::Max, 255)->
                        Attribute(AZ::Edit::Attributes::Step, 1)->

                    DataElement(0, &LensFlareConfiguration::m_brightness, "Brightness", "Lens flare brightness")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Min, 0.f)->
                        Attribute(AZ::Edit::Attributes::Step, 0.1f)->
                        Attribute(AZ::Edit::Attributes::Suffix, "x")->

                    ClassElement(AZ::Edit::ClassElements::Group, "Animation")->

                    DataElement(0, &LensFlareConfiguration::m_syncAnimWithLight, "Sync with light", "When checked uses the animation settings of a provided light")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::SyncAnimationChanged)->

                    DataElement(0, &LensFlareConfiguration::m_lightEntity, "Light", "Entity that has a light component to sync with")->
                        Attribute(AZ::Edit::Attributes::Visibility, &LensFlareConfiguration::m_syncAnimWithLight)->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->

                    DataElement(0, &LensFlareConfiguration::m_animIndex, "Style", "Light animation curve ID (\"style\") as it corresponds to values in Light.cfx")->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Max, 255)->
                        Attribute(AZ::Edit::Attributes::Visibility, &LensFlareConfiguration::ShouldShowAnimationSettings)->

                    DataElement(0, &LensFlareConfiguration::m_animSpeed, "Speed", "Multiple of the base animation rate")->
                        Attribute(AZ::Edit::Attributes::Visibility, &LensFlareConfiguration::ShouldShowAnimationSettings)->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Min, 0.f)->
                        Attribute(AZ::Edit::Attributes::Step, 0.1f)->
                        Attribute(AZ::Edit::Attributes::Max, 4.f)->
                        Attribute(AZ::Edit::Attributes::Suffix, "x")->

                    DataElement(0, &LensFlareConfiguration::m_animPhase, "Phase", "Animation start offset from 0 to 1.  0.1 would be 10% into the animation")->
                        Attribute(AZ::Edit::Attributes::Visibility, &LensFlareConfiguration::ShouldShowAnimationSettings)->
                        Attribute(AZ::Edit::Attributes::ChangeNotify, &LensFlareConfiguration::PropertyChanged)->
                        Attribute(AZ::Edit::Attributes::Min, 0.f)->
                        Attribute(AZ::Edit::Attributes::Step, 0.1f)->
                        Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ;
            }
        }
    }

    AZ::u32 EditorLensFlareConfiguration::PropertyChanged()
    {
        if (m_editorEntityId.IsValid())
        {
            if (m_syncAnimWithLight && m_lightEntity.IsValid())
            {
                //Get the config of the light we want to sync with
                LmbrCentral::LightConfiguration lightConfig;

                EBUS_EVENT_ID_RESULT(lightConfig, m_lightEntity, EditorLightComponentRequestBus, GetConfiguration);

                m_syncAnimIndex = lightConfig.m_animIndex;
                m_syncAnimSpeed = lightConfig.m_animSpeed;
                m_syncAnimPhase = lightConfig.m_animPhase;
            }

            EBUS_EVENT_ID(m_editorEntityId, EditorLensFlareComponentRequestBus, RefreshLensFlare);
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 EditorLensFlareConfiguration::SyncAnimationChanged()
    {
        if (m_syncAnimWithLight)
        {
            LightSettingsNotificationsBus::Handler::BusConnect(m_editorEntityId);
        }
        else
        {
            LightSettingsNotificationsBus::Handler::BusDisconnect();
        }

        PropertyChanged();

        return AZ_CRC("RefreshEntireTree", 0xefbc823c);
    }

    AZ::u32 EditorLensFlareConfiguration::AttachToSunChanged()
    {
        // if attached to the sun , then use VIEW_DISTANCE_MULTIPLIER_MAX value
        // else use the value set by user.
        if (m_attachToSun)
        {
            m_viewDistMultiplierUser = m_viewDistMultiplier;
            m_viewDistMultiplier = static_cast<float>(IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX);
        }
        else
        {
            // We restore the cache user set value when it is not attached to the sun.
            m_viewDistMultiplier = m_viewDistMultiplierUser;
        }

        PropertyChanged();

        return AZ_CRC("RefreshEntireTree", 0xefbc823c);
    }
    void EditorLensFlareConfiguration::AnimationSettingsChanged()
    {
        PropertyChanged();
    }

    void EditorLensFlareComponent::Init()
    {
        Base::Init();

        //Set this in Init because it will never change and doesn't need to be reset if the component re-activates
        m_configuration.m_material.SetAssetPath("EngineAssets/Materials/lens_optics");
    }

    void EditorLensFlareComponent::Activate()
    {
        Base::Activate();

        //Trigger asset loading so the the editor could show right content for the dropdown.
        //Don't call OnAssetChanged because we don't need to clear any serialized data
        //nor do we need to check for bus connection.
        if (m_asset.GetId().IsValid())
        {
            //load LensFlare asset
            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
            m_asset.QueueLoad();
        }

        m_selectedLensFlareLibrary = GetLibraryNameFromAsset();
        m_selectedLensFlareName = GetFlareNameFromPath(m_configuration.m_lensFlare);

        const AZ::EntityId entityId = GetEntityId();
        m_configuration.m_editorEntityId = entityId;

        m_light.SetEntity(entityId);
        RefreshLensFlare();

        //Check to see if we need to start connected to the LightSettingsNotificationBus
        m_configuration.SyncAnimationChanged();
        // We call this one in order to cache the user set value.
        m_configuration.m_viewDistMultiplierUser = m_configuration.m_viewDistMultiplier;

        m_configuration.AttachToSunChanged();

        EditorLensFlareComponentRequestBus::Handler::BusConnect(entityId);
        RenderNodeRequestBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void EditorLensFlareComponent::Deactivate()
    {
        //Check to see if we need to disconnect from the LightSettingsNotificationBus
        m_configuration.SyncAnimationChanged();

        AZ::Data::AssetBus::Handler::BusDisconnect();
        EditorLensFlareComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

        m_light.DestroyRenderLight();
        m_light.SetEntity(AZ::EntityId());

        m_configuration.m_editorEntityId.SetInvalid();

        m_selectedLensFlareLibrary.clear();

        Base::Deactivate();
    }

    void EditorLensFlareComponent::OnEntityVisibilityChanged(bool /*visibility*/)
    {
        RefreshLensFlare();
    }

    void EditorLensFlareComponent::OnEditorSpecChange()
    {
        RefreshLensFlare();
    }

    void EditorLensFlareComponent::RefreshLensFlare()
    {
        m_light.UpdateRenderLight(GetEditorLensFlareConfiguration());
    }

    EditorLensFlareConfiguration EditorLensFlareComponent::GetEditorLensFlareConfiguration() const
    {
        EditorLensFlareConfiguration configuration = m_configuration;

        // take the entity's visibility into account
        bool visible = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        configuration.m_visible = visible && configuration.m_visible;
        configuration.m_asset = m_asset;

        return configuration;
    }

    void EditorLensFlareComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        LensFlareComponent* lensFlareComponent = gameEntity->CreateComponent<LensFlareComponent>();

        if (lensFlareComponent)
        {
            lensFlareComponent->m_configuration = m_configuration;
            lensFlareComponent->m_configuration.m_asset = m_asset;
        }
    }

    void EditorLensFlareComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // Don't draw extra visualization unless selected.
        if (!IsSelected())
        {
            return;
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        debugDisplay.PushMatrix(transform);

        {
            const AZ::Vector3& color = m_configuration.m_tint;
            debugDisplay.SetColor(AZ::Vector4(color.GetX(), color.GetY(), color.GetZ(), 1.f));
            debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), 1.0f);
        }

        debugDisplay.PopMatrix();
    }

    AZ::u32 EditorLensFlareComponent::OnLensFlareSelected()
    {
        m_configuration.m_lensFlare = GetSelectedLensFlareFullName();

        //If the flare we've selected is valid we need to parse a couple of parameters from it to make sure
        //that we display the flare as it's seen in the lens flare editor
        if (!m_configuration.m_lensFlare.empty())
        {
            int lensOpticsID = 0;
            if (gEnv->pOpticsManager->Load(m_configuration.m_lensFlare.c_str(), lensOpticsID))
            {
                IOpticsElementBase* flare = gEnv->pOpticsManager->GetOptics(lensOpticsID);

                AZStd::vector<FuncVariableGroup> varGroups = flare->GetEditorParamGroups();

                for (size_t i = 0; i < varGroups.size(); ++i)
                {
                    if (strcmp(varGroups[i].GetName(), "Common") == 0)
                    {
                        IFuncVariable* var = varGroups[i].FindVariable("Size");
                        if (var)
                        {
                            m_configuration.m_size = var->GetFloat();
                        }

                        var = varGroups[i].FindVariable("Tint");
                        if (var)
                        {
                            ColorF color = var->GetColorF();
                            m_configuration.m_tint = AZ::Vector3(color.r, color.g, color.b);
                            m_configuration.m_tintAlpha = static_cast<AZ::u32>(color.a * 255.0f);
                        }

                        var = varGroups[i].FindVariable("Brightness");
                        if (var)
                        {
                            m_configuration.m_brightness = var->GetFloat();
                        }

                    }
                }
            }
        }

        m_configuration.PropertyChanged(); // Update the render light

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorLensFlareComponent::OnVisibleChanged()
    {
        m_configuration.m_visible = m_visible;

        m_configuration.PropertyChanged();
    }

    void EditorLensFlareComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_asset.GetId())
        {
            // Grab the lens flare list list from the Asset and refresh
            const AZStd::vector<AZStd::string>& paths = static_cast<LensFlareAsset*>(asset.Get())->GetPaths();

            if (!paths.empty())
            {
                //Store the name of the library retrieved from the filepath
                m_selectedLensFlareLibrary = GetLibraryNameFromAsset();

                // No selected lens flare, so automatically select the first one to ensure we see something right away.
                if (m_selectedLensFlareName.empty())
                {
                    m_selectedLensFlareName = GetFlareNameFromPath(paths[0]);
                    OnLensFlareSelected();
                }
            }

            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
    }

    AZStd::vector<AZStd::string> EditorLensFlareComponent::GetLensFlarePaths() const
    {
        if (m_asset.IsReady())
        {
            AZStd::vector<AZStd::string> paths = m_asset.Get()->GetPaths();

            //Remove the library name from the path as it's redundant to display it
            for (size_t i = 0; i < paths.size(); i++)
            {
                paths[i] = GetFlareNameFromPath(paths[i]);
            }

            return paths;
        }
        else
        {
            return AZStd::vector<AZStd::string>();
        }
    }

    IRenderNode* EditorLensFlareComponent::GetRenderNode()
    {
        return m_light.GetRenderNode();
    }

    float EditorLensFlareComponent::GetRenderNodeRequestBusOrder() const
    {
        return LensFlareComponent::s_renderNodeRequestBusOrder;
    }

    void EditorLensFlareComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_asset = asset;

        // Force the optics manager to reload the library. Otherwise, it will keep returning the old version of the lens flare
        // each time Load() is called.
        int unusedOutIndex = 0;
        const bool forceReload = true;
        gEnv->pOpticsManager->Load(GetSelectedLensFlareFullName().c_str(), unusedOutIndex, forceReload);

        RefreshLensFlare();
    }

    void EditorLensFlareComponent::OnAssetChanged()
    {
        m_selectedLensFlareLibrary.clear();
        m_selectedLensFlareName.clear(); //Clear this so that when the asset is ready we will always pull a new flare from the new library

        if (AZ::Data::AssetBus::Handler::BusIsConnected())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        if (m_asset.GetId().IsValid())
        {
            //load LensFlare asset
            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
            m_asset.QueueLoad();
            m_asset.BlockUntilLoadComplete();
        }

        EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorLensFlareComponent::SetPrimaryAsset(const AZ::Data::AssetId& id)
    {
        m_asset.Create(id, true);
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, AddDirtyEntity, GetEntityId());
        OnAssetChanged();
    }

    AZStd::string EditorLensFlareComponent::GetFlareNameFromPath(const AZStd::string& path) const
    {
        //Trim the library name from the beginning of the path to get the flare's name
        if (!m_selectedLensFlareLibrary.empty() && m_selectedLensFlareLibrary.size() < path.size())
        {
            return path.substr(m_selectedLensFlareLibrary.size() + 1);
        }

        return AZStd::string();
    }

    AZStd::string EditorLensFlareComponent::GetLibraryNameFromAsset() const
    {
        AZStd::string filePath;
        EBUS_EVENT_RESULT(filePath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_asset.GetId());
        if (!filePath.empty())
        {
            filePath = filePath.substr(filePath.find_last_of('/') + 1);
            return filePath.substr(0, filePath.find_last_of('.'));
        }

        return AZStd::string();
    }

    AZStd::string EditorLensFlareComponent::GetSelectedLensFlareFullName() const
    {
        return m_selectedLensFlareLibrary + "." + m_selectedLensFlareName;
    }

} // namespace LmbrCentral
