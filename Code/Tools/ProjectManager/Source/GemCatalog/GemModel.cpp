/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        item->setData(gemInfo.m_creator, RoleCreator);
        item->setData(gemInfo.m_gemOrigin, RoleGemOrigin);
        item->setData(aznumeric_cast<int>(gemInfo.m_platforms), RolePlatforms);
        item->setData(aznumeric_cast<int>(gemInfo.m_types), RoleTypes);
        item->setData(gemInfo.m_summary, RoleSummary);
        item->setData(false, RoleWasPreviouslyAdded);
        item->setData(gemInfo.m_isAdded, RoleIsAdded);
        item->setData(gemInfo.m_directoryLink, RoleDirectoryLink);
        item->setData(gemInfo.m_documentationLink, RoleDocLink);
        item->setData(gemInfo.m_dependingGemUuids, RoleDependingGems);
        item->setData(gemInfo.m_conflictingGemUuids, RoleConflictingGems);
        item->setData(gemInfo.m_version, RoleVersion);
        item->setData(gemInfo.m_lastUpdatedDate, RoleLastUpdated);
        item->setData(gemInfo.m_binarySizeInKB, RoleBinarySize);
        item->setData(gemInfo.m_features, RoleFeatures);
        item->setData(gemInfo.m_path, RolePath);
        item->setData(gemInfo.m_requirement, RoleRequirement);

        appendRow(item);

        const QModelIndex modelIndex = index(rowCount()-1, 0);
        m_nameToIndexMap[gemInfo.m_name] = modelIndex;
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

    QString GemModel::GetDirectoryLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDirectoryLink).toString();
    }

    QString GemModel::GetDocLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDocLink).toString();
    }

    QModelIndex GemModel::FindIndexByNameString(const QString& nameString) const
    {
        const auto iterator = m_nameToIndexMap.find(nameString);
        if (iterator != m_nameToIndexMap.end())
        {
            return iterator.value();
        }

        return {};
    }

    void GemModel::FindGemNamesByNameStrings(QStringList& inOutGemNames)
    {
        for (QString& dependingGemString : inOutGemNames)
        {
            QModelIndex modelIndex = FindIndexByNameString(dependingGemString);
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

        FindGemNamesByNameStrings(result);
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

        FindGemNamesByNameStrings(result);
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

    QString GemModel::GetPath(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RolePath).toString();
    }

    QString GemModel::GetRequirement(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleRequirement).toString();
    }

    bool GemModel::IsAdded(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsAdded).toBool();
    }

    void GemModel::SetIsAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded)
    {
        model.setData(modelIndex, isAdded, RoleIsAdded);
    }

    void GemModel::SetWasPreviouslyAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded)
    {
        model.setData(modelIndex, wasAdded, RoleWasPreviouslyAdded);
    }

    bool GemModel::NeedsToBeAdded(const QModelIndex& modelIndex)
    {
        return (!modelIndex.data(RoleWasPreviouslyAdded).toBool() && modelIndex.data(RoleIsAdded).toBool());
    }

    bool GemModel::NeedsToBeRemoved(const QModelIndex& modelIndex)
    {
        return (modelIndex.data(RoleWasPreviouslyAdded).toBool() && !modelIndex.data(RoleIsAdded).toBool());
    }

    bool GemModel::HasRequirement(const QModelIndex& modelIndex)
    {
        return !modelIndex.data(RoleRequirement).toString().isEmpty();
    }

    bool GemModel::DoGemsToBeAddedHaveRequirements() const
    {
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (NeedsToBeAdded(modelIndex) && HasRequirement(modelIndex))
            {
                return true;
            }
        }
        return false;
    }

    QVector<QModelIndex> GemModel::GatherGemsToBeAdded() const
    {
        QVector<QModelIndex> result;
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (NeedsToBeAdded(modelIndex))
            {
                result.push_back(modelIndex);
            }
        }
        return result;
    }

    QVector<QModelIndex> GemModel::GatherGemsToBeRemoved() const
    {
        QVector<QModelIndex> result;
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (NeedsToBeRemoved(modelIndex))
            {
                result.push_back(modelIndex);
            }
        }
        return result;
    }

    int GemModel::TotalAddedGems() const
    {
        int result = 0;
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (IsAdded(modelIndex))
            {
                ++result;
            }
        }
        return result;
    }

} // namespace O3DE::ProjectManager
