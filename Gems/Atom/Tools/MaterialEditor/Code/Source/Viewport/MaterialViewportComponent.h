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
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <Atom/Viewport/MaterialViewportSettings.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace MaterialEditor
{
    //! MaterialViewportComponent registers reflected datatypes and manages different configurations for lighting and models displayed in the viewport
    class MaterialViewportComponent
        : public AZ::Component
        , private MaterialViewportRequestBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
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

        void ClearContent();

        ////////////////////////////////////////////////////////////////////////
        // MaterialViewportRequestBus::Handler overrides ...
        void ReloadContent() override;

        AZ::Render::LightingPresetPtr AddLightingPreset(const AZ::Render::LightingPreset& preset) override;
        AZ::Render::LightingPresetPtrVector GetLightingPresets() const override;
        bool SaveLightingPreset(AZ::Render::LightingPresetPtr preset, const AZStd::string& path) const override;
        AZ::Render::LightingPresetPtr GetLightingPresetByName(const AZStd::string& name) const override;
        AZ::Render::LightingPresetPtr GetLightingPresetSelection() const override;
        void SelectLightingPreset(AZ::Render::LightingPresetPtr preset) override;
        void SelectLightingPresetByName(const AZStd::string& name) override;
        MaterialViewportPresetNameSet GetLightingPresetNames() const override;
        AZStd::string GetLightingPresetLastSavePath(AZ::Render::LightingPresetPtr preset) const override;

        AZ::Render::ModelPresetPtr AddModelPreset(const AZ::Render::ModelPreset& preset) override;
        AZ::Render::ModelPresetPtrVector GetModelPresets() const override;
        bool SaveModelPreset(AZ::Render::ModelPresetPtr preset, const AZStd::string& path) const override;
        AZ::Render::ModelPresetPtr GetModelPresetByName(const AZStd::string& name) const override;
        AZ::Render::ModelPresetPtr GetModelPresetSelection() const override;
        void SelectModelPreset(AZ::Render::ModelPresetPtr preset) override;
        void SelectModelPresetByName(const AZStd::string& name) override;
        MaterialViewportPresetNameSet GetModelPresetNames() const override;
        AZStd::string GetModelPresetLastSavePath(AZ::Render::ModelPresetPtr preset) const override;

        void SetShadowCatcherEnabled(bool enable) override;
        bool GetShadowCatcherEnabled() const override;
        void SetGridEnabled(bool enable) override;
        bool GetGridEnabled() const override;
        void SetAlternateSkyboxEnabled(bool enable) override;
        bool GetAlternateSkyboxEnabled() const override;
        void SetFieldOfView(float fieldOfView) override;
        float GetFieldOfView() const override;
        void SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType) override;
        AZ::Render::DisplayMapperOperationType GetDisplayMapperOperationType() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::MultiHandler overrides ...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogLoaded(const char* catalogFile) override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::AnyAsset>> m_lightingPresetAssets;
        AZ::Render::LightingPresetPtrVector m_lightingPresetVector;
        AZ::Render::LightingPresetPtr m_lightingPresetSelection;

        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::AnyAsset>> m_modelPresetAssets;
        AZ::Render::ModelPresetPtrVector m_modelPresetVector;
        AZ::Render::ModelPresetPtr m_modelPresetSelection;

        mutable AZStd::map<AZ::Render::LightingPresetPtr, AZStd::string> m_lightingPresetLastSavePathMap;
        mutable AZStd::map<AZ::Render::ModelPresetPtr, AZStd::string> m_modelPresetLastSavePathMap;

        AZStd::intrusive_ptr<MaterialViewportSettings> m_viewportSettings;
    };
}
