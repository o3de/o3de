/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AttachmentsPlugin.h"
#include <MCore/Source/LogManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QLabel>
#include <QVBoxLayout>


namespace EMStudio
{
    // constructor
    AttachmentsPlugin::AttachmentsPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mDialogStack                        = nullptr;
        mSelectCallback                     = nullptr;
        mUnselectCallback                   = nullptr;
        mClearSelectionCallback             = nullptr;
        mAddAttachmentCallback              = nullptr;
        mAddDeformableAttachmentCallback    = nullptr;
        mRemoveAttachmentCallback           = nullptr;
        mClearAttachmentsCallback           = nullptr;
        mAdjustActorCallback                = nullptr;
        mAttachmentNodesWindow              = nullptr;
    }


    // destructor
    AttachmentsPlugin::~AttachmentsPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAddAttachmentCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAddDeformableAttachmentCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveAttachmentCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearAttachmentsCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustActorCallback, false);

        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mAddAttachmentCallback;
        delete mAddDeformableAttachmentCallback;
        delete mRemoveAttachmentCallback;
        delete mClearAttachmentsCallback;
        delete mAdjustActorCallback;
    }


    // clone the log window
    EMStudioPlugin* AttachmentsPlugin::Clone()
    {
        return new AttachmentsPlugin();
    }


    // init after the parent dock window has been created
    bool AttachmentsPlugin::Init()
    {
        //LogInfo("Initializing attachments window.");

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack(mDock);
        mDock->setWidget(mDialogStack);

        // create the attachments window
        mAttachmentsWindow = new AttachmentsWindow(mDialogStack);
        mAttachmentsWindow->Init();
        mDialogStack->Add(mAttachmentsWindow, "Selected Actor Instance", false, true, true, false);

        // create the attachment hierarchy window
        mAttachmentsHierarchyWindow = new AttachmentsHierarchyWindow(mDialogStack);
        mAttachmentsHierarchyWindow->Init();
        mDialogStack->Add(mAttachmentsHierarchyWindow, "Hierarchy", false, true, true, false);

        // create the attachment nodes window
        mAttachmentNodesWindow = new AttachmentNodesWindow(mDialogStack);
        mDialogStack->Add(mAttachmentNodesWindow, "Attachment Nodes", false, true);

        // create and register the command callbacks only (only execute this code once for all plugins)
        mSelectCallback                     = new CommandSelectCallback(false);
        mUnselectCallback                   = new CommandUnselectCallback(false);
        mClearSelectionCallback             = new CommandClearSelectionCallback(false);

        mAddAttachmentCallback              = new CommandAddAttachmentCallback(false);
        mAddDeformableAttachmentCallback    = new CommandAddDeformableAttachmentCallback(false);
        mRemoveAttachmentCallback           = new CommandRemoveAttachmentCallback(false);
        mClearAttachmentsCallback           = new CommandClearAttachmentsCallback(false);
        mAdjustActorCallback                = new CommandAdjustActorCallback(false);
        mRemoveActorInstanceCallback        = new CommandRemoveActorInstanceCallback(false);

        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);

        GetCommandManager()->RegisterCommandCallback("AddAttachment", mAddAttachmentCallback);
        GetCommandManager()->RegisterCommandCallback("AddDeformableAttachment", mAddDeformableAttachmentCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveAttachment", mRemoveAttachmentCallback);
        GetCommandManager()->RegisterCommandCallback("ClearAttachments", mClearAttachmentsCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActor", mAdjustActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", mRemoveActorInstanceCallback);

        // reinit the dialog
        ReInit();

        // connect the window activation signal to refresh if reactivated
        connect(mDock, &QDockWidget::visibilityChanged, this, &AttachmentsPlugin::WindowReInit);

        return true;
    }


    // function to reinit the window
    void AttachmentsPlugin::ReInit()
    {
        mAttachmentsWindow->ReInit();
        mAttachmentsHierarchyWindow->ReInit();

        // get the current actor
        const CommandSystem::SelectionList& selection       = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance*           actorInstance   = selection.GetSingleActorInstance();
        EMotionFX::Actor*                   actor           = nullptr;

        // disable controls if no actor instance is selected
        if (actorInstance)
        {
            actor = actorInstance->GetActor();
        }

        // set the actor of the attachment nodes window
        mAttachmentNodesWindow->SetActor(actor);
    }


    // reinit the window when it gets activated
    void AttachmentsPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit();
        }
    }


    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------
    bool ReInitAttachmentsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AttachmentsPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        AttachmentsPlugin* attachmentsPlugin = (AttachmentsPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (attachmentsPlugin->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            attachmentsPlugin->ReInit();
        }

        return true;
    }

    bool AttachmentSelectedAttachmentsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AttachmentsPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        AttachmentsPlugin* attachmentsPlugin = (AttachmentsPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (attachmentsPlugin->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            AttachmentsWindow* attachmentsWindow = attachmentsPlugin->GetAttachmentsWindow();
            if (attachmentsWindow->GetIsWaitingForAttachment())
            {
                attachmentsWindow->OnAttachmentSelected();
            }
            else
            {
                attachmentsPlugin->ReInit();
            }
        }

        return true;
    }

    bool AttachmentsPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return AttachmentSelectedAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return AttachmentSelectedAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return AttachmentSelectedAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return AttachmentSelectedAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return AttachmentSelectedAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return AttachmentSelectedAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandAddAttachmentCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandAddAttachmentCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandAddDeformableAttachmentCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandAddDeformableAttachmentCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandRemoveAttachmentCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandRemoveAttachmentCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandClearAttachmentsCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandClearAttachmentsCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandRemoveActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandRemoveActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitAttachmentsPlugin(); }
    bool AttachmentsPlugin::CommandAdjustActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitAttachmentsPlugin();
    }
    bool AttachmentsPlugin::CommandAdjustActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitAttachmentsPlugin();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Attachments/moc_AttachmentsPlugin.cpp>
