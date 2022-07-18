/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <QSortFilterProxyModel>

namespace AssetProcessor
{
    class BuilderInfoMetricsSortModel : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        BuilderInfoMetricsSortModel(QObject* parent = nullptr);
    };
} // namespace AssetProcessor
