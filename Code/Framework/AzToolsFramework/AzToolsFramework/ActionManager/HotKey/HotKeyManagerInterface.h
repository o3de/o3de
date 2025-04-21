/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/string/string.h>

class QWidget;

namespace AzToolsFramework
{
    using HotKeyManagerOperationResult = AZ::Outcome<void, AZStd::string>;

    //! HotKeyManagerInterface
    //! Interface to manage hotkeys in the Editor.
    class HotKeyManagerInterface
    {
    public:
        AZ_RTTI(HotKeyManagerInterface, "{B6414B8D-E2F4-4B30-8E29-4707657FDC93}");

        //! Assign an owning widget to an Action Context.
        //! This allows actions registered to that Action Context to be triggered by shortcuts when the events reach that widget.
        //! @param contextIdentifier The identifier of the action context to assign the widget to.
        //! @param widget The widget to assign to the action context.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual HotKeyManagerOperationResult AssignWidgetToActionContext(const AZStd::string& contextIdentifier, QWidget* widget) = 0;

        //! Remove an owning widget from an Action Context.
        //! @param contextIdentifier The identifier of the action context to remove the widget from.
        //! @param widget The widget to remove from the action context.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual HotKeyManagerOperationResult RemoveWidgetFromActionContext(const AZStd::string& contextIdentifier, QWidget* widget) = 0;

        //! Set an Action's HotKey via its identifier.
        //! @param actionIdentifier The action identifier to set the hotkey to.
        //! @param hotKey The new key combination to bind the action to.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual HotKeyManagerOperationResult SetActionHotKey(const AZStd::string& actionIdentifier, const AZStd::string& hotKey) = 0;
    };

} // namespace AzToolsFramework
