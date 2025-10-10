/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QListView>

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(AdjustableHeaderWidget)

    class GemListView
        : public QListView
    {
        Q_OBJECT

    public:
        explicit GemListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, AdjustableHeaderWidget* header, bool readOnly, QWidget* parent = nullptr);
        ~GemListView() = default;
    };
} // namespace O3DE::ProjectManager
