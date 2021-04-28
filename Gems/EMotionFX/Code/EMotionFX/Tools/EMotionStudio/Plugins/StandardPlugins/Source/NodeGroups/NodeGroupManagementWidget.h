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

#pragma once

#if !defined(Q_MOC_RUN)
#include "NodeGroupWidget.h"
#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <QTableWidget>
#include <QEvent>
#include <QKeyEvent>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)


namespace EMStudio
{
    class NodeGroupManagementRenameWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeGroupManagementRenameWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        NodeGroupManagementRenameWindow(QWidget* parent, EMotionFX::Actor* actor, const AZStd::string& nodeGroupName);

    private slots:
        void TextEdited(const QString& text);
        void Accepted();

    private:
        EMotionFX::Actor*   mActor;
        AZStd::string       mNodeGroupName;
        QLineEdit*          mLineEdit;
        QPushButton*        mOKButton;
    };


    class NodeGroupManagementWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeGroupManagementWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        // constructor
        NodeGroupManagementWidget(NodeGroupWidget* nodeGroupWidget, QWidget* parent = nullptr);

        // destructor
        ~NodeGroupManagementWidget();

        // init function
        void Init();

        // function to set the actor
        void SetActor(EMotionFX::Actor* actor);
        void SetNodeGroupWidget(NodeGroupWidget* nodeGroupWidget);

    public slots:
        // slots for adding, removing and clearing node groups
        void AddNodeGroup();
        void RemoveSelectedNodeGroup();
        void RenameSelectedNodeGroup();
        void ClearNodeGroups();

        // used to update the interface
        void UpdateInterface();
        void UpdateNodeGroupWidget();
        //void UpdateNodeGroupWidget(QTableWidgetItem* current, QTableWidgetItem* previous);
        //void NodeGroupeNameDoubleClicked(QTableWidgetItem* item);
        //void NodeGroupNamesChanged(const QString& text);
        //void NodeGroupNameEditingFinished();

        void checkboxClicked(bool checked);

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        // pointer to the nodegroup widget
        NodeGroupWidget*    mNodeGroupWidget;

        // searches for the given text in the table
        int SearchTableForString(QTableWidget* tableWidget, const QString& text, bool ignoreCurrentSelection = false);

        // the actor
        EMotionFX::Actor*   mActor;

        // the listbox
        QTableWidget*       mNodeGroupsTable;
        uint32              mSelectedRow;

        // the buttons
        QPushButton*        mAddButton;
        QPushButton*        mRemoveButton;
        QPushButton*        mClearButton;
    };
} // namespace EMStudio
