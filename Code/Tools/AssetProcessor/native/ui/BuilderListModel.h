/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#endif

namespace AZ
{
    struct Uuid;
}

struct BuilderListModel : QAbstractListModel
{
    Q_OBJECT;
public:
    BuilderListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    QModelIndex GetIndexForBuilder(const AZ::Uuid& builderUuid);

    void Reset();
};

struct BuilderListSortFilterProxy : QSortFilterProxyModel
{
    BuilderListSortFilterProxy(QObject* parent)
        : QSortFilterProxyModel(parent)
    {}

protected:
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
};
