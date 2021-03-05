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

#ifndef __EMSTUDIO_ATTACHMENTNODESWINDOW_H
#define __EMSTUDIO_ATTACHMENTNODESWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#endif

// qt forward declarations
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)


namespace EMStudio
{
    /**
     * window for the adjustment of attachment nodes of the selected actor
     */
    class AttachmentNodesWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(AttachmentNodesWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        // constructor
        AttachmentNodesWindow(QWidget* parent = nullptr);

        // destructor
        ~AttachmentNodesWindow();

        // init function
        void Init();

        // used to update the interface
        void UpdateInterface();

        // function to set the actor
        void SetActor(EMotionFX::Actor* actor);
        void SetWidgetDisabled(bool disabled);

    public slots:
        // the slots
        void SelectNodesButtonPressed();
        void RemoveNodesButtonPressed();
        void NodeSelectionFinished(MCore::Array<SelectionItem> selectionList);
        void OnItemSelectionChanged();

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

    private:
        // the current actor
        EMotionFX::Actor*           mActor;

        // the node selection window and node group
        NodeSelectionWindow*        mNodeSelectionWindow;
        CommandSystem::SelectionList    mNodeSelectionList;
        AZStd::string               mNodeAction;

        // widgets
        QTableWidget*               mNodeTable;
        QToolButton*                mSelectNodesButton;
        QToolButton*                mAddNodesButton;
        QToolButton*                mRemoveNodesButton;
    };
} // namespace EMStudio


#endif
