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

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Rendering/LensFlareAsset.h>
#include <LmbrCentral/Rendering/LightComponentBus.h>

#include "LensFlareComponent.h"

namespace LmbrCentral
{
    /**
     * Extends LightConfiguration structure to add editor functionality
     * such as property handlers and visibility filters, as well as
     * reflection for editing.
     */
    class EditorLensFlareConfiguration
        : public LensFlareConfiguration
        , private LightSettingsNotificationsBus::Handler
    {
    public:
        AZ_TYPE_INFO(EditorLightConfiguration, "{B7E8C0BF-A7B6-4414-90FF-6E21B32E5E16}");

        ~EditorLensFlareConfiguration() override {}

        static void Reflect(AZ::ReflectContext* context);

        // Overrides from LensFlareConfiguration
        AZ::u32 PropertyChanged() override;
        AZ::u32 SyncAnimationChanged() override;
        AZ::u32 AttachToSunChanged() override;
    
        // Overrides from LightSettingsNotificationBus
        void AnimationSettingsChanged() override;

        AZ::EntityId m_editorEntityId; // Not reflected.
    };

    /*!
     * In-editor light component.
     * Handles previewing and activating lights in the editor.
     */
    class EditorLensFlareComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private EditorLensFlareComponentRequestBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;
    public:
        AZ_COMPONENT(EditorLensFlareComponent, "{4B85E77D-91F9-40C5-8FCB-B494000A9E69}", AzToolsFramework::Components::EditorComponentBase);

        EditorLensFlareComponent() {}
        ~EditorLensFlareComponent() override {}

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorVisibilityNotificationBus interface implementation
        void OnEntityVisibilityChanged(bool visibility) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // LensFlareComponentEditorRequestBus::Handler interface implementation
        virtual void RefreshLensFlare() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus::Handler interface implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetEventHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        //! Invoked in the editor when the user assigns a new lens flare.
        void OnAssetChanged();

        //! Invoked in the editor when the user selects a lens flare from the combo box.
        AZ::u32 OnLensFlareSelected();

        //! Invoked in the editor when the user changes the visibility setting from the check box
        void OnVisibleChanged();

        //! Used to populate the lens flare combo box.
        AZStd::vector<AZStd::string> GetLensFlarePaths() const;

        //! Get a copy of configuration appropriate for use with the lens flare
        EditorLensFlareConfiguration GetEditorLensFlareConfiguration() const;

        //////////////////////////////////////////////////////////////////////////
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LensFlareComponent::GetProvidedServices(provided);
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            LensFlareComponent::GetDependentServices(dependent);
            dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LensFlareComponent::GetRequiredServices(required);
        }

        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

    private:
        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEvents::Bus::Handler interface implementation
        void OnEditorSpecChange() override;
        //////////////////////////////////////////////////////////////////////////

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZStd::string GetSelectedLensFlareFullName() const;
        AZStd::string GetFlareNameFromPath(const AZStd::string& path) const;
        AZStd::string GetLibraryNameFromAsset() const;

        EditorLensFlareConfiguration m_configuration;
        LightInstance m_light;
        AZStd::string m_selectedLensFlareName;
        AZStd::string m_selectedLensFlareLibrary;
        AZ::Data::Asset<LensFlareAsset> m_asset;
        bool m_visible = true;
    };
} // namespace LmbrCentral
