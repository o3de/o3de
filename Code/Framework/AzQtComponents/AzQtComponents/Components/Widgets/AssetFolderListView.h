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
#include <AzQtComponents/Components/Widgets/TableView.h>
#endif

class QAbstractItemModel;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API AssetFolderListView
        : public TableView
    {
        Q_OBJECT
    public:
        explicit AssetFolderListView(QWidget* parent = nullptr);

        void setModel(QAbstractItemModel* model) override;
    };
}
