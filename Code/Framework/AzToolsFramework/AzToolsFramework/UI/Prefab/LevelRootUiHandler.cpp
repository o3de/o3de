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
#include <AzToolsFramework/UI/Prefab/Constants.h>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPainterPath>
#include <QTreeView>

namespace AzToolsFramework
{
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
    }

    QIcon LevelRootUiHandler::GenerateItemIcon([[maybe_unused]] AZ::EntityId entityId) const
    {
        return QIcon(m_levelRootIconPath);
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

    void LevelRootUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
    {
        if (!painter)
        {
            AZ_Warning("LevelRootUiHandler", false, "LevelRootUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        // Draw border line at the bottom
        QPen borderLinePen(m_levelRootBorderColor, PrefabUIConstants::levelDividerThickness);
        QRect rect = option.rect;
        painter->setPen(borderLinePen);
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());

        // Draw capsule
        const bool isFirstColumn = index.column() == EntityOutlinerListModel::ColumnName;
        const bool isLastColumn = index.column() == EntityOutlinerListModel::ColumnLockToggle;

        QColor backgroundColor = m_prefabCapsuleColor;
        AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(levelContainerEntityId))
        {
            backgroundColor = m_prefabCapsuleEditColor;
        }
        else if (!(option.state & QStyle::State_Enabled))
        {
            backgroundColor = m_prefabCapsuleDisabledColor;
        }

        QPainterPath backgroundPath;
        backgroundPath.setFillRule(Qt::WindingFill);

        QRect columnRect = option.rect;
        columnRect.setTop(columnRect.top() + 2);
        columnRect.setHeight(columnRect.height() - 4); // tweaking capsule height

        if (isFirstColumn || isLastColumn)
        {
            if (isFirstColumn)
            {
                columnRect.setLeft(columnRect.left() + 1);
                columnRect.setWidth(columnRect.width() + 1);
            }
            else if (isLastColumn)
            {
                columnRect.setLeft(columnRect.left() - 2);
                columnRect.setWidth(columnRect.width() - 1);
            }

            // Rounded rect to have rounded borders on top.
            backgroundPath.addRoundedRect(columnRect, PrefabUIConstants::prefabCapsuleRadius, PrefabUIConstants::prefabCapsuleRadius);

            // Regular rect, half height, to square the opposite border
            QRect squareRect = columnRect;
            if (isFirstColumn)
            {
                squareRect.setLeft(columnRect.left() + (columnRect.width() / 2));
            }
            else if (isLastColumn)
            {
                squareRect.setWidth(columnRect.width() / 2);
            }
            backgroundPath.addRect(squareRect);
        }
        else
        {
            backgroundPath.addRect(columnRect);
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(backgroundPath.simplified(), backgroundColor);

        painter->restore();
    }

    bool LevelRootUiHandler::OnOutlinerItemClick(
        [[maybe_unused]] const QPoint& position,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index) const
    {
        return false;
    }

    bool LevelRootUiHandler::OnOutlinerItemDoubleClick(const QModelIndex& index) const
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        if (!m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
        }

        // Don't propagate event.
        return true;
    }
}
