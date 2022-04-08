/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/LevelRootUiHandler.h>

#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPainterPath>
#include <QTreeView>

namespace AzToolsFramework
{
    AzFramework::EntityContextId LevelRootUiHandler::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

    const QColor LevelRootUiHandler::s_levelRootBorderColor = QColor("#656565");
    const QColor LevelRootUiHandler::s_levelRootEditColor = QColor("#4A90E2");
    const QColor LevelRootUiHandler::s_levelRootOverrideColor = QColor("#C76D0E");
    const QString LevelRootUiHandler::s_levelRootIconPath = QString(":/Level/level.svg");

    LevelRootUiHandler::LevelRootUiHandler()
    {
        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        if (m_prefabPublicInterface == nullptr)
        {
            AZ_Assert(false, "LevelRootUiHandler - could not get PrefabPublicInterface on LevelRootUiHandler construction.");
            return;
        }

        m_prefabFocusPublicInterface = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
        if (m_prefabFocusPublicInterface == nullptr)
        {
            AZ_Assert(false, "LevelRootUiHandler - could not get PrefabFocusPublicInterface on LevelRootUiHandler construction.");
            return;
        }

        // Get EditorEntityContextId
        EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);
    }

    QIcon LevelRootUiHandler::GenerateItemIcon([[maybe_unused]] AZ::EntityId entityId) const
    {
        return QIcon(s_levelRootIconPath);
    }

    QString LevelRootUiHandler::GenerateItemInfoString(AZ::EntityId entityId) const
    {
        QString infoString;

        AZ::IO::Path path = m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId);

        if (!path.empty())
        {
            QString saveFlag = "";
            auto dirtyOutcome = m_prefabPublicInterface->HasUnsavedChanges(path);

            if (dirtyOutcome.IsSuccess() && dirtyOutcome.GetValue() == true)
            {
                saveFlag = "*";
            }

            infoString = QObject::tr("<span style=\"font-style: italic; font-weight: 400;\">(%1%2)</span>")
                             .arg(path.Filename().Native().data())
                             .arg(saveFlag);
        }

        return infoString;
    }

    bool LevelRootUiHandler::CanToggleLockVisibility([[maybe_unused]] AZ::EntityId entityId) const
    {
        return false;
    }

    bool LevelRootUiHandler::CanRename([[maybe_unused]] AZ::EntityId entityId) const
    {
        return false;
    }

    void LevelRootUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (!painter)
        {
            AZ_Warning("LevelRootUiHandler", false, "LevelRootUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        const QSize iconSize = QSize(32, 16);
        QModelIndex firstColumnIndex = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        AZ::EntityId entityId(firstColumnIndex.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        QPen borderLinePen(s_levelRootBorderColor, s_levelRootBorderThickness);

        QRect rect = option.rect;

        /*
        QString prefabEditScopeIconPath = ":/stylesheet/img/UI20/toggleswitch/unchecked.svg";
        if (m_prefabFocusPublicInterface->GetPrefabEditScope(s_editorEntityContextId) == Prefab::PrefabEditScope::NESTED_INSTANCES)
        {
            backgroundColor = s_levelRootOverrideColor;
            prefabEditScopeIconPath = ":/stylesheet/img/UI20/toggleswitch/checked.svg";
        }
        */

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        /*
        // Draw background if the root is focused.
        if (m_prefabFocusPublicInterface->GetOpenInstanceMode() == 1 && m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            // Paint toggle icon
            if (index.column() == EntityOutlinerListModel::ColumnVisibilityToggle || index.column() == EntityOutlinerListModel::ColumnLockToggle)
            {
                QPoint offset = QPoint(6, 4);
                if (index.column() == EntityOutlinerListModel::ColumnLockToggle)
                {
                    offset = QPoint(-14, 4);
                }

                QIcon openIcon = QIcon(prefabEditScopeIconPath);
                painter->drawPixmap(option.rect.topLeft() + offset, openIcon.pixmap(iconSize));
            }
        }
        */

        // Draw border at the bottom.
        painter->setPen(borderLinePen);
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());

        painter->restore();
    }

    bool LevelRootUiHandler::OnOutlinerItemClick(
        [[maybe_unused]] const QPoint& position,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index) const
    {
        /*
        QModelIndex firstColumnIndex = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        AZ::EntityId entityId(firstColumnIndex.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());

        if (index.column() == EntityOutlinerListModel::ColumnVisibilityToggle ||
            index.column() == EntityOutlinerListModel::ColumnLockToggle)
        {
            if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId) && option.rect.contains(position))
            {
                if (m_prefabFocusPublicInterface->GetPrefabEditScope(s_editorEntityContextId) == Prefab::PrefabEditScope::NESTED_TEMPLATES)
                {
                    m_prefabFocusPublicInterface->SetPrefabEditScope(s_editorEntityContextId, Prefab::PrefabEditScope::NESTED_INSTANCES);
                }
                else
                {
                    m_prefabFocusPublicInterface->SetPrefabEditScope(s_editorEntityContextId, Prefab::PrefabEditScope::NESTED_TEMPLATES);
                }

                // Don't propagate event.
                return true;
            }
        }
        */
        return false;
    }

    bool LevelRootUiHandler::OnOutlinerItemDoubleClick(const QModelIndex& index) const
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        if (auto prefabFocusPublicInterface = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
            !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
        }

        // Don't propagate event.
        return true;
    }
}
