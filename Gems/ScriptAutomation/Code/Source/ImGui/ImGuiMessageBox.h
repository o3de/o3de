/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Windowing/WindowBus.h>

struct ImGuiContext;

namespace ScriptAutomation
{
    //! Shows a simple message or confirmation dialog.
    class ImGuiMessageBox
    {
    public:

        void OpenPopupMessage(AZStd::string title, AZStd::string message);
        void OpenPopupConfirmation(AZStd::string title, AZStd::string message, AZStd::function<void()> okAction, AZStd::string okButton = "OK", AZStd::string cancelButton = "Cancel");

        void TickPopup();

    private:
        enum class Type
        {
            Ok,
            OkCancel
        } m_type;

        enum class State
        {
            Closed,
            Opening,
            Open
        };

        AZStd::string m_title;
        AZStd::string m_message;
        AZStd::string m_okButtonLabel;
        AZStd::string m_cancelButtonLabel;
        AZStd::function<void()> m_okAction;
        State m_state = State::Closed;
    };

} // namespace ScriptAutomation
