/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    using HotKeyManagerOperationResult = AZ::Outcome<void, AZStd::string>;

    //! HotKeyManagerInterface
    //! Interface to manage hotkeys in the Editor.
    class HotKeyManagerInterface
    {
    public:
        AZ_RTTI(HotKeyManagerInterface, "{B6414B8D-E2F4-4B30-8E29-4707657FDC93}");

        //! Set an Action's HotKey via its identifier.
        //! @param actionIdentifier The action identifier to set the hotkey to.
        //! @param hotKey The new key combination to bind the action to.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual HotKeyManagerOperationResult SetActionHotKey(const AZStd::string& actionIdentifier, const AZStd::string& hotKey) = 0;
    };

} // namespace AzToolsFramework
