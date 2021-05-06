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

#include "GemModel.h"

namespace O3DE::ProjectManager
{
    GemModel::GemModel(QObject* parent)
        : QStandardItemModel(parent)
    {
        m_selectionModel = new QItemSelectionModel(this, parent);
    }

    QItemSelectionModel* GemModel::GetSelectionModel() const
    {
        return m_selectionModel;
    }

    void GemModel::AddGem(const GemInfo& gemInfo)
    {
        QStandardItem* item = new QStandardItem();

        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        item->setData(gemInfo.m_name, Qt::UserRole + NAME);
        item->setData(gemInfo.m_creator, Qt::UserRole + CREATOR);
        item->setData(static_cast<int>(gemInfo.m_platforms), Qt::UserRole + PLATFORMS);
        item->setData(gemInfo.m_summary, Qt::UserRole + SUMMARY);
        item->setData(gemInfo.m_isAdded, Qt::UserRole + GETSTATE);

        appendRow(item);
    }

    void GemModel::Clear()
    {
        clear();
    }

    QString GemModel::GetName(const QModelIndex& modelIndex) const
    {
        return modelIndex.data(Qt::UserRole + NAME).toString();
    }

    QString GemModel::GetCreator(const QModelIndex& modelIndex) const
    {
        return modelIndex.data(Qt::UserRole + CREATOR).toString();
    }

    int GemModel::GetPlatforms(const QModelIndex& modelIndex) const
    {
        return static_cast<GemInfo::Platforms>(modelIndex.data(Qt::UserRole + GemModel::PLATFORMS).toInt());
    }

    QString GemModel::GetSummary(const QModelIndex& modelIndex) const
    {
        return modelIndex.data(Qt::UserRole + SUMMARY).toString();
    }

    bool GemModel::IsAdded(const QModelIndex& modelIndex) const
    {
        return modelIndex.data(Qt::UserRole + GETSTATE).toBool();
    }
} // namespace O3DE::ProjectManager
