/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "AbstractSortModel.h"

AbstractSortModel::AbstractSortModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

bool AbstractSortModel::LessThan(const QModelIndex& lhs, const QModelIndex& rhs) const
{
    return lhs.data().toString() < rhs.data().toString();
}

#include <Util/moc_AbstractSortModel.cpp>
