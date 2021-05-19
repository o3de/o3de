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

        item->setData(gemInfo.m_name, RoleName);
        item->setData(gemInfo.m_creator, RoleCreator);
        item->setData(static_cast<int>(gemInfo.m_platforms), RolePlatforms);
        item->setData(gemInfo.m_summary, RoleSummary);
        item->setData(gemInfo.m_isAdded, RoleIsAdded);

        item->setData(gemInfo.m_directoryLink, RoleDirectoryLink);
        item->setData(gemInfo.m_documentationLink, RoleDocLink);
        item->setData(gemInfo.m_dependingGemUuids, RoleDependingGems);
        item->setData(gemInfo.m_conflictingGemUuids, RoleConflictingGems);
        item->setData(gemInfo.m_version, RoleVersion);
        item->setData(gemInfo.m_lastUpdatedDate, RoleLastUpdated);
        item->setData(gemInfo.m_binarySizeInKB, RoleBinarySize);

        item->setData(gemInfo.m_features, RoleFeatures);

        appendRow(item);
    }

    void GemModel::Clear()
    {
        clear();
    }

    QString GemModel::GetName(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleName).toString();
    }

    QString GemModel::GetCreator(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleCreator).toString();
    }

    GemInfo::Platforms GemModel::GetPlatforms(const QModelIndex& modelIndex)
    {
        return static_cast<GemInfo::Platforms>(modelIndex.data(RolePlatforms).toInt());
    }

    QString GemModel::GetSummary(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleSummary).toString();
    }

    bool GemModel::IsAdded(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsAdded).toBool();
    }

    QString GemModel::GetDirectoryLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDirectoryLink).toString();
    }

    QString GemModel::GetDocLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDocLink).toString();
    }

    QStringList GemModel::GetDependingGems(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDependingGems).toStringList();
    }

    QStringList GemModel::GetConflictingGems(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleConflictingGems).toStringList();
    }

    QString GemModel::GetVersion(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleVersion).toString();
    }

    QString GemModel::GetLastUpdated(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleLastUpdated).toString();
    }

    int GemModel::GetBinarySizeInKB(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleBinarySize).toInt();
    }

    QStringList GemModel::GetFeatures(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleFeatures).toStringList();
    }
} // namespace O3DE::ProjectManager
