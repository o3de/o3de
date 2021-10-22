/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoModel.h>
#include <PythonBindings.h>

#include <QItemSelectionModel>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    GemRepoModel::GemRepoModel(QObject* parent)
        : QStandardItemModel(parent)
    {
        m_selectionModel = new QItemSelectionModel(this, parent);
        m_gemModel = new GemModel(this);
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
        item->setData(gemRepoInfo.m_repoUri, RoleRepoUri);
        item->setData(gemRepoInfo.m_lastUpdated, RoleLastUpdated);
        item->setData(gemRepoInfo.m_path, RolePath);
        item->setData(gemRepoInfo.m_additionalInfo, RoleAdditionalInfo);
        item->setData(gemRepoInfo.m_includedGemPaths, RoleIncludedGems);

        appendRow(item);

        QVector<GemInfo> includedGemInfos = GetIncludedGemInfos(item->index());

        for (const GemInfo& gemInfo : includedGemInfos)
        {
            m_gemModel->AddGem(gemInfo);
        }
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

    QString GemRepoModel::GetAdditionalInfo(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleAdditionalInfo).toString();
    }

    QString GemRepoModel::GetDirectoryLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDirectoryLink).toString();
    }

    QString GemRepoModel::GetRepoUri(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleRepoUri).toString();
    }

    QDateTime GemRepoModel::GetLastUpdated(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleLastUpdated).toDateTime();
    }

    QString GemRepoModel::GetPath(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RolePath).toString();
    }

    QStringList GemRepoModel::GetIncludedGemPaths(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIncludedGems).toStringList();
    }

    QStringList GemRepoModel::GetIncludedGemNames(const QModelIndex& modelIndex)
    {
        QStringList gemNames;
        QVector<GemInfo> gemInfos = GetIncludedGemInfos(modelIndex);

        for (const GemInfo& gemInfo : gemInfos)
        {
            gemNames.append(gemInfo.m_displayName);
        }

        return gemNames;
    }

    QVector<GemInfo> GemRepoModel::GetIncludedGemInfos(const QModelIndex& modelIndex)
    {
        QVector<GemInfo> allGemInfos;
        QStringList repoGemPaths = GetIncludedGemPaths(modelIndex);

        for (const QString& gemPath : repoGemPaths)
        {
            AZ::Outcome<GemInfo> gemInfoResult = PythonBindingsInterface::Get()->GetGemInfo(gemPath);
            if (gemInfoResult.IsSuccess())
            {
                allGemInfos.append(gemInfoResult.GetValue());
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Gem Not Found"), tr("Cannot find info for gem %1.").arg(gemPath));
            }
        }

        return allGemInfos;
    }

    bool GemRepoModel::IsEnabled(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsEnabled).toBool();
    }

    void GemRepoModel::SetEnabled(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isEnabled)
    {
        model.setData(modelIndex, isEnabled, RoleIsEnabled);
    }

    bool GemRepoModel::HasAdditionalInfo(const QModelIndex& modelIndex)
    {
        return !modelIndex.data(RoleAdditionalInfo).toString().isEmpty();
    }

} // namespace O3DE::ProjectManager
