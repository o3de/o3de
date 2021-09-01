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
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <EMotionFX/CommandSystem/Source/NodeGroupCommands.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)


namespace EMStudio
{
    class NodeGroupWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeGroupWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        NodeGroupWidget(QWidget* parent = nullptr);
        ~NodeGroupWidget();

        void Init();
        void UpdateInterface();

        void SetActor(EMotionFX::Actor* actor);
        void SetNodeGroup(EMotionFX::NodeGroup* nodeGroup);
        void SetWidgetEnabled(bool enabled);

    public slots:
        void SelectNodesButtonPressed();
        void RemoveNodesButtonPressed();
        void NodeSelectionFinished(const AZStd::vector<SelectionItem>& selectionList);
        void OnItemSelectionChanged();

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

    private:
        EMotionFX::Actor*               m_actor;

        NodeSelectionWindow*            m_nodeSelectionWindow;
        CommandSystem::SelectionList    m_nodeSelectionList;
        EMotionFX::NodeGroup*           m_nodeGroup;
        CommandSystem::CommandAdjustNodeGroup::NodeAction m_nodeAction;

        // widgets
        QTableWidget*                   m_nodeTable;
        QPushButton*                    m_selectNodesButton;
        QPushButton*                    m_addNodesButton;
        QPushButton*                    m_removeNodesButton;
    };
} // namespace EMStudio
