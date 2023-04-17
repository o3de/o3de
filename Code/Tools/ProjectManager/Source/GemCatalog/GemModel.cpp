/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/Dependency/Dependency.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QList>

namespace O3DE::ProjectManager
{
    GemModel::GemModel(QObject* parent)
        : QStandardItemModel(parent)
    {
        m_selectionModel = new QItemSelectionModel(this, parent);
        connect(this, &QAbstractItemModel::rowsAboutToBeRemoved, this, &GemModel::OnRowsAboutToBeRemoved);
        connect(this, &QAbstractItemModel::rowsRemoved, this, &GemModel::OnRowsRemoved);
    }

    QItemSelectionModel* GemModel::GetSelectionModel() const
    {
        return m_selectionModel;
    }

    void SetItemDataFromGemInfo(QStandardItem* item, const GemInfo& gemInfo, bool metaDataOnly = false)
    {
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setData(gemInfo.m_name, GemModel::RoleName);
        item->setData(gemInfo.m_displayName, GemModel::RoleDisplayName);
        item->setData(gemInfo.m_dependencies, GemModel::RoleDependingGems);
        item->setData(gemInfo.m_version, GemModel::RoleVersion);
        item->setData(gemInfo.m_downloadStatus, GemModel::RoleDownloadStatus);

        if (!metaDataOnly)
        {
            item->setData(false, GemModel::RoleWasPreviouslyAdded);
            item->setData(gemInfo.m_isAdded, GemModel::RoleIsAdded);
            item->setData("", GemModel::RoleNewVersion);
        }
    }

    void AddGemInfoVersion(QStandardItem* item, const GemInfo& gemInfo, bool update)
    {
        QList<QVariant> versionList;
        auto variant = item->data(GemModel::RoleGemInfoVersions);
        if (variant.isValid())
        {
            versionList = variant.value<QList<QVariant>>();
        }
        QVariant gemVariant;
        gemVariant.setValue(gemInfo);

        // sanity check we aren't adding a gem with an existing path and version
        int versionToReplaceIndex = -1;
        for (int i = 0; i < versionList.size(); ++i)
        {
            const QVariant& existingGemVariant = versionList.at(i);
            const GemInfo& existingGemInfo = existingGemVariant.value<GemInfo>();
            if (QDir(existingGemInfo.m_path) == QDir(gemInfo.m_path))
            {
                if (existingGemInfo.m_version == gemInfo.m_version && !update)
                {
                    AZ_Info("ProjectManager", "Not adding GemInfo because a GemInfo with path (%s) and version (%s) already exists.",
                        gemInfo.m_path.toUtf8().constData(),
                        gemInfo.m_version.toUtf8().constData()
                        );
                    return;
                }

                // the path is the same but the version has changed, update the info
                versionToReplaceIndex = i;
                break;
            }
            else if (existingGemInfo.m_version == gemInfo.m_version &&
                existingGemInfo.m_downloadStatus == GemInfo::NotDownloaded &&
                gemInfo.m_downloadStatus == GemInfo::Downloaded)
            {
                // we are adding  a gem version for a gem that has been downloaded
                // so replace the content for remote gem
                versionToReplaceIndex = i;
                break;
            }

            if (existingGemInfo.m_version == gemInfo.m_version &&
                existingGemInfo.m_downloadStatus == GemInfo::Downloaded &&
                gemInfo.m_downloadStatus == GemInfo::NotDownloaded)
            {
                // do not add the not downloaded remote version of
                // something we have downloaded
                return;
            }
        }

        if(versionToReplaceIndex != -1)
        {
            versionList.replace(versionToReplaceIndex, gemVariant);
        }
        else
        {
            versionList.append(gemVariant);
        }
        item->setData(versionList, GemModel::RoleGemInfoVersions);
    }

    bool RemoveGemInfoVersion(QStandardItem* item, const QString& version, const QString& path)
    {
        QVariant variant = item->data(GemModel::RoleGemInfoVersions);
        auto versionList = variant.isValid() ? variant.value<QList<QVariant>>() : QList<QVariant>();
        const bool removeByPath = !path.isEmpty();

        QDir dir{ path };
        for (int i = 0; i < versionList.size(); ++i)
        {
            const QVariant& existingGemVariant = versionList.at(i);
            const GemInfo& existingGemInfo = existingGemVariant.value<GemInfo>();
            if (removeByPath)
            {
                if (QDir(existingGemInfo.m_path) == dir)
                {
                    versionList.removeAt(i);
                    break;
                }
            }
            else if (existingGemInfo.m_version == version)
            {
                // there could be multiple instances of the same version
                versionList.removeAt(i);
            }
        }

        item->setData(versionList, GemModel::RoleGemInfoVersions);
        return versionList.isEmpty();
    }

