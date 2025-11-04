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

namespace OpenParticleSystemEditor
{
    using OpenParticleViewportPresetNameSet = AZStd::set<AZStd::string>;

    class OpenParticleViewportRequests
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
        virtual OpenParticleViewportPresetNameSet GetLightingPresets() const = 0;

        //! Get lighting preset by name
        //! @param name preset name to search for
        //! @returns the requested preset if found, otherwise nullptr
        virtual AZ::Render::LightingPresetPtr GetLightingPresetByName(const AZStd::string& name) const = 0;

        //! Get selected lighting preset
        //! @returns selected preset if found, otherwise nullptr
        virtual AZStd::string GetLightingPresetSelection() const = 0;

        //! Select lighting preset
        //! @param preset to select
        virtual void SelectLightingPreset(AZ::Render::LightingPresetPtr preset) = 0;

        //! Select lighting preset by name
        //! @param name preset name to select
        virtual void SelectLightingPresetByName(const AZStd::string& name) = 0;

        //! Set enabled state for grid
        virtual void SetGridEnabled(bool enable) = 0;

        //! Set enabled state for alternate skybox
        virtual void SetAlternateSkyboxEnabled(bool enable) = 0;

        //! Get enabled state for alternate skybox
        virtual bool GetAlternateSkyboxEnabled() const = 0;
    };

    using OpenParticleViewportRequestBus = AZ::EBus<OpenParticleViewportRequests>;
} // namespace OpenParticleSystemEditor
