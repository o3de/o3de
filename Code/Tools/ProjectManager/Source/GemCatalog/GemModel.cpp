/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>

namespace O3DE::ProjectManager
{
    GemModel::GemModel(QObject* parent)
        : QStandardItemModel(parent)
    {
        m_selectionModel = new QItemSelectionModel(this, parent);
        connect(this, &QAbstractItemModel::rowsAboutToBeRemoved, this, &GemModel::OnRowsAboutToBeRemoved);
    }

    QItemSelectionModel* GemModel::GetSelectionModel() const
    {
        return m_selectionModel;
    }

    QModelIndex GemModel::AddGem(const GemInfo& gemInfo)
    {
        if (FindIndexByNameString(gemInfo.m_name).isValid())
        {
            // do not add gems with duplicate names
            // this can happen by mistake or when a gem repo has a gem with the same name as a local gem
            AZ_TracePrintf("GemModel", "Ignoring duplicate gem: %s", gemInfo.m_name.toUtf8().constData());
            return QModelIndex();
        }

        QStandardItem* item = new QStandardItem();

        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        item->setData(gemInfo.m_name, RoleName);
        item->setData(gemInfo.m_displayName, RoleDisplayName);
        item->setData(gemInfo.m_creator, RoleCreator);
        item->setData(gemInfo.m_gemOrigin, RoleGemOrigin);
        item->setData(aznumeric_cast<int>(gemInfo.m_platforms), RolePlatforms);
        item->setData(aznumeric_cast<int>(gemInfo.m_types), RoleTypes);
        item->setData(gemInfo.m_summary, RoleSummary);
        item->setData(false, RoleWasPreviouslyAdded);
        item->setData(gemInfo.m_isAdded, RoleIsAdded);
        item->setData(gemInfo.m_directoryLink, RoleDirectoryLink);
        item->setData(gemInfo.m_documentationLink, RoleDocLink);
        item->setData(gemInfo.m_dependencies, RoleDependingGems);
        item->setData(gemInfo.m_version, RoleVersion);
        item->setData(gemInfo.m_lastUpdatedDate, RoleLastUpdated);
        item->setData(gemInfo.m_binarySizeInKB, RoleBinarySize);
        item->setData(gemInfo.m_features, RoleFeatures);
        item->setData(gemInfo.m_path, RolePath);
        item->setData(gemInfo.m_requirement, RoleRequirement);
        item->setData(gemInfo.m_downloadStatus, RoleDownloadStatus);
        item->setData(gemInfo.m_licenseText, RoleLicenseText);
        item->setData(gemInfo.m_licenseLink, RoleLicenseLink);
        item->setData(gemInfo.m_repoUri, RoleRepoUri);

        appendRow(item);

        const QModelIndex modelIndex = index(rowCount()-1, 0);
        m_nameToIndexMap[gemInfo.m_name] = modelIndex;

        return modelIndex;
    }

    void GemModel::RemoveGem(const QModelIndex& modelIndex)
    {
        removeRow(modelIndex.row());
    }

    void GemModel::RemoveGem(const QString& gemName)
    {
        auto nameFind = m_nameToIndexMap.find(gemName);
        if (nameFind != m_nameToIndexMap.end())
        {
            removeRow(nameFind->row());
        }
    }

    void GemModel::Clear()
    {
        clear();
        m_nameToIndexMap.clear();
    }

    void GemModel::UpdateGemDependencies()
    {
        m_gemDependencyMap.clear();
        m_gemReverseDependencyMap.clear();

        for (auto iter = m_nameToIndexMap.begin(); iter != m_nameToIndexMap.end(); ++iter)
        {
            const QString& key = iter.key();
            const QModelIndex modelIndex = iter.value();
            QSet<QModelIndex> dependencies;
            GetAllDependingGems(modelIndex, dependencies);
            if (!dependencies.isEmpty())
            {
                m_gemDependencyMap.insert(key, dependencies);
            }
        }

        for (auto iter = m_gemDependencyMap.begin(); iter != m_gemDependencyMap.end(); ++iter)
        {
            const QString& dependant = iter.key();
            for (const QModelIndex& dependency : iter.value())
            {
                const QString& dependencyName = dependency.data(RoleName).toString();
                if (!m_gemReverseDependencyMap.contains(dependencyName))
                {
                    m_gemReverseDependencyMap.insert(dependencyName, QSet<QModelIndex>());
                }

                m_gemReverseDependencyMap[dependencyName].insert(m_nameToIndexMap[dependant]);
            }
        }
    }

