/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QTableView>
#endif

class QTimer;

namespace AzQtComponents
{
    class StyledDetailsTableModel;

    class AZ_QT_COMPONENTS_API StyledDetailsTableView
        : public QTableView
    {
        Q_OBJECT

    public:
        explicit StyledDetailsTableView(QWidget* parent = nullptr);

        void setModel(QAbstractItemModel* model) override;
        void ResetDelegate();

    protected:
        void paintEvent(QPaintEvent* ev) override;
        void keyPressEvent(QKeyEvent* ev) override;
        QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex&, const QEvent*) const override;

    private:
        void copySelectionToClipboard();
        void updateItemSelection(const QItemSelection& selection);

    private:
        bool m_scrollOnInsert = false;
        QTimer* m_resizeTimer;
    };
} // namespace AzQtComponents
