/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/NativeUI/NativeUISystemComponent.h>

// QT_TRANSLATE_NOOP marks strings for extraction by Qt lupdate tool.
// AzCore does not depend on Qt, so we define the macro as a no-op passthrough
// if it is not already provided by Qt headers. The marked strings can be
// extracted into .ts translation files and translated offline. Actual runtime
// translation requires a higher-level integration (e.g. AzToolsFramework or
// platform-specific translation lookup).
#include <AzCore/i18n/TranslationMacros.h>

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
        if (m_mode == NativeUI::Mode::DISABLED)
        {
            return AssertAction::NONE;
        }

        // Button names for the assert dialog. These are self-contained: the same
        // array is used for both display and comparison, so translation is safe.
        static constexpr const char* buttonNames[3] = {
            QT_TRANSLATE_NOOP("AzCore", "Ignore"),
            QT_TRANSLATE_NOOP("AzCore", "Ignore All"),
            QT_TRANSLATE_NOOP("AzCore", "Break")
        };
        AZStd::vector<AZStd::string> options;
        options.push_back(buttonNames[0]);
#if AZ_TRAIT_SHOW_IGNORE_ALL_ASSERTS_OPTION
        options.push_back(buttonNames[1]);
#endif
        options.push_back(buttonNames[2]);

        AZStd::string result = DisplayBlockingDialog(
            QT_TRANSLATE_NOOP("AzCore", "Assert Failed!"), message, options);

        if (result.compare(buttonNames[0]) == 0)
        {
            return AssertAction::IGNORE_ASSERT;
        }
        else if (result.compare(buttonNames[1]) == 0)
        {
            return AssertAction::IGNORE_ALL_ASSERTS;
        }
        else if (result.compare(buttonNames[2]) == 0)
        {
            return AssertAction::BREAK;
        }

        return AssertAction::NONE;
    }

    OkDialogResult NativeUISystem::DisplayOkDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const
    {
        if (m_mode == NativeUI::Mode::DISABLED)
        {
            return OkDialogResult::None;
        }

        // Button names are kept in local variables so the same strings are used
        // for both display and result comparison, making runtime translation safe.
        static constexpr const char* okButton = QT_TRANSLATE_NOOP("AzCore", "OK");
        static constexpr const char* cancelButton = QT_TRANSLATE_NOOP("AzCore", "Cancel");

        AZStd::vector<AZStd::string> options{ okButton };

        if (showCancel)
        {
            options.push_back(cancelButton);
        }

        AZStd::string result = DisplayBlockingDialog(title, message, options);

        if (result == okButton)
        {
            return OkDialogResult::OK;
        }
        else if (result == cancelButton)
        {
            return OkDialogResult::Cancel;
        }

        return OkDialogResult::None;
    }

    YesNoDialogResult NativeUISystem::DisplayYesNoDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const
    {
        if (m_mode == NativeUI::Mode::DISABLED)
        {
            return YesNoDialogResult::None;
        }

        // Button names are kept in local variables so the same strings are used
        // for both display and result comparison, making runtime translation safe.
        static constexpr const char* yesButton = QT_TRANSLATE_NOOP("AzCore", "Yes");
        static constexpr const char* noButton = QT_TRANSLATE_NOOP("AzCore", "No");
        static constexpr const char* cancelButton = QT_TRANSLATE_NOOP("AzCore", "Cancel");

        AZStd::vector<AZStd::string> options{ yesButton, noButton };

        if (showCancel)
        {
            options.push_back(cancelButton);
        }

        AZStd::string result = DisplayBlockingDialog(title, message, options);

        if (result == yesButton)
        {
            return YesNoDialogResult::Yes;
        }
        else if (result == noButton)
        {
            return YesNoDialogResult::No;
        }
        else if (result == cancelButton)
        {
            return YesNoDialogResult::Cancel;
        }

        return YesNoDialogResult::None;
    }
} // namespace AZ::NativeUI
