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
