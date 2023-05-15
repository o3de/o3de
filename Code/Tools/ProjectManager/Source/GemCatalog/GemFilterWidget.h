/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <QMap>
#include <QPushButton>
#endif

QT_FORWARD_DECLARE_CLASS(QButtonGroup)

namespace O3DE::ProjectManager
{
    class FilterCategoryWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit FilterCategoryWidget(const QString& header,
            bool showAllLessButton = false,
            bool collapsed = false,
            int defaultShowCount = 4,
            QWidget* parent = nullptr);

        QButtonGroup* GetButtonGroup();
        void SetElements(const QMap<QString, int>& elementNamesAndCounts);
        void SetElements(const QVector<QString>& elementNames, const QVector<int>& elementCounts);

    public slots:
        void UpdateCollapseState();

    private:
        QWidgetList GetElements() const;
        void SetElement(int index, const QString& name, int count);
        int RemoveUnusedElements(uint32_t usedCount);
        void UpdateSeeMoreLess();

        inline constexpr static int s_collapseButtonSize = 16;
        int m_defaultShowCount = 0;
        bool m_seeAll = false;

        QButtonGroup* m_buttonGroup = nullptr;
        QPushButton* m_collapseButton = nullptr;
        QWidget* m_elementsWidget = nullptr;
        QWidget* m_mainWidget = nullptr;
        LinkLabel* m_seeAllLessLabel = nullptr;
    };

    class OrFilterCategoryWidget
        : public FilterCategoryWidget
    {
        Q_OBJECT

    public:
        explicit OrFilterCategoryWidget(const QString& header, int numFilterElements, GemModel* gemModel);

        void UpdateFilter(
            bool(*filterMatch)(const GemInfo& gemInfo, int filterIndex), 
            QString(*filterLabel)(int filterIndex)
            );

    signals:
        void FilterToggled(int flag, bool checked);

    private:
        int m_numFilterElements = 0;
        GemModel* m_gemModel = nullptr;
    };

    class GemFilterWidget
        : public QScrollArea
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemFilterWidget(GemSortFilterProxyModel* filterProxyModel, QWidget* parent = nullptr);
        ~GemFilterWidget() = default;

        void UpdateAllFilters(bool resetCheckBoxes = true);

    private slots:
        void OnStatusFilterToggled(QAbstractButton* button, bool checked);
        void OnFeatureFilterToggled(QAbstractButton* button, bool checked);
        void OnUpdateFilterToggled(QAbstractButton* button, bool checked);
        void OnFilterProxyInvalidated();

    private:
        void UpdateGemStatusFilter();
        void UpdateVersionsFilter();
        void UpdateGemOriginFilter();
        void UpdateTypeFilter();
        void UpdatePlatformFilter();
        void UpdateFeatureFilter();

        GemModel* m_gemModel = nullptr;
        GemSortFilterProxyModel* m_filterProxyModel = nullptr;

        FilterCategoryWidget* m_statusFilter = nullptr;
        FilterCategoryWidget* m_versionsFilter = nullptr;
        OrFilterCategoryWidget* m_originFilter = nullptr;
        OrFilterCategoryWidget* m_typeFilter = nullptr;
        OrFilterCategoryWidget* m_platformFilter = nullptr;
        FilterCategoryWidget* m_featureFilter = nullptr;
    };
} // namespace O3DE::ProjectManager
