/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Notification bus for edge related changes
    class EditorWhiteBoxEdgeModifierNotifications : public AZ::EntityComponentBus
    {
    public:
        //! Was the polygon handle changed/updated by the polygon modifier.
        //! @note This will happen during an append/extrusion.
        virtual void OnEdgeModifierUpdatedEdgeHandle(
            [[maybe_unused]] Api::EdgeHandle previousEdgeHandle, [[maybe_unused]] Api::EdgeHandle nextEdgeHandle)
        {
        }

    protected:
        ~EditorWhiteBoxEdgeModifierNotifications() = default;
    };

    using EditorWhiteBoxEdgeModifierNotificationBus = AZ::EBus<EditorWhiteBoxEdgeModifierNotifications>;
} // namespace WhiteBox
