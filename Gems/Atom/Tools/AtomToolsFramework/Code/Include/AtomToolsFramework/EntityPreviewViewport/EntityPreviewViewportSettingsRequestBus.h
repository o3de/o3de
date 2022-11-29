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
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! EntityPreviewViewportSettingsRequests provides an interface for various settings that affect what is displayed in the viewport
    class EntityPreviewViewportSettingsRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        //! Set current lighting preset
        //! @param preset lighting preset to assign
        virtual void SetLightingPreset(const AZ::Render::LightingPreset& preset) = 0;

        //! Get current lighting preset
        //! @returns current preset
        virtual const AZ::Render::LightingPreset& GetLightingPreset() const = 0;

        //! Save lighting preset
        //! @returns true if preset was saved, otherwise false
        virtual bool SaveLightingPreset(const AZStd::string& path) = 0;

        //! Load lighting preset
        //! @returns true if preset was loaded, otherwise false
        virtual bool LoadLightingPreset(const AZStd::string& path) = 0;

        //! Load lighting preset
        //! @returns true if preset was loaded, otherwise false
        virtual bool LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId) = 0;

        //! Get last lighting preset path
        virtual AZStd::string GetLastLightingPresetPath() const = 0;

        //! Get last lighting preset path
        virtual AZStd::string GetLastLightingPresetPathWithoutAlias() const = 0;

        //! Get last lighting preset asset id
        virtual AZ::Data::AssetId GetLastLightingPresetAssetId() const = 0;

        //! Register a selectable lighting preset path
        virtual void RegisterLightingPresetPath(const AZStd::string& path) = 0;

        //! Unregister a lighting preset path so it's no longer available for selection
        virtual void UnregisterLightingPresetPath(const AZStd::string& path) = 0;

        //! Get a set of registered lighting preset pads available for selection
        virtual AZStd::set<AZStd::string> GetRegisteredLightingPresetPaths() const = 0;

        //! Set current model preset
        //! @param preset Model preset to assign
        virtual void SetModelPreset(const AZ::Render::ModelPreset& preset) = 0;

        //! Get current model preset
        //! @returns current preset
        virtual const AZ::Render::ModelPreset& GetModelPreset() const = 0;

        //! Save model preset
        //! @returns true if preset was saved, otherwise false
        virtual bool SaveModelPreset(const AZStd::string& path) = 0;

        //! Load model preset
        //! @returns true if preset was loaded, otherwise false
        virtual bool LoadModelPreset(const AZStd::string& path) = 0;

        //! Load model preset
        //! @returns true if preset was loaded, otherwise false
        virtual bool LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId) = 0;

        //! Get last model preset path
        virtual AZStd::string GetLastModelPresetPath() const = 0;

        //! Get last model preset path
        virtual AZStd::string GetLastModelPresetPathWithoutAlias() const = 0;

        //! Get last model preset asset id
        virtual AZ::Data::AssetId GetLastModelPresetAssetId() const = 0;

        //! Register a selectable model preset path
        virtual void RegisterModelPresetPath(const AZStd::string& path) = 0;

        //! Unregister a model preset path so it's no longer available for selection
        virtual void UnregisterModelPresetPath(const AZStd::string& path) = 0;

        //! Get a set of registered model preset pads available for selection
        virtual AZStd::set<AZStd::string> GetRegisteredModelPresetPaths() const = 0;

        //! Load render pipeline
        //! @returns true if render pipeline was loaded, otherwise false
        virtual bool LoadRenderPipeline(const AZStd::string& path) = 0;

        //! Load render pipeline
        //! @returns true if render pipeline was loaded, otherwise false
        virtual bool LoadRenderPipelineByAssetId(const AZ::Data::AssetId& assetId) = 0;

        //! Get last render pipeline path
        virtual AZStd::string GetLastRenderPipelinePath() const = 0;

        //! Get last render pipeline path
        virtual AZStd::string GetLastRenderPipelinePathWithoutAlias() const = 0;

        //! Get last render pipeline asset id
        virtual AZ::Data::AssetId GetLastRenderPipelineAssetId() const = 0;

        //! Register a selectable render pipeline path
        virtual void RegisterRenderPipelinePath(const AZStd::string& path) = 0;

        //! Unregister a render pipeline path so it's no longer available for selection
        virtual void UnregisterRenderPipelinePath(const AZStd::string& path) = 0;

        //! Get a set of registered render pipeline pads available for selection
        virtual AZStd::set<AZStd::string> GetRegisteredRenderPipelinePaths() const = 0;

        //! Preload and register a preset with a system without selecting or activating it.
        virtual void PreloadPreset(const AZStd::string& path) = 0;

        //! Set enabled state for shadow catcher
        virtual void SetShadowCatcherEnabled(bool enable) = 0;

        //! Get enabled state for shadow catcher
        virtual bool GetShadowCatcherEnabled() const = 0;

        //! Set enabled state for grid
        virtual void SetGridEnabled(bool enable) = 0;

        //! Get enabled state for grid
        virtual bool GetGridEnabled() const = 0;

        //! Set enabled state for alternate skybox
        virtual void SetAlternateSkyboxEnabled(bool enable) = 0;

        //! Get enabled state for alternate skybox
        virtual bool GetAlternateSkyboxEnabled() const = 0;

        //! Set field of view
        virtual void SetFieldOfView(float fieldOfView) = 0;

        //! Get field of view
        virtual float GetFieldOfView() const = 0;

        //! Set tone mapping type
        virtual void SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType) = 0;

        //! Get tone mapping type
        virtual AZ::Render::DisplayMapperOperationType GetDisplayMapperOperationType() const = 0;
    };

    using EntityPreviewViewportSettingsRequestBus = AZ::EBus<EntityPreviewViewportSettingsRequests>;
} // namespace AtomToolsFramework
