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
#include <ScreensCtrl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>
#include <GemCatalog/GemInfo.h>
#include <QSet>
#include <QString>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(GemCatalogHeaderWidget)
    QT_FORWARD_DECLARE_CLASS(GemFilterWidget)
    QT_FORWARD_DECLARE_CLASS(GemListView)
    QT_FORWARD_DECLARE_CLASS(GemInspector)
    QT_FORWARD_DECLARE_CLASS(GemModel)
    QT_FORWARD_DECLARE_CLASS(GemSortFilterProxyModel)
    QT_FORWARD_DECLARE_CLASS(DownloadController)

    class GemCatalogScreen
        : public ScreenWidget
    {
    public:
        explicit GemCatalogScreen(DownloadController* downloadController, bool readOnly = false, QWidget* parent = nullptr);
        ~GemCatalogScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        void ReinitForProject(const QString& projectPath);

        QString GetTabText() override;
        bool IsTab() override;
        void NotifyCurrentScreen() override;

        void AddToGemModel(const GemInfo& gemInfo);

        void ShowStandardToastNotification(const QString& notification);

        GemModel* GetGemModel() const { return m_gemModel; }
        DownloadController* GetDownloadController() const { return m_downloadController; }

    public slots:
        void OnGemStatusChanged(const QString& gemName, uint32_t numChangedDependencies);
        void OnDependencyGemStatusChanged(const QString& gemName);
        void OnAddGemClicked();
        void SelectGem(const QString& gemName);
        void OnGemDownloadResult(const QString& gemName, bool succeeded = true);
        void Refresh();
        void UpdateGem(const QModelIndex& modelIndex);
        void UninstallGem(const QModelIndex& modelIndex);
        void HandleGemCreated(const GemInfo& gemInfo);
        void HandleGemEdited(const GemInfo& newGemInfo);

    protected:
        void hideEvent(QHideEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void moveEvent(QMoveEvent* event) override;

        GemModel* m_gemModel = nullptr;
        QSet<QString> m_gemsToRegisterWithProject;

    private slots:
        void HandleOpenGemRepo();
        void HandleCreateGem();
        void HandleEditGem(const QModelIndex& currentModelIndex);
        void UpdateAndShowGemCart(QWidget* cartWidget);
        void ShowInspector();

    private:
        enum RightPanelWidgetOrder
        {
            Inspector = 0,
            Cart
        };

        void FillModel(const QString& projectPath);

        AZStd::unique_ptr<AzToolsFramework::ToastNotificationsView> m_notificationsView;
        GemListView* m_gemListView = nullptr;
        QStackedWidget* m_rightPanelStack = nullptr;
        GemInspector* m_gemInspector = nullptr;
        GemCatalogHeaderWidget* m_headerWidget = nullptr;
        GemSortFilterProxyModel* m_proxyModel = nullptr;
        QVBoxLayout* m_filterWidgetLayout = nullptr;
        GemFilterWidget* m_filterWidget = nullptr;
        DownloadController* m_downloadController = nullptr;
        ScreensCtrl* m_screensControl = nullptr;
        bool m_notificationsEnabled = true;
        QString m_projectPath;
        bool m_readOnly;

        QModelIndex m_curEditedIndex;

    };
} // namespace O3DE::ProjectManager
