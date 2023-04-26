/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiMessageBox.h"
#include "ScriptableImGui.h"

namespace ScriptAutomation
{
    void ImGuiMessageBox::OpenPopupMessage(AZStd::string title, AZStd::string message)
    {
        AZ_Assert(m_state == State::Closed, "Popup is already open");
        m_type = Type::Ok;
        m_title = AZStd::move(title);
        m_message = AZStd::move(message);
        m_okButtonLabel = "OK";
        m_cancelButtonLabel = "";
        m_okAction = nullptr;

        m_state = State::Opening;
    }

    void ImGuiMessageBox::OpenPopupConfirmation(AZStd::string title, AZStd::string message, AZStd::function<void()> okAction, AZStd::string okButton, AZStd::string cancelButton)
    {
        AZ_Assert(m_state == State::Closed, "Popup is already open");
        m_type = Type::OkCancel;
        m_title = AZStd::move(title);
        m_message = AZStd::move(message);
        m_okButtonLabel = AZStd::move(okButton);
        m_cancelButtonLabel = AZStd::move(cancelButton);
        m_okAction = okAction;

        m_state = State::Opening;
    }

    void ImGuiMessageBox::TickPopup()
    {
        // We delay showing the popup until TickPopup() so that a single ImGuiMessageBox can be used to service multiple controls in an ImGui update function.
        // This is because the ImGui "ID" context needs to match between OpenPopup and IsPopupOpen.
        if (m_state == State::Opening)
        {
            ImGui::OpenPopup(m_title.c_str());
            m_state = State::Open;
        }

        if (ImGui::IsPopupOpen(m_title.c_str()))
        {
            const ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_AlwaysAutoResize;

            if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, windowFlags))
            {
                ImGui::Text("%s", m_message.c_str());

                ScriptableImGui::PushNameContext(m_title.c_str());

                if (ScriptableImGui::Button(m_okButtonLabel.c_str()))
                {
                    ImGui::CloseCurrentPopup();
                    m_state = State::Closed;

                    if (m_okAction)
                    {
                        m_okAction();
                    }
                }
                else if (m_type == Type::OkCancel && ScriptableImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                    m_state = State::Closed;
                }

                ScriptableImGui::PopNameContext();

                ImGui::EndPopup();
            }
        }
        else if(m_state == State::Open)
        {
            // If another ImGui window is opened while a modal popup is open, it kills the modal popup. So open it again.
            m_state = State::Opening;
        }
    }

} // namespace ScriptAutomation
