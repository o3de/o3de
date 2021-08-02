/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QSortFilterProxyModel>

namespace AssetBundler
{
    class AssetBundlerFileTableFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AssetBundlerFileTableFilterModel(QObject* parent, int displayNameCol, int dateTimeCol = -1);

        void FilterChanged(const QString& newFilter);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

        int m_displayNameCol = 0;
        int m_dateTimeCol = -1;
    };
}
