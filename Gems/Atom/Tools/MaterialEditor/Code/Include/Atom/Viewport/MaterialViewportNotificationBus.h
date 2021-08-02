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
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>

namespace MaterialEditor
{
    class MaterialViewportNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Signal that all configs are about to be reloaded
        virtual void OnBeginReloadContent() {}

        //! Signal that all configs were reloaded
        virtual void OnEndReloadContent() {}

        //! Signal that a preset was added
        //! @param preset being added
        virtual void OnLightingPresetAdded([[maybe_unused]] AZ::Render::LightingPresetPtr preset) {}

        //! Signal that a preset was selected
        //! @param preset being selected
        virtual void OnLightingPresetSelected([[maybe_unused]] AZ::Render::LightingPresetPtr preset) {}

        //! Signal that a preset was changed
        //! @param preset being changed
        virtual void OnLightingPresetChanged([[maybe_unused]] AZ::Render::LightingPresetPtr preset) {}

        //! Signal that a preset was added
        //! @param preset being added
        virtual void OnModelPresetAdded([[maybe_unused]] AZ::Render::ModelPresetPtr preset) {}

        //! Signal that a preset was selected
        //! @param preset being selected
        virtual void OnModelPresetSelected([[maybe_unused]] AZ::Render::ModelPresetPtr preset) {}

        //! Signal that a preset was changed
        //! @param preset being changed
        virtual void OnModelPresetChanged([[maybe_unused]] AZ::Render::ModelPresetPtr preset) {}

        //! Notify when enabled state for shadow catcher changes
        virtual void OnShadowCatcherEnabledChanged([[maybe_unused]] bool enable) {}

        //! Notify when enabled state for grid changes
        virtual void OnGridEnabledChanged([[maybe_unused]] bool enable) {}

        //! Notify when enabled state for alternate skybox changes
        virtual void OnAlternateSkyboxEnabledChanged([[maybe_unused]] bool enable) {}

        //! Notify when field of view changes
        virtual void OnFieldOfViewChanged([[maybe_unused]] float fieldOfView) {}

        //! Notify when tone mapping changes
        virtual void OnDisplayMapperOperationTypeChanged([[maybe_unused]] AZ::Render::DisplayMapperOperationType operationType) {}
    };

    using MaterialViewportNotificationBus = AZ::EBus<MaterialViewportNotifications>;
} // namespace MaterialEditor
