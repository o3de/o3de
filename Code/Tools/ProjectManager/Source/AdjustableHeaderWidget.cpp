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
#include <QScrollBar>
#include <QColor>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QPalette>

namespace O3DE::ProjectManager
{
    AdjustableHeaderWidget::AdjustableHeaderWidget(
        const QStringList& headerLabels, const QVector<int>& defaultHeaderWidths, int minWidth, QWidget* parent)
        : QTableWidget(parent)
        , m_minWidth(minWidth)
        , m_previousWidth(minWidth)
    {
        setObjectName("adjustableHeaderWidget");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        setFixedHeight(s_headerWidgetHeight);

        m_header = horizontalHeader();
        m_header->setDefaultAlignment(Qt::AlignLeft);

        // Insert columns so the header labels will show up along with the additional first indent column
        setColumnCount(headerLabels.count() + 1);
        // Set header labels along with empty indent column
        setHorizontalHeaderLabels(QStringList{ "" } + headerLabels);

        // Scale the first header section used for indenting
        m_header->resizeSection(0, s_headerIndentSection);

        // Resize the other headers to their defaults now that the are created
        for (int column = 0; column < headerLabels.count() && column < defaultHeaderWidths.count(); ++column)
        {
            m_header->resizeSection(column + 1, defaultHeaderWidths[column]);
        }

        m_header->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
        m_header->setStretchLastSection(true);
    }

    int AdjustableHeaderWidget::GetScrollPosition() const
    {
        const QScrollBar* scrollBar = this->horizontalScrollBar();

        const int scrollWidth = width();
        const int absoluteWidth = m_header->length();
        const int scrollMin = scrollBar->minimum();
        const float scrollRange = static_cast<float>(scrollBar->maximum()) - static_cast<float>(scrollMin);
        const int scrollValue = scrollBar->sliderPosition();

        // Caculuate the abolute width in pixels that is scrolled
        if (scrollValue == scrollMin || scrollWidth >= absoluteWidth)
        {
            return 0;
        }
        else
        {
            return static_cast<int>((scrollValue - scrollMin) / scrollRange * (absoluteWidth - scrollWidth));
        }
    }

    int AdjustableHeaderWidget::CalcHeaderXPos(int headerIndex, bool calcEnd) const
    {
        // Total the widths of all headers before this one or including it if calcEnd is true
        // Also factors in scroll position of the header
        int xPos = -GetScrollPosition();

        if (!calcEnd)
        {
            // Don't include this header in the x pos calculations, find the start of the header
            --headerIndex;
        }

        for (; headerIndex >= 0; --headerIndex)
        {
            xPos += m_header->sectionSize(headerIndex);
        }

        return xPos;
    }

    void AdjustableHeaderWidget::resizeEvent(QResizeEvent* event)
    {
        QTableWidget::resizeEvent(event);
        // Handles header scaling while the headers are shown
        ScaleHeaders();
    }

    void AdjustableHeaderWidget::showEvent(QShowEvent* event)
    {
        QTableWidget::showEvent(event);
        // Handles header scaling incase the window was resized while this was not displayed
        ScaleHeaders();
    }

    void AdjustableHeaderWidget::ScaleHeaders()
    {
        const int currentWidth = width();
        const float newRatio = static_cast<float>(currentWidth) / static_cast<float>(m_previousWidth);

        if (newRatio != 1.0f)
        {
            int columnCount = m_header->model()->columnCount(QModelIndex());

            // Resize all columns except the first because it is a spacer and last because it is set to stretch and fill the remaining space
            for (int columnIndex = 1; columnIndex < columnCount - 1; ++columnIndex)
            {
                m_header->resizeSection(columnIndex, static_cast<int>(m_header->sectionSize(columnIndex) * newRatio));
            }

            m_previousWidth = currentWidth;
        }
    }

} // namespace O3DE::ProjectManager
