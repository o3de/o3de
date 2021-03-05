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
