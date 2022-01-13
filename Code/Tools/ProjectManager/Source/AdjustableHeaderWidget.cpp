/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AdjustableHeaderWidget.h>

#include <QHeaderView>
#include <QTimer>
#include <QResizeEvent>

namespace O3DE::ProjectManager
{
    AdjustableHeaderWidget::AdjustableHeaderWidget(
        const QStringList& headerLabels, const QVector<int>& defaultHeaderWidths, int minWidth, QWidget* parent)
        : QTableWidget(parent)
        , m_minWidth(minWidth)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        setFixedHeight(20);

        m_header = horizontalHeader();

        m_header->setDefaultAlignment(Qt::AlignLeft);

        // Insert columns so the header labels will show up
        for (int column = 0; column < headerLabels.count(); ++column)
        {
            insertColumn(column);
        }
        setHorizontalHeaderLabels(headerLabels);

        const int headerExtraMargin = 18;
        for (int column = 0; column < headerLabels.count() && column < defaultHeaderWidths.count(); ++column)
        {
            m_header->resizeSection(column, defaultHeaderWidths[column] + (column == 0 ? headerExtraMargin : 0));
        }

        m_header->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
        m_header->setCascadingSectionResizes(false);
        m_header->setStretchLastSection(true);

        // Required to set stylesheet in code as it will not be respected if set in qss
        m_header->setStyleSheet("QHeaderView::section { background-color:#333333; color:white; font-size:12px; }");
    }

    void AdjustableHeaderWidget::resizeEvent(QResizeEvent* event)
    {
        float newRatio = static_cast<float>(event->size().width()) / static_cast<float>(event->oldSize().width());

        // This is the intial scaling after window creation because window starts everything starts at size -1
        if (newRatio < 0.0f)
        {
            // Scale from the default min size set in contructor to the ratio that the window scaling allows
            newRatio = static_cast<float>(event->size().width()) / static_cast<float>(m_minWidth);
        }

        int columnCount = m_header->model()->columnCount(QModelIndex());

        // Resize all columns except the last because it is set to stretch and fill the remaining space
        for (int columnIndex = 0; columnIndex < columnCount - 1; ++columnIndex)
        {
            m_header->resizeSection(columnIndex, static_cast<int>(m_header->sectionSize(columnIndex) * newRatio));
        }
    }
} // namespace O3DE::ProjectManager
