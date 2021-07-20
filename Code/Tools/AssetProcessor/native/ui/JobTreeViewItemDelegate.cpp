/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "JobTreeViewItemDelegate.h"

namespace AssetProcessor
{
    void JobTreeViewItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
    {
        AzQtComponents::TableViewItemDelegate::initStyleOption(option, index);

        option->features &= ~(QStyleOptionViewItem::WrapText);
    }
}

#include <native/ui/moc_JobTreeViewItemDelegate.cpp>
