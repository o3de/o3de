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
        ~AssetFolderTableView() override;

        void SetShowSearchResultsMode(bool searchMode);
        void setRootIndex(const QModelIndex& index) override;

    signals:
        void rootIndexChanged(const QModelIndex& idx);
        void showInFolderTriggered(const QModelIndex& idx);

    private:
        bool m_showSearchResultsMode = false;
    };
}
