/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStandardItemModel>
#include <GemRepo/GemRepoInfo.h>
#include <TagWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)

namespace O3DE::ProjectManager
{
    class GemRepoModel
        : public QStandardItemModel
    {
        Q_OBJECT

    public:
        explicit GemRepoModel(QObject* parent = nullptr);
        QItemSelectionModel* GetSelectionModel() const;

        void AddGemRepo(const GemRepoInfo& gemInfo);
        void Clear();

        static QString GetName(const QModelIndex& modelIndex);
        static QString GetCreator(const QModelIndex& modelIndex);
        static QString GetSummary(const QModelIndex& modelIndex);
        static QString GetAdditionalInfo(const QModelIndex& modelIndex);
        static QString GetDirectoryLink(const QModelIndex& modelIndex);
        static QString GetRepoUri(const QModelIndex& modelIndex);
        static QDateTime GetLastUpdated(const QModelIndex& modelIndex);
        static QString GetPath(const QModelIndex& modelIndex);
        static GemRepoInfo::BadgeType GetBadgeType(const QModelIndex& modelIndex);

        static QVector<Tag> GetIncludedGemTags(const QModelIndex& modelIndex);
        static QVector<Tag> GetIncludedProjectTags(const QModelIndex& modelIndex);
        static QVector<Tag> GetIncludedProjectTemplateTags(const QModelIndex& modelIndex);

        static bool IsEnabled(const QModelIndex& modelIndex);
        static void SetEnabled(QAbstractItemModel& model, const QModelIndex& modelIndex, bool isEnabled);
        static bool HasAdditionalInfo(const QModelIndex& modelIndex);

        enum UserRole
        {
            RoleName = Qt::UserRole,
            RoleCreator,
            RoleSummary,
            RoleIsEnabled,
            RoleDirectoryLink,
            RoleRepoUri,
            RoleLastUpdated,
            RolePath,
            RoleAdditionalInfo,
            RoleIncludedGems,
            RoleIncludedProjects,
            RoleIncludedProjectTemplates,
            RoleBadgeType
        };

        void SetRepoEnabled(const QModelIndex& modelIndex, bool isEnabled);
        QPersistentModelIndex FindModelIndexByRepoUri(const QString& repoUri);

    signals:
        void ShowToastNotification(const QString& notification);

    private:
        QItemSelectionModel* m_selectionModel = nullptr;
    };
} // namespace O3DE::ProjectManager
