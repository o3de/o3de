/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "NodeGroupsPlugin.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/NodeGroupCommands.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"

// include qt headers
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>


namespace EMStudio
{
    // constructor
    NodeGroupsPlugin::NodeGroupsPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_dialogStack                = nullptr;
        m_nodeGroupWidget            = nullptr;
        m_nodeGroupManagementWidget  = nullptr;
        m_selectCallback             = nullptr;
        m_unselectCallback           = nullptr;
        m_clearSelectionCallback     = nullptr;
        m_adjustNodeGroupCallback    = nullptr;
        m_addNodeGroupCallback       = nullptr;
        m_removeNodeGroupCallback    = nullptr;
        m_currentActor               = nullptr;
    }


    // destructor
    NodeGroupsPlugin::~NodeGroupsPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(m_selectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_unselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_adjustNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_addNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeNodeGroupCallback, false);

        // remove the callback
        delete m_selectCallback;
        delete m_unselectCallback;
        delete m_clearSelectionCallback;
        delete m_adjustNodeGroupCallback;
        delete m_addNodeGroupCallback;
        delete m_removeNodeGroupCallback;

        // clear dialogstack and delete afterwards
        delete m_dialogStack;
    }

    // init after the parent dock window has been created
    bool NodeGroupsPlugin::Init()
    {
        // create the dialog stack
        assert(m_dialogStack == nullptr);
        m_dialogStack = new MysticQt::DialogStack();
        m_dock->setMinimumWidth(300);
        m_dock->setMinimumHeight(100);
        m_dock->setWidget(m_dialogStack);

        // create the management and node group widgets
        m_nodeGroupWidget            = new NodeGroupWidget();
        m_nodeGroupManagementWidget  = new NodeGroupManagementWidget(m_nodeGroupWidget);

        // add the widgets to the dialog stack
        m_dialogStack->Add(m_nodeGroupManagementWidget, "Node Group Management", false, true, true, false);
        m_dialogStack->Add(m_nodeGroupWidget, "Node Group", false, true, true);

        // create and register the command callbacks only (only execute this code once for all plugins)
        m_selectCallback             = new CommandSelectCallback(false);
        m_unselectCallback           = new CommandUnselectCallback(false);
        m_clearSelectionCallback     = new CommandClearSelectionCallback(false);
        m_adjustNodeGroupCallback    = new CommandAdjustNodeGroupCallback(false);
        m_addNodeGroupCallback       = new CommandAddNodeGroupCallback(false);
        m_removeNodeGroupCallback    = new CommandRemoveNodeGroupCallback(false);

        GetCommandManager()->RegisterCommandCallback("Select", m_selectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", m_unselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_clearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback(CommandSystem::CommandAdjustNodeGroup::s_commandName.data(), m_adjustNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AddNodeGroup", m_addNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveNodeGroup", m_removeNodeGroupCallback);

        // reinit the dialog
        ReInit();

        // connect the window activation signal to refresh if reactivated
        connect(m_dock, &QDockWidget::visibilityChanged, this, &NodeGroupsPlugin::WindowReInit);

        return true;
    }


    // reinit the node groups dialog, e.g. if selection changes
    void NodeGroupsPlugin::ReInit()
    {
        // get the selected actor instance
        const CommandSystem::SelectionList& selection       = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance*           actorInstance   = selection.GetSingleActorInstance();

        // show hint if no/multiple actor instances is/are selected
        if (actorInstance == nullptr)
        {
            m_currentActor = nullptr;
            m_nodeGroupWidget->SetActor(nullptr);
            m_nodeGroupWidget->SetNodeGroup(nullptr);
            m_nodeGroupManagementWidget->SetActor(nullptr);
            return;
        }

        // get the corresponding actor
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // only reinit the node groups window if actorinstance changed
        if (m_currentActor != actor)
        {
            // set the new actor
            m_currentActor = actor;

            // set the new actor on each widget
            m_nodeGroupWidget->SetActor(m_currentActor);
            m_nodeGroupManagementWidget->SetActor(m_currentActor);
        }

        // set the dialog stack as main widget
        m_dock->setWidget(m_dialogStack);

        // update the interface
        UpdateInterface();
    }


    // reinit the window when it gets activated
    void NodeGroupsPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit();
        }
    }


    // update the interface
    void NodeGroupsPlugin::UpdateInterface()
    {
        m_nodeGroupManagementWidget->UpdateInterface();
        m_nodeGroupWidget->UpdateInterface();
    }


    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------

    bool ReInitNodeGroupsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(NodeGroupsPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        NodeGroupsPlugin* nodeGroupsWindow = (NodeGroupsPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (GetManager()->GetIgnoreVisibility() || nodeGroupsWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            nodeGroupsWindow->ReInit();
        }

        return true;
    }


    bool NodeGroupsPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitNodeGroupsPlugin();
    }
    bool NodeGroupsPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitNodeGroupsPlugin();
    }
    bool NodeGroupsPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitNodeGroupsPlugin();
    }
    bool NodeGroupsPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitNodeGroupsPlugin();
    }
    bool NodeGroupsPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitNodeGroupsPlugin();
    }
    bool NodeGroupsPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitNodeGroupsPlugin();
    }
    bool NodeGroupsPlugin::CommandAdjustNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeGroupsPlugin(); }
    bool NodeGroupsPlugin::CommandAdjustNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeGroupsPlugin(); }
    bool NodeGroupsPlugin::CommandAddNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeGroupsPlugin(); }
    bool NodeGroupsPlugin::CommandAddNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeGroupsPlugin(); }
    bool NodeGroupsPlugin::CommandRemoveNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeGroupsPlugin(); }
    bool NodeGroupsPlugin::CommandRemoveNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeGroupsPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/NodeGroups/moc_NodeGroupsPlugin.cpp>
