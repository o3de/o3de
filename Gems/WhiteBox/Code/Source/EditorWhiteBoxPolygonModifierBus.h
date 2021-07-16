/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Notification bus for polygon related changes.
    class EditorWhiteBoxPolygonModifierNotifications : public AZ::EntityComponentBus
    {
    public:
        //! Was the polygon handle changed/updated by the polygon modifier.
        //! @note This will happen during an append/extrusion.
        virtual void OnPolygonModifierUpdatedPolygonHandle(
            [[maybe_unused]] const Api::PolygonHandle& previousPolygonHandle,
            [[maybe_unused]] const Api::PolygonHandle& nextPolygonHandle)
        {
        }

    protected:
        ~EditorWhiteBoxPolygonModifierNotifications() = default;
    };

    using EditorWhiteBoxPolygonModifierNotificationBus = AZ::EBus<EditorWhiteBoxPolygonModifierNotifications>;
} // namespace WhiteBox