    QVector<QPersistentModelIndex> GemModel::AddGems(const QVector<GemInfo>& gemInfos, bool updateExisting)
    {
        QVector<QPersistentModelIndex> indexesChanged;
        const int initialNumRows = rowCount();

        // block dataChanged signal if we are adding a bunch of stuff
        // to avoid sending a ton of signals that might cause large UI updates 
        // and slows us down till we are done
        blockSignals(true);

        for (const auto& gemInfo : gemInfos)
        {
            // ${Name} is a special name used in templates and should not be shown 
            // Though potentially it should be swapped out with the name of the Project being created
            if (gemInfo.m_name == "${Name}")
            {
                continue;
            }

            auto modelIndex = FindIndexByNameString(gemInfo.m_name);
            if (modelIndex.isValid())
            {
                auto gemItem = itemFromIndex(modelIndex);
                AZ_Assert(gemItem, "Failed to retrieve existing gem item from model index");

                // if this is a greater version than the existing version
                // or we are updating the existing version, update
                int versionResult = ProjectUtils::VersionCompare(gemInfo.m_version, gemItem->data(RoleVersion).toString());
                if (versionResult > 0 || (versionResult == 0 && updateExisting))
                {
                    SetItemDataFromGemInfo(gemItem, gemInfo, /*metaDataOnly=*/ true);
                }

                AddGemInfoVersion(gemItem, gemInfo, updateExisting);

                indexesChanged.append(modelIndex);
            }
            else
            {
                auto gemItem = new QStandardItem();
                SetItemDataFromGemInfo(gemItem, gemInfo);
                AddGemInfoVersion(gemItem, gemInfo, updateExisting);
                appendRow(gemItem); 

                modelIndex = index(rowCount() - 1, 0);
                indexesChanged.append(modelIndex);

                m_nameToIndexMap[gemInfo.m_name] = modelIndex;
            }
        }

        blockSignals(false);

        // send a single dataChanged signal now that we've added everything
        // this does not include rows that were changed and not added
        const int startRow = AZStd::max(0, initialNumRows - 1);
        const int endRow = AZStd::max(0, rowCount() - 1);
        emit dataChanged(index(startRow, 0), index(endRow, 0));

        return indexesChanged;
    }

    void GemModel::ActivateGems(const QHash<QString, QString>& enabledGemNames)
    {
        // block dataChanged signal if we are modifying a bunch of data 
        // to avoid sending a many signals that might cause large UI updates 
        // and slows us down till we are done
        blockSignals(true);

        for (auto itr = enabledGemNames.cbegin(); itr != enabledGemNames.cend(); itr++)
        {
            const QString& gemPath = itr.value();
            const QString& gemNameWithSpecifier = itr.key();
            AZ::Dependency<AZ::SemanticVersion::parts_count> dependency;
            auto parseOutcome = dependency.ParseVersions({ gemNameWithSpecifier.toUtf8().constData() });
            const QString& gemName = parseOutcome ? dependency.GetName().c_str() : gemNameWithSpecifier; 
            if (gemName == "${Name}")
            {
                // ${Name} is a special name used in templates and is replaced with a real gem name later 
                // in theory we could replace the name here with the project gem's name
                continue;
            }

            if (auto nameFoundIter = m_nameToIndexMap.find(gemName); nameFoundIter != m_nameToIndexMap.end())
            {
                const QModelIndex modelIndex = nameFoundIter.value();
                const auto& versionList = modelIndex.data(RoleGemInfoVersions).value<QList<QVariant>>();
                if (versionList.count() > 1 && !gemPath.isEmpty())
                {
                    // make sure the gem item delegate displays the correct version info 
                    for (auto versionVariant : versionList)
                    {
                        const auto& variantGemInfo = versionVariant.value<GemInfo>();
                        if (QDir(gemPath) == QDir(variantGemInfo.m_path))
                        {
                            QStandardItem* gemItem = itemFromIndex(modelIndex);
                            AZ_Assert(gemItem, "Failed to retrieve enabled gem item from model index");
                            SetItemDataFromGemInfo(gemItem, variantGemInfo);
                            break;
                        }
                    }
                }

                // Set Added/PreviouslyAdded after potentially updating data above which might remove
                // those settings
                GemModel::SetWasPreviouslyAdded(*this, modelIndex, true);
                GemModel::SetIsAdded(*this, modelIndex, true);

                continue;
            }

            // This gem info is missing, but the project uses it so show it to the user
            // so they can remove it if they want to
            // In the future we want to let the user browse to this gem's location on disk, or
            // let them download it
            GemInfo gemInfo;
            gemInfo.m_name = gemName;
            gemInfo.m_displayName = gemName;
            gemInfo.m_version = parseOutcome ? dependency.GetBounds().at(0).ToString().c_str() : "";
            gemInfo.m_summary = QString("This project uses %1 but a compatible gem was not found, or has not been registered yet.").arg(gemNameWithSpecifier);
            gemInfo.m_isAdded = true;

            QStandardItem* gemItem = new QStandardItem();
            SetItemDataFromGemInfo(gemItem, gemInfo);
            AddGemInfoVersion(gemItem, gemInfo, /*updateExisting=*/false);
            appendRow(gemItem); 

            const auto modelIndex = index(rowCount() - 1, 0);
            GemModel::SetWasPreviouslyAdded(*this, modelIndex, true);
            GemModel::SetIsAdded(*this, modelIndex, true);

            m_nameToIndexMap[gemInfo.m_name] = modelIndex;

            AZ_Warning("ProjectManager::GemCatalog", false,
                "Cannot find entry for gem with name '%s'. The CMake target name probably does not match the specified name in the gem.json.",
                gemName.toUtf8().constData());
        }

        blockSignals(false);

        // send a single dataChanged signal now that we've added everything
        emit dataChanged(index(0, 0), index(AZStd::max(0, rowCount() - 1), 0));
    }

