/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ACES/Aces.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Application/Application.h>
#include <OpenParticleViewportRequestBus.h>
#include <Viewport/OpenParticleViewportSettings.h>

namespace OpenParticleSystemEditor
{
    //! OpenParticleViewportComponent registers reflected datatypes and manages different configurations for lighting and models displayed in the viewport
    class OpenParticleViewportComponent
        : public AZ::Component
        , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
        , private OpenParticleViewportRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        AZ_COMPONENT(OpenParticleViewportComponent, "{5F44A79C-2692-4AC5-AA84-C8FFB0A5E8C0}");

        OpenParticleViewportComponent();
        ~OpenParticleViewportComponent() = default;
        OpenParticleViewportComponent(const OpenParticleViewportComponent&) = delete;
        OpenParticleViewportComponent& operator=(const OpenParticleViewportComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // OpenParticleViewportRequestBus::Handler overrides ...
        void ReloadContent() override;

        AZ::Render::LightingPresetPtr AddLightingPreset(const AZ::Render::LightingPreset& preset) override;
        OpenParticleViewportPresetNameSet GetLightingPresets() const override;
        AZ::Render::LightingPresetPtr GetLightingPresetByName(const AZStd::string& name) const override;
        AZStd::string GetLightingPresetSelection() const override;
        void SelectLightingPreset(AZ::Render::LightingPresetPtr preset) override;
        void SelectLightingPresetByName(const AZStd::string& name) override;

        void SetGridEnabled(bool enable) override;
        void SetAlternateSkyboxEnabled(bool enable) override;
        bool GetAlternateSkyboxEnabled() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogLoaded(const char* catalogFile) override;
        ////////////////////////////////////////////////////////////////////////

        void OnApplicationAboutToStop() override;

        AZ::Render::LightingPresetPtrVector m_lightingPresetVector;
        OpenParticleViewportPresetNameSet m_lightingPresetNameSet;
        AZ::Render::LightingPresetPtr m_lightingPresetSelection;
        AZStd::string m_lightingPresetNameSelection;

        AZ::Render::ModelPresetPtrVector m_modelPresetVector;
        AZ::Render::ModelPresetPtr m_modelPresetSelection;

        mutable AZStd::map<AZStd::string, AZ::Render::LightingPresetPtr> m_lightingPresetDisplayNameMap;

        AZStd::intrusive_ptr<OpenParticleViewportSettings> m_viewportSettings;
    };
}
