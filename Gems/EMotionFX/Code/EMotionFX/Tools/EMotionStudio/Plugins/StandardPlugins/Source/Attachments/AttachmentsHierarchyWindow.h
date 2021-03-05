/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
