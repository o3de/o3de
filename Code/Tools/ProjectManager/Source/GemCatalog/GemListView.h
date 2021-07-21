/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QListView>
#endif

namespace O3DE::ProjectManager
{
    class GemListView
        : public QListView
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent = nullptr);
        ~GemListView() = default;
    };
} // namespace O3DE::ProjectManager
