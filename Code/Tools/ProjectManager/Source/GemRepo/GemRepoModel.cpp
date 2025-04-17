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
#include <QSortFilterProxyModel>

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

    void SetItemDataSorted(QStandardItem* item, const QSet<QString>& stringSet, int role)
    {
        auto stringList = QStringList(stringSet.values());
        stringList.sort(Qt::CaseInsensitive);
        item->setData(stringList, role);
    }

    void GemRepoModel::AddGemRepo(const GemRepoInfo& gemRepoInfo)
    {
        QStandardItem* item = new QStandardItem();

        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        item->setData(gemRepoInfo.m_name, RoleName);
        item->setData(gemRepoInfo.m_origin, RoleCreator);
        item->setData(gemRepoInfo.m_summary, RoleSummary);
        item->setData(gemRepoInfo.m_isEnabled, RoleIsEnabled);
        item->setData(gemRepoInfo.m_directoryLink, RoleDirectoryLink);
        item->setData(gemRepoInfo.m_repoUri, RoleRepoUri);
        item->setData(gemRepoInfo.m_lastUpdated, RoleLastUpdated);
        item->setData(gemRepoInfo.m_path, RolePath);
        item->setData(gemRepoInfo.m_additionalInfo, RoleAdditionalInfo);
        item->setData(static_cast<int>(gemRepoInfo.m_badgeType), RoleBadgeType);

        appendRow(item);

        if (!gemRepoInfo.m_repoUri.isEmpty())
        {
            // gems - including gems from deactivated repos
            const auto& gemInfosResult = PythonBindingsInterface::Get()->GetGemInfosForRepo(gemRepoInfo.m_repoUri, /*enabledOnly*/false);
            if (gemInfosResult.IsSuccess())
            {
                const QVector<GemInfo>& gemInfos = gemInfosResult.GetValue();
                if (!gemInfos.isEmpty())
                {
                    // use a set to not include duplicate names because there are multiple versions of a gem
                    QSet<QString> includedGems;
                    for (const auto& gemInfo : gemInfos)
                    {
                        includedGems.insert(gemInfo.m_displayName.isEmpty() ? gemInfo.m_name : gemInfo.m_displayName);
                    }
                    SetItemDataSorted(item, includedGems, RoleIncludedGems);
                }
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Gems not found"), tr("Cannot find info for gems from repo %1").arg(gemRepoInfo.m_name));
            }

            // projects - including projects from deactivated repos
            const auto& projectInfosResult = PythonBindingsInterface::Get()->GetProjectsForRepo(gemRepoInfo.m_repoUri, /*enabledOnly*/false);
            if (projectInfosResult.IsSuccess())
            {
                const QVector<ProjectInfo>& projectInfos = projectInfosResult.GetValue();
                if (!projectInfos.isEmpty())
                {
                    // use a set to not include duplicate names because there are multiple versions of a gem
                    QSet<QString> includedProjects;
                    for (const auto& projectInfo : projectInfos)
                    {
                        includedProjects.insert(projectInfo.m_displayName.isEmpty() ? projectInfo.m_projectName : projectInfo.m_displayName);
                    }
                    SetItemDataSorted(item, includedProjects, RoleIncludedProjects);
                }
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Projects not found"), tr("Cannot find info for projects from repo %1").arg(gemRepoInfo.m_name));
            }

            // project templates - including projects from deactivated repos
            const auto& projectTemplateInfosResult =
                PythonBindingsInterface::Get()->GetProjectTemplatesForRepo(gemRepoInfo.m_repoUri, /*enabledOnly*/false);
            if (projectTemplateInfosResult.IsSuccess())
            {
                const QVector<ProjectTemplateInfo>& projectTemplateInfos = projectTemplateInfosResult.GetValue();
                if (!projectTemplateInfos.isEmpty())
                {
                    // use a set to not include duplicate names because there are multiple versions of a gem
                    QSet<QString> includedProjectTemplates;
                    for (const auto& projectTemplateInfo : projectTemplateInfos)
                    {
                        includedProjectTemplates.insert(projectTemplateInfo.m_displayName.isEmpty() ? projectTemplateInfo.m_name : projectTemplateInfo.m_displayName);
                    }
                    SetItemDataSorted(item, includedProjectTemplates, RoleIncludedProjectTemplates);
                }
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Project templates not found"), tr("Cannot find info for project templates from repo %1").arg(gemRepoInfo.m_name));
            }
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

    GemRepoInfo::BadgeType GemRepoModel::GetBadgeType(const QModelIndex& modelIndex)
    {
        return static_cast<GemRepoInfo::BadgeType>(modelIndex.data(RoleBadgeType).toInt());
    }

    QVector<Tag> TagsFromStringList(const QStringList& stringList)
    {
        if (stringList.isEmpty())
        {
            return {};
        }

        QVector<Tag> tags;
        tags.reserve(stringList.size());
        for (const QString& tagName : stringList)
        {
            tags.append({ tagName, tagName });
        }

        return tags;
    }

    QVector<Tag> GemRepoModel::GetIncludedGemTags(const QModelIndex& modelIndex)
    {
        return TagsFromStringList(modelIndex.data(RoleIncludedGems).toStringList());
    }

    QVector<Tag> GemRepoModel::GetIncludedProjectTags(const QModelIndex& modelIndex)
    {
        return TagsFromStringList(modelIndex.data(RoleIncludedProjects).toStringList());
    }

    QVector<Tag> GemRepoModel::GetIncludedProjectTemplateTags(const QModelIndex& modelIndex)
    {
        return TagsFromStringList(modelIndex.data(RoleIncludedProjectTemplates).toStringList());
    }

    bool GemRepoModel::IsEnabled(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsEnabled).toBool();
    }

    void GemRepoModel::SetEnabled(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isEnabled)
    {
        QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(&model);
        if (proxyModel)
        {
            GemRepoModel* repoModel = qobject_cast<GemRepoModel*>(proxyModel->sourceModel());
            if (repoModel)
            {
                repoModel->SetRepoEnabled(proxyModel->mapToSource(modelIndex), isEnabled);
            }
        }
    }

    void GemRepoModel::SetRepoEnabled(const QModelIndex& modelIndex, bool isEnabled)
    {
        const QString repoUri = GetRepoUri(modelIndex);
        const QString repoName = GetName(modelIndex);
        if(PythonBindingsInterface::Get()->SetRepoEnabled(repoUri, isEnabled))
        {
            if (isEnabled)
            {
                emit ShowToastNotification(tr("%1 activated").arg(repoName));
            }
            else
            {
                emit ShowToastNotification(tr("%1 deactivated").arg(repoName));
            }

            setData(modelIndex, isEnabled, RoleIsEnabled);
        }
        else
        {
            QMessageBox::critical(nullptr, tr("Failed to change repo status"), tr("Failed to change the repo status for %1.  The local repo.json cache file could be corrupt or the repo.json was not downloaded").arg(repoName));
        }
    }

    bool GemRepoModel::HasAdditionalInfo(const QModelIndex& modelIndex)
    {
        return !modelIndex.data(RoleAdditionalInfo).toString().isEmpty();
    }

    QPersistentModelIndex GemRepoModel::FindModelIndexByRepoUri(const QString& repoUri)
    {
        // the number of repos should be small enough that we don't need a hash
        for (int row = 0; row < rowCount(); ++row)
        {
            QModelIndex modelIndex = index(row, /*column*/ 0);
            if (modelIndex.isValid() && modelIndex.data(RoleRepoUri).toString() == repoUri)
            {
                return modelIndex;
            }
        }

        return {};
    }


} // namespace O3DE::ProjectManager
