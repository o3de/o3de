/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/string/string.h>

class QWidget;

namespace AzToolsFramework
{
    //! HotKeyWidgetRegistrationInterface
    //! Interface to manage hotkeys in the Editor.
    class HotKeyWidgetRegistrationInterface
    {
    public:
        AZ_RTTI(HotKeyWidgetRegistrationInterface, "{B9197853-9655-4B72-885C-F02B9B63164F}");

        //! Tries assigning a widget to an Action Context.
        //! If the system has not yet been initialized, the system will store the arguments and assign the widgets
        //! after the registration hooks have been called.
        //! @param contextIdentifier The identifier of the action context to assign the widget to.
        //! @param widget The widget to assign to the action context.
        virtual void AssignWidgetToActionContext(const AZStd::string& actionContextIdentifier, QWidget* widget) = 0;
    };

} // namespace AzToolsFramework
