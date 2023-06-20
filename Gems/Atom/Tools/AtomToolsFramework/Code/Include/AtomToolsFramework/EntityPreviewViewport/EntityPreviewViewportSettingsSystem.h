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
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace AtomToolsFramework
{
    //! EntityPreviewViewportSettingsSystem manages storing and retrieving different viewport settings, lighting, and model presets
    class EntityPreviewViewportSettingsSystem final
        : public EntityPreviewViewportSettingsRequestBus::Handler
        , public AZ::TickBus::Handler
        , public AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        AZ_RTTI(EntityPreviewViewportSettingsSystem, "{3CA1A2F4-AD6E-478A-B1E8-565E66BD5B69}");
        AZ_CLASS_ALLOCATOR(EntityPreviewViewportSettingsSystem, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(EntityPreviewViewportSettingsSystem);

        static void Reflect(AZ::ReflectContext* context);

        EntityPreviewViewportSettingsSystem() = default;
        EntityPreviewViewportSettingsSystem(const AZ::Crc32& toolId);
        ~EntityPreviewViewportSettingsSystem();

        void ClearContent();

        // EntityPreviewViewportSettingsRequestBus::Handler overrides ...
        void SetLightingPreset(const AZ::Render::LightingPreset& preset) override;
        const AZ::Render::LightingPreset& GetLightingPreset() const override;
        bool SaveLightingPreset(const AZStd::string& path) override;
        bool LoadLightingPreset(const AZStd::string& path) override;
        AZStd::string GetLastLightingPresetPath() const override;
        AZStd::string GetLastLightingPresetPathWithoutAlias() const override;
        bool LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId) override;
        AZ::Data::AssetId GetLastLightingPresetAssetId() const override;
        void RegisterLightingPresetPath(const AZStd::string& path) override;
        void UnregisterLightingPresetPath(const AZStd::string& path) override;
        AZStd::set<AZStd::string> GetRegisteredLightingPresetPaths() const override;

        void SetModelPreset(const AZ::Render::ModelPreset& preset) override;
        const AZ::Render::ModelPreset& GetModelPreset() const override;
        bool SaveModelPreset(const AZStd::string& path) override;
        bool LoadModelPreset(const AZStd::string& path) override;
        bool LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId) override;
        AZStd::string GetLastModelPresetPath() const override;
        AZStd::string GetLastModelPresetPathWithoutAlias() const override;
        AZ::Data::AssetId GetLastModelPresetAssetId() const override;
        void RegisterModelPresetPath(const AZStd::string& path) override;
        void UnregisterModelPresetPath(const AZStd::string& path) override;
        AZStd::set<AZStd::string> GetRegisteredModelPresetPaths() const override;

        bool LoadRenderPipeline(const AZStd::string& path) override;
        bool LoadRenderPipelineByAssetId(const AZ::Data::AssetId& assetId) override;
        AZStd::string GetLastRenderPipelinePath() const override;
        AZStd::string GetLastRenderPipelinePathWithoutAlias() const override;
        AZ::Data::AssetId GetLastRenderPipelineAssetId() const override;
        void RegisterRenderPipelinePath(const AZStd::string& path) override;
        void UnregisterRenderPipelinePath(const AZStd::string& path) override;
        AZStd::set<AZStd::string> GetRegisteredRenderPipelinePaths() const override;

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

        void PreloadPreset(const AZStd::string& path) override;

    private:
        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;

        void RegisterLightingPreset(const AZStd::string& path, const AZ::Render::LightingPreset& preset);
        void RegisterModelPreset(const AZStd::string& path, const AZ::Render::ModelPreset& preset);
        void RegisterRenderPipeline(const AZStd::string& path, const AZ::RPI::RenderPipelineDescriptor& preset);
        void PreloadPresets();

        const AZ::Crc32 m_toolId = {};

        AZ::Render::LightingPreset m_lightingPreset;
        AZ::Render::ModelPreset m_modelPreset;
        AZ::RPI::RenderPipelineDescriptor m_renderPipelineDescriptor;

        mutable AZStd::unordered_map<AZStd::string, AZ::Render::LightingPreset> m_lightingPresetCache;
        mutable AZStd::unordered_map<AZStd::string, AZ::Render::ModelPreset> m_modelPresetCache;
        mutable AZStd::unordered_map<AZStd::string, AZ::RPI::RenderPipelineDescriptor> m_renderPipelineDescriptorCache;
        mutable bool m_settingsNotificationPending = {};
    };
} // namespace AtomToolsFramework
