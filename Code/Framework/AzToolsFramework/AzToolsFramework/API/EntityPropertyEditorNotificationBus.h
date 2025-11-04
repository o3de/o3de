/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    //! Provides a bus to notify when EntityPropertyEditor is interacted with.
    class EntityPropertyEditorNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Notifies whenever component selection changes on an EntityPropertyEditor
        virtual void OnComponentSelectionChanged(
            [[maybe_unused]] EntityPropertyEditor* entityPropertyEditor,
            [[maybe_unused]] const AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedEntityComponentIds)
        {}
    };

    using EntityPropertyEditorNotificationBus = AZ::EBus<EntityPropertyEditorNotifications>;
} // namespace AzToolsFramework

AZ_DECLARE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::EntityPropertyEditorNotifications);
