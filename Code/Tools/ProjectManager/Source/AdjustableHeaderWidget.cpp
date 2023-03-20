/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AdjustableHeaderWidget.h>
#include <AzCore/Debug/Trace.h>

#include <QHeaderView>
#include <QTimer>

namespace O3DE::ProjectManager
{
    AdjustableHeaderWidget::AdjustableHeaderWidget( const QStringList& headerLabels,
        const QVector<int>& defaultHeaderWidths, int minHeaderWidth,
        const QVector<QHeaderView::ResizeMode>& resizeModes, QWidget* parent)
        : QTableWidget(parent)
    {
        setObjectName("adjustableHeaderWidget");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        setFixedHeight(s_headerWidgetHeight);

        m_header = horizontalHeader();
        m_header->setDefaultAlignment(Qt::AlignLeft);

        setColumnCount(headerLabels.count());
        setHorizontalHeaderLabels(headerLabels);

        AZ_Assert(defaultHeaderWidths.count() == columnCount(), "Default header widths does not match number of columns");
        AZ_Assert(resizeModes.count() == columnCount(), "Resize modesdoes not match number of columns");

        for (int column = 0; column < columnCount(); ++column)
        {
            m_header->resizeSection(column, defaultHeaderWidths[column]);
            m_header->setSectionResizeMode(column, resizeModes[column]);
        }

        m_header->setMinimumSectionSize(minHeaderWidth);
        m_header->setCascadingSectionResizes(true);

        connect(m_header, &QHeaderView::sectionResized, this, &AdjustableHeaderWidget::OnSectionResized);
    }

    void AdjustableHeaderWidget::OnSectionResized(int logicalIndex, int oldSize, int newSize)
    {
        const int headerCount = columnCount(); 
        const int headerWidth = m_header->width();
        const int totalSectionWidth = m_header->length();

        if (totalSectionWidth > headerWidth && newSize > oldSize)
        {
            int xPos = 0;
            int requiredWidth = 0;

            for (int i = 0; i < headerCount; i++)
            {
                if (i < logicalIndex)
                {
                    xPos += m_header->sectionSize(i);
                }
                else if (i == logicalIndex)
                {
                    xPos += newSize;
                }
                else if (i > logicalIndex)
                {
                    if (m_header->sectionResizeMode(i) == QHeaderView::ResizeMode::Fixed)
                    {
                        requiredWidth += m_header->sectionSize(i);
                    }
                    else
                    {
                        requiredWidth += m_header->minimumSectionSize();
                    }
                }
            }

            if (xPos + requiredWidth > headerWidth)
            {
                m_header->resizeSection(logicalIndex, oldSize);
            }
        }

        // wait till all columns resized
        QTimer::singleShot(0, [&]()
        {
            // only re-paint when the header and section widths have settled
            const int headerWidth = m_header->width();
            const int totalSectionWidth = m_header->length();
            if (totalSectionWidth == headerWidth)
            {
                emit sectionsResized();
            }
        });
    }

    QPair<int, int> AdjustableHeaderWidget::CalcColumnXBounds(int headerIndex) const
    {
        // Total the widths of all headers before this one in first and including it in second
        QPair<int, int> bounds(0, 0);

        for (int curIndex = 0; curIndex <= headerIndex; ++curIndex)
        {
            if (curIndex == headerIndex)
            {
                bounds.first = bounds.second;
            }
            bounds.second += m_header->sectionSize(curIndex);
        }

        return bounds;
    }


} // namespace O3DE::ProjectManager
