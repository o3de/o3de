/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include "AttachmentsWindow.h"
#include "AttachmentsHierarchyWindow.h"
#include "AttachmentNodesWindow.h"
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMStudio
{
    class AttachmentsPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(AttachmentsPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000017
        };

        AttachmentsPlugin();
        ~AttachmentsPlugin();

        // overloaded
        const char* GetName() const override                { return "Attachments"; }
        uint32 GetClassID() const override                  { return AttachmentsPlugin::CLASS_ID; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new AttachmentsPlugin(); }

        void ReInit();
        AttachmentsWindow* GetAttachmentsWindow() const     { return m_attachmentsWindow; }

    public slots:
        void WindowReInit(bool visible);

    private:
        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAddAttachmentCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAddDeformableAttachmentCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAttachmentCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearAttachmentsCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorInstanceCallback);

        CommandSelectCallback*                      m_selectCallback;
        CommandUnselectCallback*                    m_unselectCallback;
        CommandClearSelectionCallback*              m_clearSelectionCallback;
        CommandAddAttachmentCallback*               m_addAttachmentCallback;
        CommandAddDeformableAttachmentCallback*     m_addDeformableAttachmentCallback;
        CommandRemoveAttachmentCallback*            m_removeAttachmentCallback;
        CommandClearAttachmentsCallback*            m_clearAttachmentsCallback;
        CommandAdjustActorCallback*                 m_adjustActorCallback;
        CommandRemoveActorInstanceCallback*         m_removeActorInstanceCallback;

        MysticQt::DialogStack*                      m_dialogStack;
        AttachmentsWindow*                          m_attachmentsWindow;
        AttachmentsHierarchyWindow*                 m_attachmentsHierarchyWindow;
        AttachmentNodesWindow*                      m_attachmentNodesWindow;
    };
} // namespace EMStudio
