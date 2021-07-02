/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AZ::NativeUI
{
    enum class AssertAction
    {
        IGNORE_ASSERT = 0,
        IGNORE_ALL_ASSERTS,
        BREAK,
        NONE,
    };

    enum class Mode
    {
        DISABLED = 0,
        ENABLED,
    };

    class NativeUIRequests
    {
    public:
        AZ_RTTI(NativeUIRequests, "{48361EE6-C1E7-4965-A13A-7425B2691817}");
        virtual ~NativeUIRequests() = default;

        //! Waits for user to select an option before execution continues
        //! Returns the option string selected by the user
        virtual AZStd::string DisplayBlockingDialog(
            [[maybe_unused]] const AZStd::string& title,
            [[maybe_unused]] const AZStd::string& message,
            [[maybe_unused]] const AZStd::vector<AZStd::string>& options) const
        {
            return {};
        }

        //! Waits for user to select an option ('Ok' or optionally 'Cancel') before execution continues
        //! Returns the option string selected by the user
        virtual AZStd::string DisplayOkDialog(
            [[maybe_unused]] const AZStd::string& title,
            [[maybe_unused]] const AZStd::string& message,
            [[maybe_unused]] bool showCancel) const
        {
            return {};
        }

        //! Waits for user to select an option ('Yes', 'No' or optionally 'Cancel') before execution continues
        //! Returns the option string selected by the user
        virtual AZStd::string DisplayYesNoDialog(
            [[maybe_unused]] const AZStd::string& title,
            [[maybe_unused]] const AZStd::string& message,
            [[maybe_unused]] bool showCancel) const
        {
            return {};
        }

        //! Displays an assert dialog box
        //! Returns the action selected by the user
        virtual AssertAction DisplayAssertDialog([[maybe_unused]] const AZStd::string& message) const { return AssertAction::NONE; }

        //! Set the operation mode of the native UI systen
        void SetMode(NativeUI::Mode mode)
        {
            m_mode = mode;
        }

    protected:
        NativeUI::Mode m_mode = NativeUI::Mode::DISABLED;
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
