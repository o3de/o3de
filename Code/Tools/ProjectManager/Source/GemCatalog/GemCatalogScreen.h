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
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <GemCatalog/GemFilterWidget.h>
#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
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

    private:
        void FillModel(const QString& projectPath);

        GemListView* m_gemListView = nullptr;
        GemInspector* m_gemInspector = nullptr;
        GemModel* m_gemModel = nullptr;
        GemCatalogHeaderWidget* m_headerWidget = nullptr;
        GemSortFilterProxyModel* m_proxModel = nullptr;
        QVBoxLayout* m_filterWidgetLayout = nullptr;
        GemFilterWidget* m_filterWidget = nullptr;
    };
} // namespace O3DE::ProjectManager
