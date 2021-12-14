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
#include <AzCore/Interface/Interface.h>
#include <utilities/BuilderInterface.h>
#endif

struct MyBuilderList : QAbstractListModel
{
    Q_OBJECT;
public:
    MyBuilderList(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role) const override;
};
