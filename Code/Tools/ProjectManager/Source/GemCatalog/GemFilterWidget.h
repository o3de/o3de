/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <LinkWidget.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QVector>
#include <QPushButton>
#endif

QT_FORWARD_DECLARE_CLASS(QButtonGroup)

namespace O3DE::ProjectManager
{
    class FilterCategoryWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit FilterCategoryWidget(const QString& header,
            const QVector<QString>& elementNames,
            const QVector<int>& elementCounts,
            bool showAllLessButton = true,
            bool collapsed = false,
            int defaultShowCount = 4,
            QWidget* parent = nullptr);

        QButtonGroup* GetButtonGroup();

        bool IsCollapsed();

    private:
        void UpdateCollapseState();
        void UpdateSeeMoreLess();

        inline constexpr static int s_collapseButtonSize = 16;
        QPushButton* m_collapseButton = nullptr;

        QWidget* m_mainWidget = nullptr;
        QButtonGroup* m_buttonGroup = nullptr;
        QVector<QWidget*> m_elementWidgets; //! Includes checkbox and the count labl.
        LinkLabel* m_seeAllLessLabel = nullptr;
        int m_defaultShowCount = 0;
        bool m_seeAll = false;
    };

    class GemFilterWidget
        : public QScrollArea
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemFilterWidget(GemSortFilterProxyModel* filterProxyModel, QWidget* parent = nullptr);
        ~GemFilterWidget() = default;

    public slots:
        void ResetGemStatusFilter();

    private:
        void AddGemOriginFilter();
        void AddTypeFilter();
        void AddPlatformFilter();
        void AddFeatureFilter();

        QVBoxLayout* m_filterLayout = nullptr;
        GemModel* m_gemModel = nullptr;
        GemSortFilterProxyModel* m_filterProxyModel = nullptr;
        FilterCategoryWidget* m_statusFilter = nullptr;
    };
} // namespace O3DE::ProjectManager
