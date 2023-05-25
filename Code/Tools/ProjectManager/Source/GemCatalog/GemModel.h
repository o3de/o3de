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

        QPersistentModelIndex AddGem(const GemInfo& gemInfo);
        QVector<QPersistentModelIndex> AddGems(const QVector<GemInfo>& gemInfos, bool updateExisting = false);
        void ActivateGems(const QHash<QString, QString>& enabledGemNames);
        void RemoveGem(const QModelIndex& modelIndex);
        void RemoveGem(const QString& gemName, const QString& version = "", const QString& path = "");
        void Clear();
        void UpdateGemDependencies();

        QPersistentModelIndex FindIndexByNameString(const QString& nameString) const;
        QVector<Tag> GetDependingGemTags(const QModelIndex& modelIndex);
        bool HasDependentGems(const QModelIndex& modelIndex) const;

        /*
         * Get the GemInfo for the currently displayed item if no version or path is specified,
         * otherwise, return the gem info for the requested version and/or path
         * @param modelIndex The model index for the gem
         * @param version The optional version to find
         * @param path The optional path to find, this is often preferred over version because it is possible the user
         *             has multiple instances of the same gem registered
         */
        static const GemInfo GetGemInfo(const QPersistentModelIndex& modelIndex, const QString& version = "", const QString& path = "");
        static const QList<QVariant> GetGemVersions(const QModelIndex& modelIndex);
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
        static bool HasUpdates(const QModelIndex& modelIndex, bool compatibleOnly = true);
        static bool IsCompatible(const QModelIndex& modelIndex);
        static bool IsAddedMissing(const QModelIndex& modelIndex);
        static void UpdateDependencies(QAbstractItemModel& model, const QString& gemName, bool isAdded);
        static void UpdateWithVersion(
            QAbstractItemModel& model, const QPersistentModelIndex& modelIndex, const QString& version, const QString& path = "");
        static void DeactivateDependentGems(QAbstractItemModel& model, const QModelIndex& modelIndex);
        static void SetDownloadStatus(QAbstractItemModel& model, const QModelIndex& modelIndex, GemInfo::DownloadStatus status);

        bool DoGemsToBeAddedHaveRequirements() const;
        bool HasDependentGemsToRemove() const;

        QVector<QPersistentModelIndex> GatherGemDependencies(const QPersistentModelIndex& modelIndex) const;
        QVector<QPersistentModelIndex> GatherDependentGems(const QPersistentModelIndex& modelIndex, bool addedOnly = false) const;
        QVector<QModelIndex> GatherGemsToBeAdded(bool includeDependencies = false) const;
        QVector<QModelIndex> GatherGemsToBeRemoved(bool includeDependencies = false) const;

        int TotalAddedGems(bool includeDependencies = false) const;
        void ShowCompatibleGems();

    signals:
        void gemStatusChanged(const QString& gemName, uint32_t numChangedDependencies);
        void dependencyGemStatusChanged(const QString& gemName);

    protected slots: 
        void OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
        void OnRowsRemoved(const QModelIndex& parent, int first, int last);

    private:
        void GetAllDependingGems(const QModelIndex& modelIndex, QSet<QPersistentModelIndex>& inOutGems);
        QStringList GetDependingGems(const QModelIndex& modelIndex);
        QString GetMostCompatibleVersion(const QModelIndex& modelIndex); 
        bool VersionIsCompatible(const QModelIndex& modelIndex, const QString& version); 
        bool ShouldUpdateItemDataFromGemInfo(const QModelIndex& modelIndex, const GemInfo& gemInfo);

        QHash<QString, QPersistentModelIndex> m_nameToIndexMap;
        QItemSelectionModel* m_selectionModel = nullptr;
        QHash<QString, QSet<QPersistentModelIndex>> m_gemDependencyMap;
        QHash<QString, QSet<QPersistentModelIndex>> m_gemReverseDependencyMap;
    };
} // namespace O3DE::ProjectManager
