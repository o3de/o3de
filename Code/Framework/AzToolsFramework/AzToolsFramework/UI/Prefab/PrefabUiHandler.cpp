/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabUiHandler.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPainterPath>
#include <QTreeView>

namespace AzToolsFramework
{
    const QColor PrefabUiHandler::m_backgroundColor = QColor("#444444");
    const QColor PrefabUiHandler::m_backgroundHoverColor = QColor("#5A5A5A");
    const QColor PrefabUiHandler::m_backgroundSelectedColor = QColor("#656565");
    const QColor PrefabUiHandler::m_prefabCapsuleColor = QColor("#1E252F");
    const QColor PrefabUiHandler::m_prefabCapsuleDisabledColor = QColor("#35383C");
    const QColor PrefabUiHandler::m_prefabCapsuleEditColor = QColor("#4A90E2");
    const QString PrefabUiHandler::m_prefabIconPath = QString(":/Entity/prefab.svg");
    const QString PrefabUiHandler::m_prefabEditIconPath = QString(":/Entity/prefab_edit.svg");
    const QString PrefabUiHandler::m_prefabEditOpenIconPath = QString(":/Entity/prefab_edit_open.svg");
    const QString PrefabUiHandler::m_prefabEditCloseIconPath = QString(":/Entity/prefab_edit_close.svg");

    PrefabUiHandler::PrefabUiHandler()
    {
        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        if (m_prefabPublicInterface == nullptr)
        {
            AZ_Assert(false, "PrefabUiHandler - could not get PrefabPublicInterface on PrefabUiHandler construction.");
            return;
        }

        m_prefabFocusPublicInterface = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
        if (m_prefabFocusPublicInterface == nullptr)
        {
            AZ_Assert(false, "PrefabUiHandler - could not get PrefabFocusPublicInterface on PrefabUiHandler construction.");
            return;
        }
    }

    QString PrefabUiHandler::GenerateItemInfoString(AZ::EntityId entityId) const
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

    QString PrefabUiHandler::GenerateItemTooltip(AZ::EntityId entityId) const
    {
        QString tooltip;

        AZ::IO::Path path = m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId);

        if (!path.empty())
        {
            tooltip = QObject::tr("Double click to edit.\n%1").arg(path.Native().data());
        }

