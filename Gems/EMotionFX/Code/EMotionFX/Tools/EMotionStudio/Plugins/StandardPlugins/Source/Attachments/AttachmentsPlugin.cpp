/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_dialogStack                        = nullptr;
        m_selectCallback                     = nullptr;
        m_unselectCallback                   = nullptr;
        m_clearSelectionCallback             = nullptr;
        m_addAttachmentCallback              = nullptr;
        m_addDeformableAttachmentCallback    = nullptr;
        m_removeAttachmentCallback           = nullptr;
        m_clearAttachmentsCallback           = nullptr;
        m_adjustActorCallback                = nullptr;
        m_attachmentNodesWindow              = nullptr;
    }


    // destructor
    AttachmentsPlugin::~AttachmentsPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(m_selectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_unselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_addAttachmentCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_addDeformableAttachmentCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeAttachmentCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearAttachmentsCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_adjustActorCallback, false);

        delete m_selectCallback;
        delete m_unselectCallback;
        delete m_clearSelectionCallback;
        delete m_addAttachmentCallback;
        delete m_addDeformableAttachmentCallback;
        delete m_removeAttachmentCallback;
        delete m_clearAttachmentsCallback;
        delete m_adjustActorCallback;
    }

    // init after the parent dock window has been created
    bool AttachmentsPlugin::Init()
    {
        //LogInfo("Initializing attachments window.");

        // create the dialog stack
        assert(m_dialogStack == nullptr);
        m_dialogStack = new MysticQt::DialogStack(m_dock);
        m_dock->setWidget(m_dialogStack);

        // create the attachments window
        m_attachmentsWindow = new AttachmentsWindow(m_dialogStack);
        m_attachmentsWindow->Init();
        m_dialogStack->Add(m_attachmentsWindow, "Selected Actor Instance", false, true, true, false);

        // create the attachment hierarchy window
        m_attachmentsHierarchyWindow = new AttachmentsHierarchyWindow(m_dialogStack);
        m_attachmentsHierarchyWindow->Init();
        m_dialogStack->Add(m_attachmentsHierarchyWindow, "Hierarchy", false, true, true, false);

        // create the attachment nodes window
        m_attachmentNodesWindow = new AttachmentNodesWindow(m_dialogStack);
        m_dialogStack->Add(m_attachmentNodesWindow, "Attachment Nodes", false, true);

        // create and register the command callbacks only (only execute this code once for all plugins)
        m_selectCallback                     = new CommandSelectCallback(false);
        m_unselectCallback                   = new CommandUnselectCallback(false);
        m_clearSelectionCallback             = new CommandClearSelectionCallback(false);

        m_addAttachmentCallback              = new CommandAddAttachmentCallback(false);
        m_addDeformableAttachmentCallback    = new CommandAddDeformableAttachmentCallback(false);
        m_removeAttachmentCallback           = new CommandRemoveAttachmentCallback(false);
        m_clearAttachmentsCallback           = new CommandClearAttachmentsCallback(false);
        m_adjustActorCallback                = new CommandAdjustActorCallback(false);
        m_removeActorInstanceCallback        = new CommandRemoveActorInstanceCallback(false);

        GetCommandManager()->RegisterCommandCallback("Select", m_selectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", m_unselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_clearSelectionCallback);

        GetCommandManager()->RegisterCommandCallback("AddAttachment", m_addAttachmentCallback);
        GetCommandManager()->RegisterCommandCallback("AddDeformableAttachment", m_addDeformableAttachmentCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveAttachment", m_removeAttachmentCallback);
        GetCommandManager()->RegisterCommandCallback("ClearAttachments", m_clearAttachmentsCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActor", m_adjustActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", m_removeActorInstanceCallback);

        // reinit the dialog
        ReInit();

        // connect the window activation signal to refresh if reactivated
        connect(m_dock, &QDockWidget::visibilityChanged, this, &AttachmentsPlugin::WindowReInit);

        return true;
    }


    // function to reinit the window
    void AttachmentsPlugin::ReInit()
    {
        m_attachmentsWindow->ReInit();
        m_attachmentsHierarchyWindow->ReInit();

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
        m_attachmentNodesWindow->SetActor(actor);
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
