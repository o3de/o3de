/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRenderOptionsActionHandler.h"

#include "AtomRenderOptions.h"

#include <Atom/RPI.Public/Base.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string_view.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>

namespace AZ::Render
{
    constexpr AZStd::string_view RenderOptionsMenuIdentifier = "o3de.menu.editor.viewport.renderOptions";
    constexpr const char* RenderOptionsActionBaseFmtString = "o3de.action.viewport.renderOptions.%s";

    void AtomRenderOptionsActionHandler::Activate()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();

        if (m_actionManagerInterface && m_menuManagerInterface)
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
        m_menuManagerInterface->RegisterMenu(RenderOptionsMenuIdentifier, menuProperties);
    }

    void AtomRenderOptionsActionHandler::OnActionRegistrationHook()
    {
        for (const AZ::Name& passName : m_exposedPassNames)
        {
            const auto actionIdentifier = AZStd::string::format(RenderOptionsActionBaseFmtString, passName.GetCStr());
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = passName.GetCStr();

            m_actionManagerInterface->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [passName]
                {
                    const auto pipeline = GetDefaultViewportPipelinePtr();
                    if (pipeline)
                    {
                        EnablePass(*pipeline, passName, !IsPassEnabled(*pipeline, passName));
                    }
                },
                [passName]() -> bool
                {
                    const auto pipeline = GetDefaultViewportPipelinePtr();
                    return pipeline ? IsPassEnabled(*pipeline, passName) : false;
                });
        }
    }

    void AtomRenderOptionsActionHandler::OnMenuBindingHook()
    {
        int i = 0;
        for (const AZ::Name& passName : m_exposedPassNames)
        {
            const auto actionIdentifier = AZStd::string::format(RenderOptionsActionBaseFmtString, passName.GetCStr());
            m_menuManagerInterface->AddActionToMenu(RenderOptionsMenuIdentifier, actionIdentifier, 100 + i++);
        }

        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, 801);
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, RenderOptionsMenuIdentifier, 802);
    }

    void AtomRenderOptionsActionHandler::NotifyMainWindowInitialized([[maybe_unused]] QMainWindow* mainWindow)
    {
        const auto pipeline = GetDefaultViewportPipelinePtr();
        if (!pipeline)
        {
            AZ_Warning("AtomRenderOptions", false, "Failed to find default viewport pipeline. No render options will be visible");
        }

        GetToolExposedPasses(*pipeline.get(), m_exposedPassNames);
    }

} // namespace AZ::Render
