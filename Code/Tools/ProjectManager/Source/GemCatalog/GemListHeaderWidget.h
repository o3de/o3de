/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemFilterTagWidget.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <QFrame>
#endif

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
