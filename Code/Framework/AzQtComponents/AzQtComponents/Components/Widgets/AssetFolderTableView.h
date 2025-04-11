/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>

AZ_PUSH_DISABLE_WARNING(4244, "-Wunknown-warning-option")
#include <AzQtComponents/Components/Widgets/TableView.h>
AZ_POP_DISABLE_WARNING
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API AssetFolderTableView
        : public TableView
    {
        Q_OBJECT

    public:
        explicit AssetFolderTableView(QWidget* parent = nullptr);

        void SetShowSearchResultsMode(bool searchMode);
        void setRootIndex(const QModelIndex& index) override;

    protected Q_SLOTS:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
        void onClickedView(const QModelIndex& idx);

    signals:
        void tableRootIndexChanged(const QModelIndex& idx);
        void showInTableFolderTriggered(const QModelIndex& idx);
        void rowDeselected();
        void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);
        
    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        bool m_showSearchResultsMode = false;
    };
}
