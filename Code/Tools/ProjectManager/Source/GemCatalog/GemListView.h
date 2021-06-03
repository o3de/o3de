/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include "GemInfo.h"
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
