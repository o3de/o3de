/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRenderOptionsActionHandler.h"

#include "AtomRenderOptions.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Name.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>

namespace AZ::Render
{
    constexpr AZStd::string_view ViewportRenderOptionsMenuIdentifier = "o3de.menu.editor.viewport.renderoptions";

    void AtomRenderOptionsActionHandler::Activate()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        m_toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();

        if (m_actionManagerInterface && m_menuManagerInterface && m_toolBarManagerInterface)
        {
            AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
        }

        AzToolsFramework::EditorEventsBus::Handler::BusConnect();
    }

    void AtomRenderOptionsActionHandler::Deactivate()
    {
        AzToolsFramework::EditorEventsBus::Handler::BusDisconnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void AtomRenderOptionsActionHandler::OnMenuRegistrationHook()
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Render Options";
        m_menuManagerInterface->RegisterMenu(ViewportRenderOptionsMenuIdentifier, menuProperties);
    }

    void AtomRenderOptionsActionHandler::OnActionRegistrationHook()
    {
        // Render Options menu icon
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.renderoptions";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Render Options";
            actionProperties.m_iconPath = ":/Icons/Material_80.svg";

            m_actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                });
        }

        // Temporal anti aliasing
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.renderoptions.taa";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Anti-aliasing (TAA)";
            actionProperties.m_description = "Use temporal anti-aliasing";

            m_actionManagerInterface->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [this]
                {
                    if (m_taaEnabled.has_value())
                    {
                        m_taaEnabled = !(m_taaEnabled.value());
                        EnableTAA(m_taaEnabled.value());
                    }
                },
                [this]() -> bool
                {
                    // If TaaPass not found, fallback on UI always disabled
                    return m_taaEnabled.has_value() ? m_taaEnabled.value() : false;
                });
        }
    }

    void AtomRenderOptionsActionHandler::OnMenuBindingHook()
    {
        m_menuManagerInterface->AddActionToMenu(ViewportRenderOptionsMenuIdentifier, "o3de.action.viewport.renderoptions.taa", 100);
    }

    void AtomRenderOptionsActionHandler::OnToolBarBindingHook()
    {
        m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
            EditorIdentifiers::ViewportTopToolBarIdentifier,
            "o3de.action.viewport.renderoptions",
            ViewportRenderOptionsMenuIdentifier,
            601);
    }

    void AtomRenderOptionsActionHandler::NotifyMainWindowInitialized([[maybe_unused]] QMainWindow* mainWindow)
    {
        m_taaEnabled = IsPassEnabled(AZ::Name("TaaPass"));
        if (!m_taaEnabled.has_value())
        {
            AZ_Warning("AtomRenderOptions", false, "Failed to find TaaPass");
        }
    }

} // namespace AZ::Render
