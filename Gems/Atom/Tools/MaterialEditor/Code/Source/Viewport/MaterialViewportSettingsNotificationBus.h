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
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        //! Notify when any setting changes
        virtual void OnViewportSettingsChanged() {}
    };

    using MaterialViewportSettingsNotificationBus = AZ::EBus<MaterialViewportNotifications>;
} // namespace MaterialEditor
