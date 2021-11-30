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
        item->setData(gemRepoInfo.m_includedGemUris, RoleIncludedGems);

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

    QStringList GemRepoModel::GetIncludedGemUris(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIncludedGems).toStringList();
    }

    QVector<Tag> GemRepoModel::GetIncludedGemTags(const QModelIndex& modelIndex)
    {
        QVector<Tag> tags;
        const QVector<GemInfo>& gemInfos = GetIncludedGemInfos(modelIndex);
        tags.reserve(gemInfos.size());
        for (const GemInfo& gemInfo : gemInfos)
        {
            tags.append({ gemInfo.m_displayName, gemInfo.m_name });
        }

        return tags;
    }

    QVector<GemInfo> GemRepoModel::GetIncludedGemInfos(const QModelIndex& modelIndex)
    {
        QString repoUri = GetRepoUri(modelIndex);

        const AZ::Outcome<QVector<GemInfo>, AZStd::string>& gemInfosResult = PythonBindingsInterface::Get()->GetGemInfosForRepo(repoUri);
        if (gemInfosResult.IsSuccess())
        {
            return gemInfosResult.GetValue();
        }
        else
        {
            QMessageBox::critical(nullptr, tr("Gems not found"), tr("Cannot find info for gems from repo %1").arg(GetName(modelIndex)));
        }

        return QVector<GemInfo>();
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
