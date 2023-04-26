/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QSortFilterProxyModel>
#endif

namespace O3DE::ProjectManager
{
    class GemRepoProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        explicit GemRepoProxyModel(QObject* parent = nullptr);

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    };
} // namespace O3DE::ProjectManager
