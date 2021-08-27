/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>

#include "../StandardPluginsConfig.h"
#include <MCore/Source/MemoryCategoriesCore.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"

#include <QWidget>
#include <QTreeWidget>
#include <QMimeData>
#include <QUrl>
#include <QDropEvent>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QFileDialog)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)
QT_FORWARD_DECLARE_CLASS(QShortcut)


namespace EMStudio
{
    class AttachmentsWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(AttachmentsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        AttachmentsWindow(QWidget* parent, bool deformable = false);
        ~AttachmentsWindow();

        void Init();
        void ReInit();

        void UpdateInterface();

        void AddAttachment(const AZStd::string& filename);
        void AddAttachments(const AZStd::vector<AZStd::string>& filenames);

        void OnAttachmentSelected();
        bool GetIsWaitingForAttachment() const      { return m_waitingForAttachment; }

        EMotionFX::ActorInstance* GetSelectedAttachment();
        AZStd::string GetSelectedNodeName();

    protected:
        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

    private slots:
        void OnNodeChanged();
        void OnSelectionChanged();
        void OnOpenAttachmentButtonClicked();
        void OnOpenDeformableAttachmentButtonClicked();
        void OnRemoveButtonClicked();
        void OnClearButtonClicked();
        void OnSelectNodeButtonClicked();
        void OnDroppedAttachmentsActors();
        void OnDroppedDeformableActors();
        void OnVisibilityChanged(int visibility);
        void OnAttachmentNodesSelected(AZStd::vector<SelectionItem> selection);
        void OnCancelAttachmentNodeSelection();
        void OnEscapeButtonPressed();
        void OnUpdateButtonsEnabled();

    private:
        void RemoveTableItems(const QList<QTableWidgetItem*>& items);
        int GetIDFromTableRow(int row);
        AZStd::string GetNodeNameFromTableRow(int row);
        int GetRowContainingWidget(const QWidget* widget);

        bool                                    m_waitingForAttachment;
        bool                                    m_isDeformableAttachment;

        QVBoxLayout*                            m_waitingForAttachmentLayout;
        QVBoxLayout*                            m_noSelectionLayout;
        QVBoxLayout*                            m_mainLayout;
        QVBoxLayout*                            m_attachmentsLayout;

        QWidget*                                m_attachmentsWidget;
        QWidget*                                m_waitingForAttachmentWidget;
        QWidget*                                m_noSelectionWidget;

        QShortcut*                              m_escapeShortcut;

        QTableWidget*                           m_tableWidget;
        EMotionFX::ActorInstance*               m_actorInstance;
        AZStd::vector<AZStd::string>            m_attachments;
        AZStd::string                           m_nodeBeforeSelectionWindow;

        QToolButton*                            m_openAttachmentButton;
        QToolButton*                            m_openDeformableAttachmentButton;
        QToolButton*                            m_removeButton;
        QToolButton*                            m_clearButton;
        QToolButton*                            m_cancelSelectionButton;

        NodeSelectionWindow*                    m_nodeSelectionWindow;

        AZStd::vector<AZStd::string>            m_dropFileNames;
        AZStd::string                           m_tempString;
    };

} // namespace EMStudio
