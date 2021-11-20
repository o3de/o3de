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
        Q_OBJECT // AUTOMOC

    public:
        explicit GemModel(QObject* parent = nullptr);
        QItemSelectionModel* GetSelectionModel() const;

        enum UserRole
        {
            RoleName = Qt::UserRole,
            RoleDisplayName,
            RoleCreator,
            RoleGemOrigin,
            RolePlatforms,
            RoleSummary,
            RoleWasPreviouslyAdded,
            RoleWasPreviouslyAddedDependency,
            RoleIsAdded,
            RoleIsAddedDependency,
            RoleDirectoryLink,
            RoleDocLink,
            RoleDependingGems,
            RoleVersion,
            RoleLastUpdated,
            RoleBinarySize,
            RoleFeatures,
            RoleTypes,
            RolePath,
            RoleRequirement,
            RoleDownloadStatus,
            RoleLicenseText,
            RoleLicenseLink,
            RoleRepoUri
        };

        QModelIndex AddGem(const GemInfo& gemInfo);
        void RemoveGem(const QModelIndex& modelIndex);
        void RemoveGem(const QString& gemName);
        void Clear();
        void UpdateGemDependencies();

        QModelIndex FindIndexByNameString(const QString& nameString) const;
        QVector<Tag> GetDependingGemTags(const QModelIndex& modelIndex);
        bool HasDependentGems(const QModelIndex& modelIndex) const;

        static QString GetName(const QModelIndex& modelIndex);
        static QString GetDisplayName(const QModelIndex& modelIndex);
        static QString GetCreator(const QModelIndex& modelIndex);
        static GemInfo::GemOrigin GetGemOrigin(const QModelIndex& modelIndex);
        static GemInfo::Platforms GetPlatforms(const QModelIndex& modelIndex);
        static GemInfo::Types GetTypes(const QModelIndex& modelIndex);
        static GemInfo::DownloadStatus GetDownloadStatus(const QModelIndex& modelIndex);
        static QString GetSummary(const QModelIndex& modelIndex);
        static QString GetDirectoryLink(const QModelIndex& modelIndex);
        static QString GetDocLink(const QModelIndex& modelIndex);
        static QString GetVersion(const QModelIndex& modelIndex);
        static QString GetLastUpdated(const QModelIndex& modelIndex);
        static int GetBinarySizeInKB(const QModelIndex& modelIndex);
        static QStringList GetFeatures(const QModelIndex& modelIndex);
        static QString GetPath(const QModelIndex& modelIndex);
        static QString GetRequirement(const QModelIndex& modelIndex);
        static QString GetLicenseText(const QModelIndex& modelIndex);
        static QString GetLicenseLink(const QModelIndex& modelIndex);
        static QString GetRepoUri(const QModelIndex& modelIndex);
        static GemModel* GetSourceModel(QAbstractItemModel* model);
        static const GemModel* GetSourceModel(const QAbstractItemModel* model);

        static bool IsAdded(const QModelIndex& modelIndex);
        static bool IsAddedDependency(const QModelIndex& modelIndex);
        static void SetIsAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded);
        static void SetIsAddedDependency(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded);
        static void SetWasPreviouslyAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded);
        static bool WasPreviouslyAdded(const QModelIndex& modelIndex);
        static void SetWasPreviouslyAddedDependency(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded);
        static bool WasPreviouslyAddedDependency(const QModelIndex& modelIndex);
        static bool NeedsToBeAdded(const QModelIndex& modelIndex, bool includeDependencies = false);
        static bool NeedsToBeRemoved(const QModelIndex& modelIndex, bool includeDependencies = false);
        static bool HasRequirement(const QModelIndex& modelIndex);
        static void UpdateDependencies(QAbstractItemModel& model, const QString& gemName, bool isAdded);
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

    private:
        void GetAllDependingGems(const QModelIndex& modelIndex, QSet<QModelIndex>& inOutGems);
        QStringList GetDependingGems(const QModelIndex& modelIndex);

        QHash<QString, QModelIndex> m_nameToIndexMap;
        QItemSelectionModel* m_selectionModel = nullptr;
        QHash<QString, QSet<QModelIndex>> m_gemDependencyMap;
        QHash<QString, QSet<QModelIndex>> m_gemReverseDependencyMap;
    };
} // namespace O3DE::ProjectManager
