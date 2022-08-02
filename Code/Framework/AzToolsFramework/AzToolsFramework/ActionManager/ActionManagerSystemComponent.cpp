/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/Editor/ActionManagerUtils.h>

// Temp
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

namespace AzToolsFramework
{
    ActionManagerSystemComponent::~ActionManagerSystemComponent()
    {
        if (m_defaultParentObject)
        {
            delete m_defaultParentObject;
            m_defaultParentObject = nullptr;
        }
    }

    void ActionManagerSystemComponent::Init()
    {
        if (IsNewActionManagerEnabled())
        {
            m_defaultParentObject = new QWidget();

            m_actionManager = AZStd::make_unique<ActionManager>();
            m_menuManager = AZStd::make_unique<MenuManager>(m_defaultParentObject);
            m_toolBarManager = AZStd::make_unique<ToolBarManager>(m_defaultParentObject);
        }
    }

    void ActionManagerSystemComponent::Activate()
    {
        EditorEventsBus::Handler::BusConnect();
    }

    void ActionManagerSystemComponent::Deactivate()
    {
        EditorEventsBus::Handler::BusDisconnect();
    }

    void ActionManagerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ActionManagerSystemComponent, AZ::Component>()->Version(1);
        }

        ToolBarManager::Reflect(context);
    }

    void ActionManagerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ActionManager"));
    }

    void ActionManagerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ActionManagerSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void ActionManagerSystemComponent::NotifyEditorInitialized()
    {
        ToolBarManagerInternalInterface* toolBarManagerInternalInterface = AZ::Interface<ToolBarManagerInternalInterface>::Get();

        if (toolBarManagerInternalInterface)
        {
            auto outcome = toolBarManagerInternalInterface->SerializeToolBar("o3de.toolbar.editor.playcontrols");

            if (outcome.IsSuccess())
            {
                AZ_TracePrintf("Action Manager Test", "%s", outcome.GetValue().c_str());
            }
        }
    }

} // namespace AzToolsFramework
