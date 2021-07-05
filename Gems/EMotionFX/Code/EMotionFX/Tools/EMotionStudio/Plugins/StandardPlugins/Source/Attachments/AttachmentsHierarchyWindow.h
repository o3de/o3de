/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_ATTACHMENTSHIERARCHYWINDOW_H
#define __EMSTUDIO_ATTACHMENTSHIERARCHYWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include <MCore/Source/MemoryCategoriesCore.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <QWidget>
#include <QTreeWidget>
#endif


namespace EMStudio
{
    class AttachmentsHierarchyWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(AttachmentsHierarchyWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:

        // BIG TODO: ONLY UPDATE THIS DIALOG WHEN:
        // calling add/remove attachments, NOT when selecting stuff !!!!!!!!!!!!!!!!!!!
        AttachmentsHierarchyWindow(QWidget* parent);
        ~AttachmentsHierarchyWindow();

        void Init();
        void ReInit();

    private:
        void RecursivelyAddAttachments(QTreeWidgetItem* parent, EMotionFX::ActorInstance* actorInstance);
        QTreeWidget* mHierarchy;
    };
} // namespace EMStudio


#endif
