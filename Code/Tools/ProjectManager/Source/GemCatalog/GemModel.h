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
#include <QStandardItemModel>
#include <QItemSelectionModel>
#endif

namespace O3DE::ProjectManager
{
    class GemModel
        : public QStandardItemModel
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemModel(QObject* parent = nullptr);
        QItemSelectionModel* GetSelectionModel() const;

        void AddGem(const GemInfo& gemInfo);
        void Clear();

        QString GetName(const QModelIndex& modelIndex) const;
        QString GetCreator(const QModelIndex& modelIndex) const;
        int GetPlatforms(const QModelIndex& modelIndex) const;
        QString GetSummary(const QModelIndex& modelIndex) const;
        bool IsAdded(const QModelIndex& modelIndex) const;

    private:
        enum UserRole
        {
            NAME = 0,
            CREATOR = 1,
            PLATFORMS = 2,
            SUMMARY = 3,
            GETSTATE = 4
        };

        QItemSelectionModel* m_selectionModel = nullptr;
    };
} // namespace O3DE::ProjectManager
