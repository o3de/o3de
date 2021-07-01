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
    class GemRequirementListView
        : public QListView
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemRequirementListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent = nullptr);
        ~GemRequirementListView() = default;
    };
} // namespace O3DE::ProjectManager
