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
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QHeaderView)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(GemRepoInspector)
    QT_FORWARD_DECLARE_CLASS(GemRepoListView)
    QT_FORWARD_DECLARE_CLASS(GemRepoModel)

    class GemRepoScreen
        : public ScreenWidget
    {
    public:
        explicit GemRepoScreen(QWidget* parent = nullptr);
        ~GemRepoScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        void Reinit();

        GemRepoModel* GetGemRepoModel() const { return m_gemRepoModel; }

    public slots:
        void HandleAddRepoButton();
        void HandleRemoveRepoButton(const QModelIndex& modelIndex);
        void HandleRefreshAllButton();
        void HandleRefreshRepoButton(const QModelIndex& modelIndex);

    private:
        void FillModel();
        QFrame* CreateNoReposContent();
        QFrame* CreateReposContent();

        QStackedWidget* m_contentStack = nullptr;
        QFrame* m_noRepoContent;
        QFrame* m_repoContent;

        QTableWidget* m_gemRepoHeaderTable = nullptr;
        QHeaderView* m_gemRepoListHeader = nullptr;
        GemRepoListView* m_gemRepoListView = nullptr;
        GemRepoInspector* m_gemRepoInspector = nullptr;
        GemRepoModel* m_gemRepoModel = nullptr;

        QLabel* m_lastAllUpdateLabel;
        QPushButton* m_AllUpdateButton;
    };
} // namespace O3DE::ProjectManager
