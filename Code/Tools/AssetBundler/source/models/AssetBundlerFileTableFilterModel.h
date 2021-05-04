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