        return tooltip;
    }

    QIcon PrefabUiHandler::GenerateItemIcon(AZ::EntityId entityId) const
    {
        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            return QIcon(m_prefabEditIconPath);
        }

        return QIcon(m_prefabIconPath);
    }

    void PrefabUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (!painter)
        {
            AZ_Warning("PrefabUiHandler", false, "PrefabUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        const bool isFirstColumn = index.column() == EntityOutlinerListModel::ColumnName;
        const bool isLastColumn = index.column() == EntityOutlinerListModel::ColumnLockToggle;
        QModelIndex firstColumnIndex = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        const bool hasVisibleChildren =
            firstColumnIndex.data(EntityOutlinerListModel::ExpandedRole).value<bool>() &&
            firstColumnIndex.model()->hasChildren(firstColumnIndex);

        QColor backgroundColor = m_prefabCapsuleColor;
        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            backgroundColor = m_prefabCapsuleEditColor;
        }
        else if (!(option.state & QStyle::State_Enabled))
        {
            backgroundColor = m_prefabCapsuleDisabledColor;
        }

        QPainterPath backgroundPath;
        backgroundPath.setFillRule(Qt::WindingFill);

        QRect tempRect = option.rect;
        tempRect.setTop(tempRect.top() + 1);

        if (!hasVisibleChildren)
        {
            tempRect.setBottom(tempRect.bottom() - 1);
        }

        if (isFirstColumn)
        {
            tempRect.setLeft(tempRect.left() - 1);
        }

        if (isLastColumn)
        {
            tempRect.setWidth(tempRect.width() - 1);
        }

        if (isFirstColumn || isLastColumn)
        {
            // Rounded rect to have rounded borders on top
            backgroundPath.addRoundedRect(tempRect, m_prefabCapsuleRadius, m_prefabCapsuleRadius);

            if (hasVisibleChildren)
            {
                // Regular rect, half height, to square the bottom borders
                QRect bottomRect = tempRect;
                bottomRect.setTop(bottomRect.top() + (bottomRect.height() / 2));
                backgroundPath.addRect(bottomRect);
            }
            
            // Regular rect, half height, to square the opposite border
            QRect squareRect = tempRect;
            if (isFirstColumn)
            {
                squareRect.setLeft(tempRect.left() + (tempRect.width() / 2));
            }
            else if (isLastColumn)
            {
                squareRect.setWidth(tempRect.width() / 2);
            }
            backgroundPath.addRect(squareRect);
        }
        else
        {
            backgroundPath.addRect(tempRect);
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(backgroundPath.simplified(), backgroundColor);
        painter->restore();
    }

    void PrefabUiHandler::PaintDescendantBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
        const QModelIndex& descendantIndex) const
    {
        if (!painter)
        {
            AZ_Warning("PrefabUiHandler", false, "PrefabUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());

        const QTreeView* outlinerTreeView(qobject_cast<const QTreeView*>(option.widget));
        const int ancestorLeft = outlinerTreeView->visualRect(index).left() + (m_prefabBorderThickness / 2) - 1;
        const int curveRectSize = m_prefabCapsuleRadius * 2;
        const bool isFirstColumn = descendantIndex.column() == EntityOutlinerListModel::ColumnName;
        const bool isLastColumn = descendantIndex.column() == EntityOutlinerListModel::ColumnLockToggle;

        // There is no legal way of opening prefabs in their default state, so default to disabled.
        QColor borderColor = m_prefabCapsuleDisabledColor;
        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            borderColor = m_prefabCapsuleEditColor;
        }

        QPen borderLinePen(borderColor, m_prefabBorderThickness);

        // Find the rect that extends fully to the left
        QRect fullRect = option.rect;
        fullRect.setLeft(ancestorLeft);
        if (isLastColumn)
        {
            fullRect.setWidth(fullRect.width() - 1);
        }

        // Adjust option.rect to account for the border thickness
        QRect rect = option.rect;
        rect.setLeft(rect.left() + (m_prefabBorderThickness / 2));

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(borderLinePen);

        if (IsLastVisibleChild(index, descendantIndex))
        {
            // This is the last visible entity in the prefab, so close the container

            if (isFirstColumn)
            {
                // Left border, curve on the bottom left, bottom border

                // Define curve start, end and size
                QPoint curveStart = fullRect.bottomLeft();
                curveStart.setY(curveStart.y() - m_prefabCapsuleRadius);
                QPoint curveEnd = fullRect.bottomLeft();
                curveEnd.setX(curveEnd.x() + m_prefabCapsuleRadius);
                QRect curveRect = QRect(fullRect.left(), fullRect.bottom() - curveRectSize, curveRectSize, curveRectSize);

                // Curved Corner
                QPainterPath curvedCorner;
                curvedCorner.moveTo(fullRect.topLeft());
                curvedCorner.lineTo(curveStart);
                curvedCorner.arcTo(curveRect, 180, 90);
                curvedCorner.lineTo(fullRect.bottomRight());
                painter->drawPath(curvedCorner);

            }
            else if (isLastColumn)
            {
                // Right border, curve on the bottom right, bottom border

                // Define curve start, end and size
                QPoint curveStart = fullRect.bottomRight();
                curveStart.setY(curveStart.y() - m_prefabCapsuleRadius);
                QRect curveRect = QRect(fullRect.right() - curveRectSize, fullRect.bottom() - curveRectSize, curveRectSize, curveRectSize);

                // Curved Corner
                QPainterPath curvedCorner;
                curvedCorner.moveTo(fullRect.topRight());
                curvedCorner.lineTo(curveStart);
                curvedCorner.arcTo(curveRect, 0, -90);
                curvedCorner.lineTo(rect.bottomLeft());
                painter->drawPath(curvedCorner);
            }
            else
            {
                // Bottom Border
                painter->drawLine(rect.bottomLeft(), rect.bottomRight());
            }
        }
        else
        {
            if (isFirstColumn)
            {
                // Left Border
                painter->drawLine(fullRect.topLeft(), fullRect.bottomLeft());
            }

            if (isLastColumn)
            {
                // Right Border
                painter->drawLine(fullRect.topRight(), fullRect.bottomRight());
            }
        }

        painter->restore();
    }

    void PrefabUiHandler::PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
    {
        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        const QPoint offset = QPoint(-18, 3);
        QModelIndex firstColumnIndex = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        const int iconSize = 16;
        const bool isHovered = (option.state & QStyle::State_MouseOver);
        const bool isSelected = index.data(EntityOutlinerListModel::SelectedRole).template value<bool>();
        const bool isFirstColumn = index.column() == EntityOutlinerListModel::ColumnName;
        const bool isExpanded =
            firstColumnIndex.data(EntityOutlinerListModel::ExpandedRole).value<bool>() &&
            firstColumnIndex.model()->hasChildren(firstColumnIndex);

        if (!isFirstColumn || !(option.state & QStyle::State_Enabled))
        {
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            // Only show the close icon if the prefab is expanded.
            // This allows the prefab container to be opened if it was collapsed during propagation.
            if (!isExpanded)
            {
                return;
            }

            // Use the same color as the background.
            QColor backgroundColor = m_backgroundColor;
            if (isSelected)
            {
                backgroundColor = m_backgroundSelectedColor;
            }
            else if (isHovered)
            {
                backgroundColor = m_backgroundHoverColor;
            }

            // Paint a rect to cover up the expander.
            QRect rect = QRect(0, 0, 16, 16);
            rect.translate(option.rect.topLeft() + offset);
            painter->fillRect(rect, backgroundColor);

            // Paint the icon.
            QIcon closeIcon = QIcon(m_prefabEditCloseIconPath);
            painter->drawPixmap(option.rect.topLeft() + offset, closeIcon.pixmap(iconSize));
        }
        else
        {
            // Only show the edit icon on hover.
            if (!isHovered)
            {
                return;
            }

            QIcon openIcon = QIcon(m_prefabEditOpenIconPath);
            painter->drawPixmap(option.rect.topLeft() + offset, openIcon.pixmap(iconSize));
        }

        painter->restore();
    }

    bool PrefabUiHandler::IsLastVisibleChild(const QModelIndex& parent, const QModelIndex& child)
    {
        QModelIndex lastVisibleItemIndex = GetLastVisibleChild(parent);
        QModelIndex index = child;

        // GetLastVisibleChild returns an index set to the ColumnName column
        if (index.column() != EntityOutlinerListModel::ColumnName)
        {
            index = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        }

        return index == lastVisibleItemIndex;
    }

    QModelIndex PrefabUiHandler::GetLastVisibleChild(const QModelIndex& parent)
    {
        auto model = parent.model();
        QModelIndex index = parent;

        // The parenting information for the index are stored in the ColumnName column
        if (index.column() != EntityOutlinerListModel::ColumnName)
        {
            index = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        }

        return Internal_GetLastVisibleChild(model, index);
    }

    QModelIndex PrefabUiHandler::Internal_GetLastVisibleChild(const QAbstractItemModel* model, const QModelIndex& index)
    {
        if (!model->hasChildren(index) || !index.data(EntityOutlinerListModel::ExpandedRole).value<bool>())
        {
            return index;
        }

        int childCount = index.data(EntityOutlinerListModel::ChildCountRole).value<int>();
        QModelIndex lastChild = model->index(childCount - 1, EntityOutlinerListModel::ColumnName, index);

        return Internal_GetLastVisibleChild(model, lastChild);
    }

    bool PrefabUiHandler::OnOutlinerItemClick(const QPoint& position, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        const QPoint offset = QPoint(-18, 3);

        if (m_prefabFocusPublicInterface->IsOwningPrefabInFocusHierarchy(entityId))
        {
            QRect iconRect = QRect(0, 0, 16, 16);
            iconRect.translate(option.rect.topLeft() + offset);

            if (iconRect.contains(position))
            {
                if (!m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
                {
                    // Focus on this prefab.
                    m_prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
                }

                // Don't propagate event.
                return true;
            }
        }

        return false;
    }

    void PrefabUiHandler::OnOutlinerItemCollapse(const QModelIndex& index) const
    {
        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());

        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            auto editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            // Go one level up.
            int length = m_prefabFocusPublicInterface->GetPrefabFocusPathLength(editorEntityContextId);
            m_prefabFocusPublicInterface->FocusOnPathIndex(editorEntityContextId, length - 2);
        }
    }

    bool PrefabUiHandler::OnEntityDoubleClick(AZ::EntityId entityId) const
    {
        // Focus on this prefab
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);

        // Don't propagate event.
        return true;
    }
}
