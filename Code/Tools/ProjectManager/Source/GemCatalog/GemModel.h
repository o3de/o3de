/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemInfo.h>
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

        void AddGem(const GemInfo& gemInfo);
        void Clear();

        QModelIndex FindIndexByNameString(const QString& nameString) const;
        void FindGemNamesByNameStrings(QStringList& inOutGemNames);
        QStringList GetDependingGemUuids(const QModelIndex& modelIndex);
        QStringList GetDependingGemNames(const QModelIndex& modelIndex);
        QStringList GetConflictingGemUuids(const QModelIndex& modelIndex);
        QStringList GetConflictingGemNames(const QModelIndex& modelIndex);

        static QString GetName(const QModelIndex& modelIndex);
        static QString GetCreator(const QModelIndex& modelIndex);
        static GemInfo::GemOrigin GetGemOrigin(const QModelIndex& modelIndex);
        static GemInfo::Platforms GetPlatforms(const QModelIndex& modelIndex);
        static GemInfo::Types GetTypes(const QModelIndex& modelIndex);
        static QString GetSummary(const QModelIndex& modelIndex);
        static QString GetDirectoryLink(const QModelIndex& modelIndex);
        static QString GetDocLink(const QModelIndex& modelIndex);
        static QString GetVersion(const QModelIndex& modelIndex);
        static QString GetLastUpdated(const QModelIndex& modelIndex);
        static int GetBinarySizeInKB(const QModelIndex& modelIndex);
        static QStringList GetFeatures(const QModelIndex& modelIndex);
        static QString GetPath(const QModelIndex& modelIndex);
        static QString GetRequirement(const QModelIndex& modelIndex);

        static bool IsAdded(const QModelIndex& modelIndex);
        static void SetIsAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isAdded);
        static void SetWasPreviouslyAdded(QAbstractItemModel& model, const QModelIndex& modelIndex, bool wasAdded);
        static bool NeedsToBeAdded(const QModelIndex& modelIndex);
        static bool NeedsToBeRemoved(const QModelIndex& modelIndex);
        static bool HasRequirement(const QModelIndex& modelIndex);

        bool DoGemsToBeAddedHaveRequirements() const;

        QVector<QModelIndex> GatherGemsToBeAdded() const;
        QVector<QModelIndex> GatherGemsToBeRemoved() const;

        int TotalAddedGems() const;

    private:
        enum UserRole
        {
            RoleName = Qt::UserRole,
            RoleCreator,
            RoleGemOrigin,
            RolePlatforms,
            RoleSummary,
            RoleWasPreviouslyAdded,
            RoleIsAdded,
            RoleDirectoryLink,
            RoleDocLink,
            RoleDependingGems,
            RoleConflictingGems,
            RoleVersion,
            RoleLastUpdated,
            RoleBinarySize,
            RoleFeatures,
            RoleTypes,
            RolePath,
            RoleRequirement
        };

        QHash<QString, QModelIndex> m_nameToIndexMap;
        QItemSelectionModel* m_selectionModel = nullptr;
    };
} // namespace O3DE::ProjectManager
