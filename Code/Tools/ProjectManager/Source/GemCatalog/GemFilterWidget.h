/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            int defaultShowCount = 4,
            QWidget* parent = nullptr);

        QButtonGroup* GetButtonGroup();

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

    private:
        void AddGemOriginFilter();
        void AddTypeFilter();
        void AddPlatformFilter();
        void AddFeatureFilter();

        QVBoxLayout* m_mainLayout = nullptr;
        GemModel* m_gemModel = nullptr;
        GemSortFilterProxyModel* m_filterProxyModel = nullptr;
    };
} // namespace O3DE::ProjectManager
