/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

namespace AzToolsFramework
{
    //! Used to notify changes in the ToolBar Manager.
    class ToolBarManagerNotifications : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Triggered when a ToolBar has been registered.
        virtual void OnToolBarRegistered([[maybe_unused]] const AZStd::string& toolBarIdentifier, [[maybe_unused]] const ToolBarProperties& properties) {}

    protected:
        ~ToolBarManagerNotifications() = default;
    };

    using ToolBarManagerNotificationBus = AZ::EBus<ToolBarManagerNotifications>;

} // namespace AzToolsFramework
