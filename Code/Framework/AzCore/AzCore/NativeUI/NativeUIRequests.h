/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AZ::NativeUI
{
    enum AssertAction
    {
        IGNORE_ASSERT = 0,
        IGNORE_ALL_ASSERTS,
        BREAK,
        NONE,
    };

    class NativeUIRequests
    {
    public:
        AZ_RTTI(NativeUIRequests, "{48361EE6-C1E7-4965-A13A-7425B2691817}");
        virtual ~NativeUIRequests() = default;

        // Waits for user to select an option before execution continues
        // Returns the option string selected by the user
        virtual AZStd::string DisplayBlockingDialog(const AZStd::string& /*title*/, const AZStd::string& /*message*/, const AZStd::vector<AZStd::string>& /*options*/) const { return ""; };

        // Waits for user to select an option ('Ok' or optionally 'Cancel') before execution continues
        // Returns the option string selected by the user
        virtual AZStd::string DisplayOkDialog(const AZStd::string& /*title*/, const AZStd::string& /*message*/, bool /*showCancel*/) const { return ""; };

        // Waits for user to select an option ('Yes', 'No' or optionally 'Cancel') before execution continues
        // Returns the option string selected by the user
        virtual AZStd::string DisplayYesNoDialog(const AZStd::string& /*title*/, const AZStd::string& /*message*/, bool /*showCancel*/) const { return ""; };

        // Displays an assert dialog box
        // Returns the action selected by the user
        virtual AssertAction DisplayAssertDialog(const AZStd::string& /*message*/) const { return AssertAction::NONE; };
    };

    class NativeUIEBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
    };

    using NativeUIRequestBus = AZ::EBus<NativeUIRequests, NativeUIEBusTraits>;
} // namespace AZ::NativeUI
