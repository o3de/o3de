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

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Outcome/Outcome.h>
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

        AZ::Outcome<QString> FindGemNameByUuidString(const QString& uuidString) const;
        QStringList GetDependingGems(const QModelIndex& modelIndex);

        static QString GetName(const QModelIndex& modelIndex);
        static QString GetCreator(const QModelIndex& modelIndex);
        static QString GetUuidString(const QModelIndex& modelIndex);
        static GemInfo::Platforms GetPlatforms(const QModelIndex& modelIndex);
        static GemInfo::Types GetTypes(const QModelIndex& modelIndex);
        static QString GetSummary(const QModelIndex& modelIndex);
        static bool IsAdded(const QModelIndex& modelIndex);
        static QString GetDirectoryLink(const QModelIndex& modelIndex);
        static QString GetDocLink(const QModelIndex& modelIndex);
        static QStringList GetConflictingGems(const QModelIndex& modelIndex);
        static QString GetVersion(const QModelIndex& modelIndex);
        static QString GetLastUpdated(const QModelIndex& modelIndex);
        static int GetBinarySizeInKB(const QModelIndex& modelIndex);
        static QStringList GetFeatures(const QModelIndex& modelIndex);

    private:
        enum UserRole
        {
            RoleName = Qt::UserRole,
            RoleUuid,
            RoleCreator,
            RolePlatforms,
            RoleSummary,
            RoleIsAdded,
            RoleDirectoryLink,
            RoleDocLink,
            RoleDependingGems,
            RoleConflictingGems,
            RoleVersion,
            RoleLastUpdated,
            RoleBinarySize,
            RoleFeatures,
            RoleTypes
        };

        QHash<QString, QString> m_uuidToNameMap;
        QItemSelectionModel* m_selectionModel = nullptr;
    };
} // namespace O3DE::ProjectManager
