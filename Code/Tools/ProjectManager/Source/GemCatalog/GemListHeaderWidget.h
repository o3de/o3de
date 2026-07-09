/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GemCatalog/GemFilterTagWidget.h>
#include <GemCatalog/GemSortFilterProxyModel.h>

#include <QFrame>

namespace O3DE::ProjectManager
{
    class GemListHeaderWidget
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit GemListHeaderWidget(GemSortFilterProxyModel* proxyModel, QWidget* parent = nullptr);
        ~GemListHeaderWidget() = default;

    signals:
        void OnRefresh(bool refreshRemoteRepos);
    };
} // namespace O3DE::ProjectManager
