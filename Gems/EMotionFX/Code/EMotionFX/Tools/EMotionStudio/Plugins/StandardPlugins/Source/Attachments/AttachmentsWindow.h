/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        bool GetIsWaitingForAttachment() const      { return mWaitingForAttachment; }

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
        void OnAttachmentNodesSelected(MCore::Array<SelectionItem> selection);
        void OnCancelAttachmentNodeSelection();
        void OnEscapeButtonPressed();
        void OnUpdateButtonsEnabled();

    private:
        void RemoveTableItems(const QList<QTableWidgetItem*>& items);
        int GetIDFromTableRow(int row);
        AZStd::string GetNodeNameFromTableRow(int row);
        int GetRowContainingWidget(const QWidget* widget);

        bool                                    mWaitingForAttachment;
        bool                                    mIsDeformableAttachment;

        QVBoxLayout*                            mWaitingForAttachmentLayout;
        QVBoxLayout*                            mNoSelectionLayout;
        QVBoxLayout*                            mMainLayout;
        QVBoxLayout*                            mAttachmentsLayout;

        QWidget*                                mAttachmentsWidget;
        QWidget*                                mWaitingForAttachmentWidget;
        QWidget*                                mNoSelectionWidget;

        QShortcut*                              mEscapeShortcut;

        QTableWidget*                           mTableWidget;
        EMotionFX::ActorInstance*               mActorInstance;
        AZStd::vector<AZStd::string>            mAttachments;
        AZStd::string                           mNodeBeforeSelectionWindow;

        QToolButton*                            mOpenAttachmentButton;
        QToolButton*                            mOpenDeformableAttachmentButton;
        QToolButton*                            mRemoveButton;
        QToolButton*                            mClearButton;
        QToolButton*                            mCancelSelectionButton;

        NodeSelectionWindow*                    mNodeSelectionWindow;

        AZStd::vector<AZStd::string>            mDropFileNames;
        AZStd::string                           mTempString;
    };

} // namespace EMStudio
