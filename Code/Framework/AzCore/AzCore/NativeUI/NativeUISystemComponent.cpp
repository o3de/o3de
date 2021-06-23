/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/NativeUI/NativeUISystemComponent.h>

namespace AZ::NativeUI
{
    NativeUISystem::NativeUISystem()
    {
        NativeUIRequestBus::Handler::BusConnect();
    }

    NativeUISystem::~NativeUISystem()
    {
        NativeUIRequestBus::Handler::BusDisconnect();
    }

    AssertAction NativeUISystem::DisplayAssertDialog(const AZStd::string& message) const
    {
        static const char* buttonNames[3] = { "Ignore", "Ignore All", "Break" };
        AZStd::vector<AZStd::string> options;
        options.push_back(buttonNames[0]);
#if AZ_TRAIT_SHOW_IGNORE_ALL_ASSERTS_OPTION
        options.push_back(buttonNames[1]);
#endif
        options.push_back(buttonNames[2]);
        AZStd::string result;
        result = DisplayBlockingDialog("Assert Failed!", message, options);

        if (result.compare(buttonNames[0]) == 0)
            return AssertAction::IGNORE_ASSERT;
        else if (result.compare(buttonNames[1]) == 0)
            return AssertAction::IGNORE_ALL_ASSERTS;
        else if (result.compare(buttonNames[2]) == 0)
            return AssertAction::BREAK;

        return AssertAction::NONE;
    }

    AZStd::string NativeUISystem::DisplayOkDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const
    {
        AZStd::vector<AZStd::string> options;

        options.push_back("OK");
        if (showCancel)
        {
            options.push_back("Cancel");
        }

        return DisplayBlockingDialog(title, message, options);
    }

    AZStd::string NativeUISystem::DisplayYesNoDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const
    {
        AZStd::vector<AZStd::string> options;

        options.push_back("Yes");
        options.push_back("No");
        if (showCancel)
        {
            options.push_back("Cancel");
        }

        return DisplayBlockingDialog(title, message, options);
    }
} // namespace AZ::NativeUI
