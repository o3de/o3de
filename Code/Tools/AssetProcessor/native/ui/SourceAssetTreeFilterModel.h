/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ui/AssetTreeFilterModel.h>

namespace AssetProcessor
{
    class SourceAssetTreeFilterModel : public AssetTreeFilterModel
    {
        Q_OBJECT
    public:
        SourceAssetTreeFilterModel(QObject* parent = nullptr);

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    };
} // namespace AssetProcessor
