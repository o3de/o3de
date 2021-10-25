/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QListView>
#include <QItemSelectionModel>
#endif

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)

namespace O3DE::ProjectManager
{
    class GemRepoListView
        : public QListView
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemRepoListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent = nullptr);
        ~GemRepoListView() = default;

    signals:
        void RemoveRepo(const QModelIndex& modelIndex);
        void RefreshRepo(const QModelIndex& modelIndex);
    };
} // namespace O3DE::ProjectManager
