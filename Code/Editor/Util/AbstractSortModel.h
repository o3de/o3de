/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ABSTRACTSORTMODEL_H
#define ABSTRACTSORTMODEL_H

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#endif

class AbstractSortModel
    : public QAbstractTableModel
{
    Q_OBJECT

public:
    AbstractSortModel(QObject* parent = nullptr);

    virtual bool LessThan(const QModelIndex& lhs, const QModelIndex& rhs) const;
};

#endif // ABSTRACTSORTMODEL_H
