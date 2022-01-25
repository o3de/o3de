/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QTableWidget>

#include <QHeaderView>
#include <QStringList>
#include <QVector>
#include <QPair>
#endif

namespace O3DE::ProjectManager
{
    // Using a QTableWidget for its header
    // Using a seperate model allows the setup of a header exactly as needed
    class AdjustableHeaderWidget
        : public QTableWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit AdjustableHeaderWidget(const QStringList& headerLabels,
            const QVector<int>& defaultHeaderWidths, int minHeaderWidth,
            const QVector<QHeaderView::ResizeMode>& resizeModes,
            QWidget* parent = nullptr);
        ~AdjustableHeaderWidget() = default;

        QPair<int, int> CalcColumnXBounds(int headerIndex) const;

        inline constexpr static int s_headerTextIndent = 7;
        inline constexpr static int s_headerWidgetHeight = 24;

        QHeaderView* m_header;

    signals:
        void sectionsResized();

    protected slots:
        void OnSectionResized(int logicalIndex, int oldSize, int newSize);

    private:
        inline constexpr static int s_headerIndentSection = 11;
    };
} // namespace O3DE::ProjectManager
