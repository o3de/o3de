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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <GemCatalog/GemFilterWidget.h>
#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <QSet>
#include <QString>
#endif

namespace O3DE::ProjectManager
{
    class GemCatalogScreen
        : public ScreenWidget
    {
    public:
        explicit GemCatalogScreen(QWidget* parent = nullptr);
        ~GemCatalogScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        void ReinitForProject(const QString& projectPath);

        enum class EnableDisableGemsResult 
        {
            Failed = 0,
            Success,
            Cancel
        };
        EnableDisableGemsResult EnableDisableGemsForProject(const QString& projectPath);

        GemModel* GetGemModel() const { return m_gemModel; }
        DownloadController* GetDownloadController() const { return m_downloadController; }

    public slots:
        void OnGemStatusChanged(const QModelIndex& modelIndex, uint32_t numChangedDependencies);
        void OnAddGemClicked();

    protected:
        void hideEvent(QHideEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void moveEvent(QMoveEvent* event) override;

    private slots:
        void HandleOpenGemRepo();


    private:
        void FillModel(const QString& projectPath);

        AZStd::unique_ptr<AzToolsFramework::ToastNotificationsView> m_notificationsView;

        GemListView* m_gemListView = nullptr;
        GemInspector* m_gemInspector = nullptr;
        GemModel* m_gemModel = nullptr;
        GemCatalogHeaderWidget* m_headerWidget = nullptr;
        GemSortFilterProxyModel* m_proxModel = nullptr;
        QVBoxLayout* m_filterWidgetLayout = nullptr;
        GemFilterWidget* m_filterWidget = nullptr;
        DownloadController* m_downloadController = nullptr;
        bool m_notificationsEnabled = true;
        QSet<QString> m_gemsToRegisterWithProject;
    };
} // namespace O3DE::ProjectManager