    QString GemModel::GetName(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleName).toString();
    }

    QString GemModel::GetDisplayName(const QModelIndex& modelIndex)
    {
        QString displayName = modelIndex.data(RoleDisplayName).toString();

        if (displayName.isEmpty())
        {
            return GetName(modelIndex);
        }
        else
        {
            return displayName;
        }
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

    GemInfo::DownloadStatus GemModel::GetDownloadStatus(const QModelIndex& modelIndex)
    {
        return static_cast<GemInfo::DownloadStatus>(modelIndex.data(RoleDownloadStatus).toInt());
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

    QStringList GemModel::GetDependingGems(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleDependingGems).toStringList();
    }

    void GemModel::GetAllDependingGems(const QModelIndex& modelIndex, QSet<QModelIndex>& inOutGems)
    {
        QStringList dependencies = GetDependingGems(modelIndex);
        for (const QString& dependency : dependencies)
        {
            QModelIndex dependencyIndex = FindIndexByNameString(dependency);
            if (!inOutGems.contains(dependencyIndex))
            {
                inOutGems.insert(dependencyIndex);
                GetAllDependingGems(dependencyIndex, inOutGems);
            }
        }
    }

    QVector<Tag> GemModel::GetDependingGemTags(const QModelIndex& modelIndex)
    {
        QVector<Tag> tags;

        QStringList dependingGemNames = GetDependingGems(modelIndex);
        tags.reserve(dependingGemNames.size());

        for (QString& gemName : dependingGemNames)
        {
            const QModelIndex& dependingIndex = FindIndexByNameString(gemName);
            if (dependingIndex.isValid())
            {
                tags.push_back({ GetDisplayName(dependingIndex), GetName(dependingIndex) });
            }
        }

        return tags;
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

    QString GemModel::GetLicenseText(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleLicenseText).toString();
    }

    QString GemModel::GetLicenseLink(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleLicenseLink).toString();
    }

    QString GemModel::GetRepoUri(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleRepoUri).toString();
    }

    GemModel* GemModel::GetSourceModel(QAbstractItemModel* model)
    {
        GemSortFilterProxyModel* proxyModel = qobject_cast<GemSortFilterProxyModel*>(model);
        if (proxyModel)
        {
            return proxyModel->GetSourceModel();
        }
        else
        {
            return qobject_cast<GemModel*>(model);
        }
    }

    const GemModel* GemModel::GetSourceModel(const QAbstractItemModel* model)
    {
        const GemSortFilterProxyModel* proxyModel = qobject_cast<const GemSortFilterProxyModel*>(model);
        if (proxyModel)
        {
            return proxyModel->GetSourceModel();
        }
        else
        {
            return qobject_cast<const GemModel*>(model);
        }
    }

    bool GemModel::IsAdded(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsAdded).toBool();
    }

    bool GemModel::IsAddedDependency(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleIsAddedDependency).toBool();
    }

    void GemModel::SetIsAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded)
    {
        // get the gemName first, because the modelIndex data change after adding because of filters
        QString gemName = modelIndex.data(RoleName).toString();
        model.setData(modelIndex, isAdded, RoleIsAdded);

        UpdateDependencies(model, gemName, isAdded);
    }

    bool GemModel::HasDependentGems(const QModelIndex& modelIndex) const
    {
        QVector<QModelIndex> dependentGems = GatherDependentGems(modelIndex);
        for (const QModelIndex& dependency : dependentGems)
        {
            if (IsAdded(dependency))
            {
                return true;
            }
        }
        return false;
    }

    void GemModel::UpdateDependencies(QAbstractItemModel& model, const QString& gemName, bool isAdded)
    {
        GemModel* gemModel = GetSourceModel(&model);
        AZ_Assert(gemModel, "Failed to obtain GemModel");

        QModelIndex modelIndex = gemModel->FindIndexByNameString(gemName);

        QVector<QModelIndex> dependencies = gemModel->GatherGemDependencies(modelIndex);
        uint32_t numChangedDependencies = 0;

        if (isAdded)
        {
            for (const QModelIndex& dependency : dependencies)
            {
                if (!IsAddedDependency(dependency))
                {
                    SetIsAddedDependency(*gemModel, dependency, true);

                    // if the gem was already added then the state didn't really change
                    if (!IsAdded(dependency))
                    {
                        numChangedDependencies++;
                        const QString dependencyName = gemModel->GetName(dependency);
                        gemModel->emit dependencyGemStatusChanged(dependencyName);
                    }
                }
            }
        }
        else
        {
            // still a dependency if some added gem depends on this one 
            bool hasDependentGems = gemModel->HasDependentGems(modelIndex);
            if (IsAddedDependency(modelIndex) != hasDependentGems)
            {
                SetIsAddedDependency(*gemModel, modelIndex, hasDependentGems);
            }

            for (const QModelIndex& dependency : dependencies)
            {
                hasDependentGems = gemModel->HasDependentGems(dependency);
                if (IsAddedDependency(dependency) != hasDependentGems)
                {
                    SetIsAddedDependency(*gemModel, dependency, hasDependentGems);

                    // if the gem was already added then the state didn't really change
                    if (!IsAdded(dependency))
                    {
                        numChangedDependencies++;
                        const QString dependencyName = gemModel->GetName(dependency);
                        gemModel->emit dependencyGemStatusChanged(dependencyName);
                    }
                }
            }
        }

        gemModel->emit gemStatusChanged(gemName, numChangedDependencies);
    }

    void GemModel::OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
    {
        bool selectedRowRemoved = false;
        for (int i = first; i <= last; ++i)
        {
            QModelIndex modelIndex = index(i, 0, parent);
            const QString& gemName = GetName(modelIndex);
            m_nameToIndexMap.remove(gemName);

            if (GetSelectionModel()->isRowSelected(i))
            {
                selectedRowRemoved = true;
            }
        }

        // Select a valid row if currently selected row was removed
        if (selectedRowRemoved)
        {
            for (const QModelIndex& index : m_nameToIndexMap)
            {
                if (index.isValid())
                {
                    GetSelectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
                    break;
                }
            }
        }
    }

    void GemModel::SetIsAddedDependency(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded)
    {
        model.setData(modelIndex, isAdded, RoleIsAddedDependency);
    }

    void GemModel::SetWasPreviouslyAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded)
    {
        model.setData(modelIndex, wasAdded, RoleWasPreviouslyAdded);

        if (wasAdded)
        {
            // update all dependencies
            GemModel* gemModel = GetSourceModel(&model);
            AZ_Assert(gemModel, "Failed to obtain GemModel");
            QVector<QModelIndex> dependencies = gemModel->GatherGemDependencies(modelIndex);
            for (const QModelIndex& dependency : dependencies)
            {
                SetWasPreviouslyAddedDependency(*gemModel, dependency, true);
            }
        }
    }

    void GemModel::SetWasPreviouslyAddedDependency(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded)
    {
        model.setData(modelIndex, wasAdded, RoleWasPreviouslyAddedDependency);
    }

    bool GemModel::WasPreviouslyAdded(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleWasPreviouslyAdded).toBool();
    }

    bool GemModel::WasPreviouslyAddedDependency(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleWasPreviouslyAddedDependency).toBool();
    }

    bool GemModel::NeedsToBeAdded(const QModelIndex& modelIndex, bool includeDependencies)
    {
        bool previouslyAdded = modelIndex.data(RoleWasPreviouslyAdded).toBool();
        bool added = modelIndex.data(RoleIsAdded).toBool();
        if (includeDependencies)
        {
            previouslyAdded |= modelIndex.data(RoleWasPreviouslyAddedDependency).toBool();
            added |= modelIndex.data(RoleIsAddedDependency).toBool();
        }
        return !previouslyAdded && added;
    }

    bool GemModel::NeedsToBeRemoved(const QModelIndex& modelIndex, bool includeDependencies)
    {
        bool previouslyAdded = modelIndex.data(RoleWasPreviouslyAdded).toBool();
        bool added = modelIndex.data(RoleIsAdded).toBool();
        if (includeDependencies)
        {
            previouslyAdded |= modelIndex.data(RoleWasPreviouslyAddedDependency).toBool();
            added |= modelIndex.data(RoleIsAddedDependency).toBool();
        }
        return previouslyAdded && !added;
    }

    void GemModel::DeactivateDependentGems(QAbstractItemModel& model, const QModelIndex& modelIndex)
    {
        GemModel* gemModel = GetSourceModel(&model);
        AZ_Assert(gemModel, "Failed to obtain GemModel");

        QVector<QModelIndex> dependentGems = gemModel->GatherDependentGems(modelIndex);
        if (!dependentGems.isEmpty())
        {
            // we need to deactivate all gems that depend on this one
            for (auto dependentModelIndex : dependentGems)
            {
                SetIsAdded(model, dependentModelIndex, false);
            }

        }
    }

    void GemModel::SetDownloadStatus(QAbstractItemModel& model, const QModelIndex& modelIndex, GemInfo::DownloadStatus status)
    {
        model.setData(modelIndex, status, RoleDownloadStatus);
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

    bool GemModel::HasDependentGemsToRemove() const
    {
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (GemModel::NeedsToBeRemoved(modelIndex, /*includeDependencies=*/true) &&
                GemModel::WasPreviouslyAddedDependency(modelIndex))
            {
                return true;
            }
        }
        return false;
    }

    QVector<QModelIndex> GemModel::GatherGemDependencies(const QModelIndex& modelIndex) const 
    {
        QVector<QModelIndex> result;
        const QString& gemName = modelIndex.data(RoleName).toString();
        if (m_gemDependencyMap.contains(gemName))
        {
            for (const QModelIndex& dependency : m_gemDependencyMap[gemName])
            {
                result.push_back(dependency);
            }
        }
        return result;
    }

    QVector<QModelIndex> GemModel::GatherDependentGems(const QModelIndex& modelIndex, bool addedOnly) const
    {
        QVector<QModelIndex> result;
        const QString& gemName = modelIndex.data(RoleName).toString();
        if (m_gemReverseDependencyMap.contains(gemName))
        {
            for (const QModelIndex& dependency : m_gemReverseDependencyMap[gemName])
            {
                if (!addedOnly || GemModel::IsAdded(dependency))
                {
                    result.push_back(dependency);
                }
            }
        }
        return result;
    }

    QVector<QModelIndex> GemModel::GatherGemsToBeAdded(bool includeDependencies) const
    {
        QVector<QModelIndex> result;
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (NeedsToBeAdded(modelIndex, includeDependencies))
            {
                result.push_back(modelIndex);
            }
        }
        return result;
    }

    QVector<QModelIndex> GemModel::GatherGemsToBeRemoved(bool includeDependencies) const
    {
        QVector<QModelIndex> result;
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (NeedsToBeRemoved(modelIndex, includeDependencies))
            {
                result.push_back(modelIndex);
            }
        }
        return result;
    }

    int GemModel::TotalAddedGems(bool includeDependencies) const
    {
        int result = 0;
        for (int row = 0; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0);
            if (IsAdded(modelIndex) || (includeDependencies && IsAddedDependency(modelIndex)))
            {
                ++result;
            }
        }
        return result;
    }
} // namespace O3DE::ProjectManager
