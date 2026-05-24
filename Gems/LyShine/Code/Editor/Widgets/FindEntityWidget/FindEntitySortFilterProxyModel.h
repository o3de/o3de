/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>
#include <QSortFilterProxyModel>
#endif

/*!
 * Enables the Find Entity widget to filter entries based on search string.
 */
class FindEntitySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(FindEntitySortFilterProxyModel, AZ::SystemAllocator);

    FindEntitySortFilterProxyModel(QObject* pParent = nullptr);

    void UpdateFilter();

    // Qt overrides
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
};
