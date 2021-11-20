/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <AzFramework/Entity/EntityContext.h>

namespace AzToolsFramework
{
    //! Used to notify when the editor focus changes.
    class FocusModeNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AzFramework::EntityContextId;
        //////////////////////////////////////////////////////////////////////////
        
        //! Triggered when the editor focus is changed to a different entity.
        //! @param previousFocusEntityId The entity the focus has been moved from.
        //! @param newFocusEntityId The entity the focus has been moved to.
        virtual void OnEditorFocusChanged(
            [[maybe_unused]] AZ::EntityId previousFocusEntityId, [[maybe_unused]] AZ::EntityId newFocusEntityId) {}

    protected:
        ~FocusModeNotifications() = default;
    };

    using FocusModeNotificationBus = AZ::EBus<FocusModeNotifications>;

} // namespace AzToolsFramework
