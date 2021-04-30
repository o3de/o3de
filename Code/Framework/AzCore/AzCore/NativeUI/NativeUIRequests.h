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
