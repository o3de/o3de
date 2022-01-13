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

#include <QStringList>
#include <QVector>
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
            const QVector<int>& defaultHeaderWidths, int minWidth, QWidget* parent = nullptr);
        ~AdjustableHeaderWidget() = default;

        QHeaderView* m_header;

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private:
        int m_minWidth;
    };
} // namespace O3DE::ProjectManager
