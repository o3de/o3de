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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>

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
        virtual bool SaveLightingPresetSelection(const AZStd::string& path) const = 0;

        //! Get lighting preset by name
        //! @param name preset name to search for
        //! @returns the requested preset if found, otherwise nullptr
        virtual AZ::Render::LightingPresetPtr GetLightingPresetByName(const AZStd::string& name) const = 0;

        //! Get selected lighting preset
        //! @returns selected preset if found, otherwise nullptr
        virtual AZ::Render::LightingPresetPtr GetLightingPresetSelection() const = 0;

        //! Select lighting preset
        //! @param name preset to select
        virtual void SelectLightingPreset(AZ::Render::LightingPresetPtr preset) = 0;

        //! Select lighting preset by name
        //! @param name preset name to select
        virtual void SelectLightingPresetByName(const AZStd::string& name) = 0;

        //! Get set of lighting preset names
        virtual MaterialViewportPresetNameSet GetLightingPresetNames() const = 0;

        //! Add model preset
        //! @param preset model preset to add for selection
        //! @returns pointer to new, managed preset
        virtual AZ::Render::ModelPresetPtr AddModelPreset(const AZ::Render::ModelPreset& preset) = 0;

        //! Get model presets
        //! @returns all presets
        virtual AZ::Render::ModelPresetPtrVector GetModelPresets() const = 0;

        //! Save Model preset
        //! @returns true if preset was saved, otherwise false
        virtual bool SaveModelPresetSelection(const AZStd::string& path) const = 0;

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

        //! Set enabled state for shadow catcher
        virtual void SetShadowCatcherEnabled(bool enable) = 0;

        //! Get enabled state for shadow catcher
        virtual bool GetShadowCatcherEnabled() const = 0;

        //! Set enabled state for grid
        virtual void SetGridEnabled(bool enable) = 0;

        //! Get enabled state for grid
        virtual bool GetGridEnabled() const = 0;
    };

    using MaterialViewportRequestBus = AZ::EBus<MaterialViewportRequests>;
} // namespace MaterialEditor
