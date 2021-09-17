/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoModel.h>

#include <QItemSelectionModel>

namespace O3DE::ProjectManager
{
    GemRepoModel::GemRepoModel(QObject* parent)
        : QStandardItemModel(parent)
    {
        m_selectionModel = new QItemSelectionModel(this, parent);
    }

    QItemSelectionModel* GemRepoModel::GetSelectionModel() const
    {
        return m_selectionModel;
    }

    void GemRepoModel::AddGemRepo(const GemRepoInfo& gemRepoInfo)
    {
        QStandardItem* item = new QStandardItem();

        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        item->setData(gemRepoInfo.m_name, RoleName);
        item->setData(gemRepoInfo.m_creator, RoleCreator);
        item->setData(gemRepoInfo.m_summary, RoleSummary);
        item->setData(gemRepoInfo.m_isEnabled, RoleIsEnabled);
        item->setData(gemRepoInfo.m_directoryLink, RoleDirectoryLink);
        item->setData(gemRepoInfo.m_repoLink, RoleRepoLink);
        item->setData(gemRepoInfo.m_lastUpdated, RoleLastUpdated);
        item->setData(gemRepoInfo.m_path, RolePath);

        appendRow(item);
    }

    void GemRepoModel::Clear()
    {
        clear();
    }

    QString GemRepoModel::GetName(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleName).toString();
    }

    QString GemRepoModel::GetCreator(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleCreator).toString();
    }

    QString GemRepoModel::GetSummary(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleSummary).toString();
    }

    QString GemRepoModel::GetDirectoryLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDirectoryLink).toString();
    }

    QString GemRepoModel::GetRepoLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleRepoLink).toString();
    }

    QDateTime GemRepoModel::GetLastUpdated(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleLastUpdated).toDateTime();
    }

    QString GemRepoModel::GetPath(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RolePath).toString();
    }

    bool GemRepoModel::IsEnabled(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsEnabled).toBool();
    }

    void GemRepoModel::SetEnabled(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isEnabled)
    {
        model.setData(modelIndex, isEnabled, RoleIsEnabled);
    }

} // namespace O3DE::ProjectManager
