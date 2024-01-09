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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>

namespace AZ::Render
{
    constexpr AZStd::string_view RenderOptionsMenuIdentifier = "o3de.menu.editor.viewport.renderOptions";

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
        for (const auto& [passName, actionName] : m_passToActionNames)
        {
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = passName.GetCStr();

            m_actionManagerInterface->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionName,
                actionProperties,
                [passNameCpy = passName]
                {
                    const auto pipeline = GetDefaultViewportPipelinePtr();
                    if (pipeline)
                    {
                        EnablePass(*pipeline, passNameCpy, !IsPassEnabled(*pipeline, passNameCpy));
                    }
                },
                [passNameCpy = passName]() -> bool
                {
                    const auto pipeline = GetDefaultViewportPipelinePtr();
                    return pipeline ? IsPassEnabled(*pipeline, passNameCpy) : false;
                });
        }
    }

    void AtomRenderOptionsActionHandler::OnMenuBindingHook()
    {
        int i = 0;
        for (const auto& [passName, actionName] : m_passToActionNames)
        {
            m_menuManagerInterface->AddActionToMenu(RenderOptionsMenuIdentifier, actionName, 100 + i++);
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

        AZStd::vector<AZ::Name> passNames;
        GetViewportOptionsPasses(*pipeline.get(), passNames);
        for (const AZ::Name& name : passNames)
        {
            m_passToActionNames[name] = AZStd::string::format("o3de.action.viewport.renderOptions.%s", name.GetCStr());
        }
    }

} // namespace AZ::Render
