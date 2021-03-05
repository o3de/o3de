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

#include <AzCore/Component/Component.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>

namespace MaterialEditor
{
    //! MaterialViewportComponent registers reflected datatypes and manages different configurations for lighting and models displayed in the viewport
    class MaterialViewportComponent
        : public AZ::Component
        , private MaterialViewportRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialViewportComponent, "{A92305C3-32AB-4D50-BE4D-430FCF436C4E}");

        MaterialViewportComponent();
        ~MaterialViewportComponent() = default;
        MaterialViewportComponent(const MaterialViewportComponent&) = delete;
        MaterialViewportComponent& operator =(const MaterialViewportComponent&) = delete;

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
        // MaterialViewportRequestBus::Handler overrides ...
        void ReloadContent() override;

        AZ::Render::LightingPresetPtr AddLightingPreset(const AZ::Render::LightingPreset& preset) override;
        AZ::Render::LightingPresetPtrVector GetLightingPresets() const override;
        bool SaveLightingPresetSelection(const AZStd::string& path) const override;
        AZ::Render::LightingPresetPtr GetLightingPresetByName(const AZStd::string& name) const override;
        AZ::Render::LightingPresetPtr GetLightingPresetSelection() const override;
        void SelectLightingPreset(AZ::Render::LightingPresetPtr preset) override;
        void SelectLightingPresetByName(const AZStd::string& name) override;
        MaterialViewportPresetNameSet GetLightingPresetNames() const override;

        AZ::Render::ModelPresetPtr AddModelPreset(const AZ::Render::ModelPreset& preset) override;
        AZ::Render::ModelPresetPtrVector GetModelPresets() const override;
        bool SaveModelPresetSelection(const AZStd::string& path) const override;
        AZ::Render::ModelPresetPtr GetModelPresetByName(const AZStd::string& name) const override;
        AZ::Render::ModelPresetPtr GetModelPresetSelection() const override;
        void SelectModelPreset(AZ::Render::ModelPresetPtr preset) override;
        void SelectModelPresetByName(const AZStd::string& name) override;
        MaterialViewportPresetNameSet GetModelPresetNames() const override;

        void SetShadowCatcherEnabled(bool enable) override;
        bool GetShadowCatcherEnabled() const override;
        void SetGridEnabled(bool enable) override;
        bool GetGridEnabled() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogLoaded(const char* catalogFile) override;
        ////////////////////////////////////////////////////////////////////////

        AZ::Render::LightingPresetPtrVector m_lightingPresetVector;
        AZ::Render::LightingPresetPtr m_lightingPresetSelection;

        AZ::Render::ModelPresetPtrVector m_modelPresetVector;
        AZ::Render::ModelPresetPtr m_modelPresetSelection;

        bool m_shadowCatcherEnabled = true;
        bool m_gridEnabled = true;
    };
}
