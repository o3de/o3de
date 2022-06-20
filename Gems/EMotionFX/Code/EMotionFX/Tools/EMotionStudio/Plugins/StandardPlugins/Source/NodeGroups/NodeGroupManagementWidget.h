/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        EMotionFX::Actor*   m_actor;
        AZStd::string       m_nodeGroupName;
        QLineEdit*          m_lineEdit;
        QPushButton*        m_okButton;
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
        NodeGroupWidget*    m_nodeGroupWidget;

        // searches for the given text in the table
        int SearchTableForString(QTableWidget* tableWidget, const QString& text, bool ignoreCurrentSelection = false);

        // the actor
        EMotionFX::Actor*   m_actor;

        // the listbox
        QTableWidget*       m_nodeGroupsTable;
        uint32              m_selectedRow;

        // the buttons
        QPushButton*        m_addButton;
        QPushButton*        m_removeButton;
        QPushButton*        m_clearButton;
    };
} // namespace EMStudio
