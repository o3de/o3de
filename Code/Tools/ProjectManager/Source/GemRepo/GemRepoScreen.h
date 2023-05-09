/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QHeaderView)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)
QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(GemRepoInspector)
    QT_FORWARD_DECLARE_CLASS(GemRepoListView)
    QT_FORWARD_DECLARE_CLASS(GemRepoModel)
    QT_FORWARD_DECLARE_CLASS(GemRepoProxyModel)
    QT_FORWARD_DECLARE_CLASS(AdjustableHeaderWidget)

    class GemRepoScreen
        : public ScreenWidget
    {
        Q_OBJECT
    public:
        explicit GemRepoScreen(QWidget* parent = nullptr);
        ~GemRepoScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        void Reinit();

        GemRepoModel* GetGemRepoModel() const { return m_gemRepoModel; }

        void NotifyCurrentScreen() override;

    public slots:
        void ShowStandardToastNotification(const QString& notification);
        void HandleAddRepoButton();
        void HandleRemoveRepoButton(const QModelIndex& modelIndex);
        void HandleRefreshAllButton();
        void HandleRefreshRepoButton(const QModelIndex& modelIndex);
        void OnModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

    private:
        void FillModel();

        AZStd::unique_ptr<AzToolsFramework::ToastNotificationsView> m_notificationsView;

        QFrame* CreateNoReposContent();
        QFrame* CreateReposContent();

        QStackedWidget* m_contentStack = nullptr;
        QFrame* m_noRepoContent;
        QFrame* m_repoContent;

        AdjustableHeaderWidget* m_gemRepoHeaderTable = nullptr;
        QHeaderView* m_gemRepoListHeader = nullptr;
        GemRepoListView* m_gemRepoListView = nullptr;
        GemRepoInspector* m_gemRepoInspector = nullptr;
        GemRepoModel* m_gemRepoModel = nullptr;
        GemRepoProxyModel* m_sortProxyModel = nullptr;
        QItemSelectionModel* m_selectionModel = nullptr;

        QLabel* m_lastAllUpdateLabel;
    };
} // namespace O3DE::ProjectManager
