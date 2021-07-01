/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "NodeGroupsPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
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
        mDialogStack                = nullptr;
        mNodeGroupWidget            = nullptr;
        mNodeGroupManagementWidget  = nullptr;
        mSelectCallback             = nullptr;
        mUnselectCallback           = nullptr;
        mClearSelectionCallback     = nullptr;
        mAdjustNodeGroupCallback    = nullptr;
        mAddNodeGroupCallback       = nullptr;
        mRemoveNodeGroupCallback    = nullptr;
        mCurrentActor               = nullptr;
    }


    // destructor
    NodeGroupsPlugin::~NodeGroupsPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAddNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveNodeGroupCallback, false);

        // remove the callback
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mAdjustNodeGroupCallback;
        delete mAddNodeGroupCallback;
        delete mRemoveNodeGroupCallback;

        // clear dialogstack and delete afterwards
        delete mDialogStack;
    }


    // clone the log window
    EMStudioPlugin* NodeGroupsPlugin::Clone()
    {
        NodeGroupsPlugin* newPlugin = new NodeGroupsPlugin();
        return newPlugin;
    }


    // init after the parent dock window has been created
    bool NodeGroupsPlugin::Init()
    {
        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack();
        mDock->setMinimumWidth(300);
        mDock->setMinimumHeight(100);
        mDock->setWidget(mDialogStack);

        // create the management and node group widgets
        mNodeGroupWidget            = new NodeGroupWidget();
        mNodeGroupManagementWidget  = new NodeGroupManagementWidget(mNodeGroupWidget);

        // add the widgets to the dialog stack
        mDialogStack->Add(mNodeGroupManagementWidget, "Node Group Management", false, true, true, false);
        mDialogStack->Add(mNodeGroupWidget, "Node Group", false, true, true);

        // create and register the command callbacks only (only execute this code once for all plugins)
        mSelectCallback             = new CommandSelectCallback(false);
        mUnselectCallback           = new CommandUnselectCallback(false);
        mClearSelectionCallback     = new CommandClearSelectionCallback(false);
        mAdjustNodeGroupCallback    = new CommandAdjustNodeGroupCallback(false);
        mAddNodeGroupCallback       = new CommandAddNodeGroupCallback(false);
        mRemoveNodeGroupCallback    = new CommandRemoveNodeGroupCallback(false);

        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustNodeGroup", mAdjustNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AddNodeGroup", mAddNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveNodeGroup", mRemoveNodeGroupCallback);

        // reinit the dialog
        ReInit();

        // connect the window activation signal to refresh if reactivated
        connect(mDock, &QDockWidget::visibilityChanged, this, &NodeGroupsPlugin::WindowReInit);

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
            mCurrentActor = nullptr;
            mNodeGroupWidget->SetActor(nullptr);
            mNodeGroupWidget->SetNodeGroup(nullptr);
            mNodeGroupManagementWidget->SetActor(nullptr);
            return;
        }

        // get the corresponding actor
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // only reinit the node groups window if actorinstance changed
        if (mCurrentActor != actor)
        {
            // set the new actor
            mCurrentActor = actor;

            // set the new actor on each widget
            mNodeGroupWidget->SetActor(mCurrentActor);
            mNodeGroupManagementWidget->SetActor(mCurrentActor);
        }

        // set the dialog stack as main widget
        mDock->setWidget(mDialogStack);

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
        mNodeGroupManagementWidget->UpdateInterface();
        mNodeGroupWidget->UpdateInterface();
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
