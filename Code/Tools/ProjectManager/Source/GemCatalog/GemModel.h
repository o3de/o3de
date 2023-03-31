/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemInfo.h>
#include <TagWidget.h>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#endif

namespace O3DE::ProjectManager
{
    class GemModel
        : public QStandardItemModel
    {
        Q_OBJECT

    public:
        explicit GemModel(QObject* parent = nullptr);
        QItemSelectionModel* GetSelectionModel() const;

        enum UserRole
        {
            RoleName = Qt::UserRole,
            RoleDisplayName,
            RoleWasPreviouslyAdded,
            RoleWasPreviouslyAddedDependency,
            RoleIsAdded,
            RoleIsAddedDependency,
            RoleDependingGems,
            RoleVersion,            // the current version
            RoleNewVersion,         // the new version the user wants to use
            RoleDownloadStatus,
            RoleGemInfoVersions
        };

        QModelIndex AddGem(const GemInfo& gemInfo);
        QVector<QModelIndex> AddGems(const QVector<GemInfo>& gemInfos);
        void ActivateGems(const QHash<QString, QString>& enabledGemNames);
        void RemoveGem(const QModelIndex& modelIndex);
        void RemoveGem(const QString& gemName);
        void Clear();
        void UpdateGemDependencies();

        QModelIndex FindIndexByNameString(const QString& nameString) const;
        QVector<Tag> GetDependingGemTags(const QModelIndex& modelIndex);
        bool HasDependentGems(const QModelIndex& modelIndex) const;

        static const GemInfo GetGemInfo(const QModelIndex& modelIndex, const QString& version = "");
        static const QStringList GetGemVersions(const QModelIndex& modelIndex);
        static QString GetName(const QModelIndex& modelIndex);
        static QString GetDisplayName(const QModelIndex& modelIndex);
        static GemInfo::DownloadStatus GetDownloadStatus(const QModelIndex& modelIndex);
        static QString GetVersion(const QModelIndex& modelIndex);
        static QString GetNewVersion(const QModelIndex& modelIndex);
        static GemModel* GetSourceModel(QAbstractItemModel* model);
        static const GemModel* GetSourceModel(const QAbstractItemModel* model);

        static bool IsAdded(const QModelIndex& modelIndex);
        static bool IsAddedDependency(const QModelIndex& modelIndex);
        static void SetIsAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded, const QString& version = "");
        static void SetIsAddedDependency(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded);

        //! Set the version the user confirms they want to use
        static void SetNewVersion(QAbstractItemModel& model, const QModelIndex& modelIndex, const QString& version);
        static void SetWasPreviouslyAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded);
        static bool WasPreviouslyAdded(const QModelIndex& modelIndex);
        static void SetWasPreviouslyAddedDependency(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded);
        static bool WasPreviouslyAddedDependency(const QModelIndex& modelIndex);
        static bool NeedsToBeAdded(const QModelIndex& modelIndex, bool includeDependencies = false);
        static bool NeedsToBeRemoved(const QModelIndex& modelIndex, bool includeDependencies = false);
        static bool HasRequirement(const QModelIndex& modelIndex);
        static bool HasUpdates(const QModelIndex& modelIndex);
        static void UpdateDependencies(QAbstractItemModel& model, const QString& gemName, bool isAdded);
        static void UpdateWithVersion(QAbstractItemModel& model, const QModelIndex& modelIndex, const QString& version);
        static void DeactivateDependentGems(QAbstractItemModel& model, const QModelIndex& modelIndex);
        static void SetDownloadStatus(QAbstractItemModel& model, const QModelIndex& modelIndex, GemInfo::DownloadStatus status);

        bool DoGemsToBeAddedHaveRequirements() const;
        bool HasDependentGemsToRemove() const;

        QVector<QModelIndex> GatherGemDependencies(const QModelIndex& modelIndex) const;
        QVector<QModelIndex> GatherDependentGems(const QModelIndex& modelIndex, bool addedOnly = false) const;
        QVector<QModelIndex> GatherGemsToBeAdded(bool includeDependencies = false) const;
        QVector<QModelIndex> GatherGemsToBeRemoved(bool includeDependencies = false) const;

        int TotalAddedGems(bool includeDependencies = false) const;

    signals:
        void gemStatusChanged(const QString& gemName, uint32_t numChangedDependencies);
        void dependencyGemStatusChanged(const QString& gemName);

    protected slots: 
        void OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
        void OnRowsRemoved(const QModelIndex& parent, int first, int last);

    private:
        void GetAllDependingGems(const QModelIndex& modelIndex, QSet<QModelIndex>& inOutGems);
        QStringList GetDependingGems(const QModelIndex& modelIndex);

        QHash<QString, QModelIndex> m_nameToIndexMap;
        QItemSelectionModel* m_selectionModel = nullptr;
        QHash<QString, QSet<QModelIndex>> m_gemDependencyMap;
        QHash<QString, QSet<QModelIndex>> m_gemReverseDependencyMap;
    };
} // namespace O3DE::ProjectManager
