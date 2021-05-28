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

#include <AzCore/std/string/string.h>
#include <GemCatalog/GemModel.h>

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
        const QString uuidString = gemInfo.m_uuid.ToString<AZStd::string>().c_str();
        item->setData(uuidString, RoleUuid);
        item->setData(gemInfo.m_creator, RoleCreator);
        item->setData(gemInfo.m_gemOrigin, RoleGemOrigin);
        item->setData(aznumeric_cast<int>(gemInfo.m_platforms), RolePlatforms);
        item->setData(aznumeric_cast<int>(gemInfo.m_types), RoleTypes);
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

        const QModelIndex modelIndex = index(rowCount()-1, 0);
        m_uuidToIndexMap[uuidString] = modelIndex;
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

    GemInfo::GemOrigin GemModel::GetGemOrigin(const QModelIndex& modelIndex)
    {
        return static_cast<GemInfo::GemOrigin>(modelIndex.data(RoleGemOrigin).toInt());
    }

    QString GemModel::GetUuidString(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleUuid).toString();
    }

    GemInfo::Platforms GemModel::GetPlatforms(const QModelIndex& modelIndex)
    {
        return static_cast<GemInfo::Platforms>(modelIndex.data(RolePlatforms).toInt());
    }

    GemInfo::Types GemModel::GetTypes(const QModelIndex& modelIndex)
    {
        return static_cast<GemInfo::Types>(modelIndex.data(RoleTypes).toInt());
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

    QModelIndex GemModel::FindIndexByUuidString(const QString& uuidString) const
    {
        const auto iterator = m_uuidToIndexMap.find(uuidString);
        if (iterator != m_uuidToIndexMap.end())
        {
            return iterator.value();
        }

        return {};
    }

    void GemModel::FindGemNamesByUuidStrings(QStringList& inOutGemNames)
    {
        for (QString& dependingGemString : inOutGemNames)
        {
            QModelIndex modelIndex = FindIndexByUuidString(dependingGemString);
            if (modelIndex.isValid())
            {
                dependingGemString = GetName(modelIndex);
            }
        }
    }

    QStringList GemModel::GetDependingGemUuids(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDependingGems).toStringList();
    }

    QStringList GemModel::GetDependingGemNames(const QModelIndex& modelIndex)
    {
        QStringList result = GetDependingGemUuids(modelIndex);
        if (result.isEmpty())
        {
            return {};
        }

        FindGemNamesByUuidStrings(result);
        return result;
    }

    QStringList GemModel::GetConflictingGemUuids(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleConflictingGems).toStringList();
    }

    QStringList GemModel::GetConflictingGemNames(const QModelIndex& modelIndex)
    {
        QStringList result = GetConflictingGemUuids(modelIndex);
        if (result.isEmpty())
        {
            return {};
        }

        FindGemNamesByUuidStrings(result);
        return result;
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
