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
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettings.h>

namespace AzToolsFramework
{
    //! GlobalPaintBrushSettingsNotificationBus is used to send out notifications whenever the global paintbrush settings have changed.
    class GlobalPaintBrushSettingsNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Notifies listeners that the set of visible paintbrush settings has changed.
        virtual void OnVisiblePropertiesChanged()
        {
        }

        //! Notifies listeners that the paintbrush settings have changed.
        //! @param newSettings The settings after the change
        virtual void OnSettingsChanged([[maybe_unused]] const GlobalPaintBrushSettings& newSettings)
        {
        }

        //! Notifies listeners that the paintbrush mode has changed.
        //! @param newSettings The settings after the change
        virtual void OnPaintBrushModeChanged([[maybe_unused]] PaintBrushMode newBrushMode)
        {
        }
    };

    using GlobalPaintBrushSettingsNotificationBus = AZ::EBus<GlobalPaintBrushSettingsNotifications>;

} // namespace AzToolsFramework
