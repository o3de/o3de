/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
