/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ACES/Aces.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <QImage>

namespace MaterialEditor
{
    using MaterialViewportPresetNameSet = AZStd::set<AZStd::string>;

    class MaterialViewportRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Reload all presets
        virtual void ReloadContent() = 0;

        //! Add lighting preset
        //! @param preset lighting preset to add for selection
        //! @returns pointer to new, managed preset
        virtual AZ::Render::LightingPresetPtr AddLightingPreset(const AZ::Render::LightingPreset& preset) = 0;

        //! Get lighting presets
        //! @returns all presets
        virtual AZ::Render::LightingPresetPtrVector GetLightingPresets() const = 0;

        //! Save lighting preset
        //! @returns true if preset was saved, otherwise false
        virtual bool SaveLightingPreset(AZ::Render::LightingPresetPtr preset, const AZStd::string& path) const = 0;

        //! Get lighting preset by name
        //! @param name preset name to search for
        //! @returns the requested preset if found, otherwise nullptr
        virtual AZ::Render::LightingPresetPtr GetLightingPresetByName(const AZStd::string& name) const = 0;

        //! Get selected lighting preset
        //! @returns selected preset if found, otherwise nullptr
        virtual AZ::Render::LightingPresetPtr GetLightingPresetSelection() const = 0;

        //! Select lighting preset
        //! @param preset to select
        virtual void SelectLightingPreset(AZ::Render::LightingPresetPtr preset) = 0;

        //! Select lighting preset by name
        //! @param name preset name to select
        virtual void SelectLightingPresetByName(const AZStd::string& name) = 0;

        //! Get set of lighting preset names
        virtual MaterialViewportPresetNameSet GetLightingPresetNames() const = 0;

        //! Get model preset last save path
        //! @param preset to lookup last save path
        virtual AZStd::string GetLightingPresetLastSavePath(AZ::Render::LightingPresetPtr preset) const = 0;

        //! Add model preset
        //! @param preset model preset to add for selection
        //! @returns pointer to new, managed preset
        virtual AZ::Render::ModelPresetPtr AddModelPreset(const AZ::Render::ModelPreset& preset) = 0;

        //! Get model presets
        //! @returns all presets
        virtual AZ::Render::ModelPresetPtrVector GetModelPresets() const = 0;

        //! Save Model preset
        //! @returns true if preset was saved, otherwise false
        virtual bool SaveModelPreset(AZ::Render::ModelPresetPtr preset, const AZStd::string& path) const = 0;

        //! Get model preset by name
        //! @param name preset name to search for
        //! @returns the requested preset if found, otherwise nullptr
        virtual AZ::Render::ModelPresetPtr GetModelPresetByName(const AZStd::string& name) const = 0;

        //! Get selected model preset
        //! @returns selected preset if found, otherwise nullptr
        virtual AZ::Render::ModelPresetPtr GetModelPresetSelection() const = 0;

        //! Select lighting preset
        //! @param name preset to select
        virtual void SelectModelPreset(AZ::Render::ModelPresetPtr preset) = 0;

        //! Select model preset by name
        //! @param name preset name to select
        virtual void SelectModelPresetByName(const AZStd::string& name) = 0;

        //! Get set of model preset names
        virtual MaterialViewportPresetNameSet GetModelPresetNames() const = 0;

        //! Get model preset last save path
        //! @param preset to lookup last save path
        virtual AZStd::string GetModelPresetLastSavePath(AZ::Render::ModelPresetPtr preset) const = 0;

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

    using MaterialViewportRequestBus = AZ::EBus<MaterialViewportRequests>;
} // namespace MaterialEditor
