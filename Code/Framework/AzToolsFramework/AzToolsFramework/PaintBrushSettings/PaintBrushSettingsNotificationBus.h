/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettings.h>

namespace AzToolsFramework
{
    //! PaintBrushSettingsNotificationBus is used to send out notifications whenever the global paintbrush settings have changed.
    class PaintBrushSettingsNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Notifies listeners that the set of visible paintbrush settings has changed.
        virtual void OnVisiblePropertiesChanged()
        {
        }

        //! Notifies listeners that the paintbrush settings have changed.
        //! @param newSettings The settings after the change
        virtual void OnSettingsChanged([[maybe_unused]] const PaintBrushSettings& newSettings)
        {
        }
    };

    using PaintBrushSettingsNotificationBus = AZ::EBus<PaintBrushSettingsNotifications>;

} // namespace AzToolsFramework
