/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    //! Used to synchronize registrations to the Action Manager across all sources.
    //! Hooks are called in the order they are defined in this bus. This allows different classes
    //! to synchronize and use contexts/actions/toolbars/menus etc. that are defined by other sources.
    class ActionManagerRegistrationNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        //! Synchronization signal to register Action Contexts.
        virtual void OnActionContextRegistrationHook() {}

        //! Synchronization signal to register Action Context Modes.
        virtual void OnActionContextModeRegistrationHook() {}

        //! Synchronization signal to register Action Updaters.
        virtual void OnActionUpdaterRegistrationHook() {}

        //! Synchronization signal to register Menu Bars.
        virtual void OnMenuBarRegistrationHook() {}

        //! Synchronization signal to register Menus.
        virtual void OnMenuRegistrationHook() {}

        //! Synchronization signal to register ToolBar Areas.
        virtual void OnToolBarAreaRegistrationHook() {}

        //! Synchronization signal to register ToolBars.
        virtual void OnToolBarRegistrationHook() {}

        //! Synchronization signal to register Actions.
        virtual void OnActionRegistrationHook() {}

        //! Synchronization signal to register Widget Actions.
        virtual void OnWidgetActionRegistrationHook() {}

        //! Synchronization signal to bind Actions to Action Context Modes.
        virtual void OnActionContextModeBindingHook() {}

        //! Synchronization signal to add actions/widgets to Menus.
        virtual void OnMenuBindingHook() {}

        //! Synchronization signal to add actions/widgets/menus to ToolBars.
        virtual void OnToolBarBindingHook() {}

        //! Synchronization signal for any post-registration activity.
        virtual void OnPostActionManagerRegistrationHook() {}

    protected:
        ~ActionManagerRegistrationNotifications() = default;
    };

    using ActionManagerRegistrationNotificationBus = AZ::EBus<ActionManagerRegistrationNotifications>;

} // namespace AzToolsFramework
