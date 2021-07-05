/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/TableView.h>
#endif

namespace AssetProcessor
{
    class JobTreeViewItemDelegate
        : public AzQtComponents::TableViewItemDelegate
    {
        Q_OBJECT

    public:
        using AzQtComponents::TableViewItemDelegate::TableViewItemDelegate;

    protected:
        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    };
} // namespace AssetProcessor