    QPersistentModelIndex GemModel::AddGem(const GemInfo& gemInfo)
    {
        if (const auto& indexes = AddGems({gemInfo}); !indexes.isEmpty())
        {
            return indexes.at(0);
        }

        return index(rowCount()-1, 0);
    }

    void GemModel::RemoveGem(const QModelIndex& modelIndex)
    {
        removeRow(modelIndex.row());
    }

    void GemModel::RemoveGem(const QString& gemName, const QString& version, const QString& path)
    {
        auto nameFind = m_nameToIndexMap.find(gemName);
        if (nameFind != m_nameToIndexMap.end())
        {
            if (!version.isEmpty() || !path.isEmpty())
            {
                const bool removedAllVersions = RemoveGemInfoVersion(itemFromIndex(nameFind.value()), version, path);
                if (removedAllVersions)
                {
                    removeRow(nameFind->row());
                }
            }
            else
            {
                removeRow(nameFind->row());
            }
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
            QSet<QPersistentModelIndex> dependencies;
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
                    m_gemReverseDependencyMap.insert(dependencyName, QSet<QPersistentModelIndex>());
                }

                m_gemReverseDependencyMap[dependencyName].insert(m_nameToIndexMap[dependant]);
            }
        }
    }

    const GemInfo GemModel::GetGemInfo(const QPersistentModelIndex& modelIndex, const QString& version, const QString& path)
    {
        const auto& versionList = modelIndex.data(RoleGemInfoVersions).value<QList<QVariant>>();
        const QString& gemVersion = modelIndex.data(RoleVersion).toString();
        if (versionList.isEmpty())
        {
            return {};
        }
        else if (gemVersion.isEmpty() && version.isEmpty())
        {
            // no version to look for so return the first GemInfo
            return versionList.at(0).value<GemInfo>();
        }

        bool usePath = !path.isEmpty();
        bool useVersion = !version.isEmpty();
        bool useCurrentVersion = !useVersion && !usePath;
        for (const auto& versionVariant : versionList)
        {
            // there may be multiple instances of the same gem with the same version
            // at different paths
            const QString& variantVersion = versionVariant.value<GemInfo>().m_version;
            const QString& variantPath = versionVariant.value<GemInfo>().m_path;

            // if no version is provided, try to find the one that matches the current version
            // if a path and/or version is provided try to find an exact match
            if ((useCurrentVersion && gemVersion == variantVersion) ||
                (usePath && variantPath == path) ||
                (!usePath && useVersion && variantVersion == version))
            {
                return versionVariant.value<GemInfo>();
            }
        }

        // no gem info found for this version
        return {};
    }

    const QList<QVariant> GemModel::GetGemVersions(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleGemInfoVersions).value<QList<QVariant>>();
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

    GemInfo::DownloadStatus GemModel::GetDownloadStatus(const QModelIndex& modelIndex)
    {
        return static_cast<GemInfo::DownloadStatus>(modelIndex.data(RoleDownloadStatus).toInt());
    }

    QPersistentModelIndex GemModel::FindIndexByNameString(const QString& nameString) const
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

    void GemModel::OnRowsRemoved(const QModelIndex& parent, int first, [[maybe_unused]] int last)
    {
        // fix up the name to index map for all rows that changed
        for (int row = first; row < rowCount(); ++row)
        {
            const QModelIndex modelIndex = index(row, 0, parent);
            m_nameToIndexMap[GetName(modelIndex)] = modelIndex;
        }
    }

    void GemModel::GetAllDependingGems(const QModelIndex& modelIndex, QSet<QPersistentModelIndex>& inOutGems)
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

    QString GemModel::GetNewVersion(const QModelIndex& modelIndex)
    {
        return modelIndex.data(RoleNewVersion).toString();
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

    void GemModel::SetIsAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded, const QString& version)
    {
        // get the gemName first, because the modelIndex data change after adding because of filters
        QString gemName = modelIndex.data(RoleName).toString();
        model.setData(modelIndex, isAdded, RoleIsAdded);

        if (!version.isEmpty())
        {
            QString gemVersion = modelIndex.data(RoleVersion).toString();
            model.setData(modelIndex, version == gemVersion ? "" : version, RoleNewVersion);
        }

        UpdateDependencies(model, gemName, isAdded);
    }

    void GemModel::SetNewVersion(QAbstractItemModel& model, const QModelIndex& modelIndex, const QString& version)
    {
        model.setData(modelIndex, version, RoleNewVersion);
    }

    bool GemModel::HasDependentGems(const QModelIndex& modelIndex) const
    {
        auto dependentGems = GatherDependentGems(modelIndex);
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

        auto modelIndex = gemModel->FindIndexByNameString(gemName);

        auto dependencies = gemModel->GatherGemDependencies(modelIndex);
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

            for (const auto& dependency : dependencies)
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

    void GemModel::UpdateWithVersion(QAbstractItemModel& model, const QPersistentModelIndex& modelIndex, const QString& version, const QString& path)
    {
        GemModel* gemModel = GetSourceModel(&model);
        AZ_Assert(gemModel, "Failed to obtain GemModel");
        AZ_Assert(&model == modelIndex.model(), "Model is different - did you use the proxy or selection model instead of source?");
        AZ_Assert(modelIndex.isValid(), "Invalid model index");
        auto gemItem = gemModel->itemFromIndex(modelIndex);
        AZ_Assert(gemItem, "Failed to obtain gem model item");
        SetItemDataFromGemInfo(gemItem, GetGemInfo(modelIndex, version, path), /*metaDataOnly*/ true);
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
            auto dependencies = gemModel->GatherGemDependencies(modelIndex);
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
        QString newVersion = modelIndex.data(RoleNewVersion).toString();
        if (includeDependencies)
        {
            previouslyAdded |= modelIndex.data(RoleWasPreviouslyAddedDependency).toBool();
            added |= modelIndex.data(RoleIsAddedDependency).toBool();
        }
        return (!previouslyAdded && added) || (added && !newVersion.isEmpty());
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

        auto dependentGems = gemModel->GatherDependentGems(modelIndex);
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
        return !GemModel::GetGemInfo(modelIndex).m_requirement.isEmpty();
    }

    bool GemModel::HasUpdates(const QModelIndex& modelIndex)
    {
        const auto& gemInfo = GemModel::GetGemInfo(modelIndex);
        if (gemInfo.m_isEngineGem)
        {
            // engine gems are only updated with the engine
            return false;
        }

        const auto& versions = GemModel::GetGemVersions(modelIndex);
        if (versions.count() < 2)
        {
            return false;
        }

        auto currentVersion = modelIndex.data(RoleVersion).toString();

        // gem versions are sorted so we can just compare if we're using the latest version
        return currentVersion != versions.at(0).value<GemInfo>().m_version;
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

    QVector<QPersistentModelIndex> GemModel::GatherGemDependencies(const QPersistentModelIndex& modelIndex) const 
    {
        QVector<QPersistentModelIndex> result;
        const QString& gemName = modelIndex.data(RoleName).toString();
        if (m_gemDependencyMap.contains(gemName))
        {
            for (const auto& dependency : m_gemDependencyMap[gemName])
            {
                result.push_back(dependency);
            }
        }
        return result;
    }

    QVector<QPersistentModelIndex> GemModel::GatherDependentGems(const QPersistentModelIndex& modelIndex, bool addedOnly) const
    {
        QVector<QPersistentModelIndex> result;
        const QString& gemName = modelIndex.data(RoleName).toString();
        if (m_gemReverseDependencyMap.contains(gemName))
        {
            for (const auto& dependency : m_gemReverseDependencyMap[gemName])
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
