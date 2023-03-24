/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabUiHandler.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Utilities/ScreenUtilities.h>
#include <AzQtComponents/Utilities/TextUtilities.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>
#include <AzToolsFramework/UI/Prefab/Constants.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QTreeView>
#include <QtGui/private/qhighdpiscaling_p.h>

namespace AzToolsFramework
{
    static const QPoint EditIconOffset = { -18, 3 };

    AzFramework::EntityContextId PrefabUiHandler::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

    PrefabUiHandler::PrefabUiHandler()
    {
        m_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
        if (m_containerEntityInterface == nullptr)
        {
            AZ_Assert(false, "PrefabUiHandler - could not get ContainerEntityInterface on PrefabUiHandler construction.");
            return;
        }

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

        m_prefabOverridePublicInterface = AZ::Interface<Prefab::PrefabOverridePublicInterface>::Get();
        if (m_prefabOverridePublicInterface == nullptr)
        {
            AZ_Assert(false, "PrefabUiHandler - could not get PrefabOverridePublicInterface on PrefabUiHandler construction.");
            return;
        }

        // Cache override pixmaps. QIcon handles dpi scaling
        QIcon editOverrideIcon = QIcon(m_editEntityOverrideImagePath);
        m_editEntityOverrideImage = editOverrideIcon.pixmap(s_overrideImageSize);
        QIcon addOverrideIcon = QIcon(m_addEntityOverrideImagePath);
        m_addEntityOverrideImage = addOverrideIcon.pixmap(s_overrideImageSize);

        // Get EditorEntityContextId
        EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);
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

            infoString = QObject::tr("<table style=\"font-size: %4px;\"><tr><td>%1%2</td><td width=\"%3\"></td></tr></table>")
                             .arg(path.Filename().Native().data())
                             .arg(saveFlag)
                             .arg(PrefabUIConstants::prefabEditIconSize)
                             .arg(PrefabUIConstants::prefabFileNameFontSize);
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

        if (m_prefabFocusPublicInterface->IsOwningPrefabInFocusHierarchy(entityId))
        {
            return QIcon(m_prefabEditIconPath);
        }

        return QIcon(m_prefabIconPath);
    }

    bool PrefabUiHandler::CanToggleLockVisibility(AZ::EntityId entityId) const
    {
        return !m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId);
    }

    void PrefabUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (!painter)
        {
            AZ_Warning("PrefabUiHandler", false, "PrefabUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        AZ::EntityId entityId = GetEntityIdFromIndex(index);
        const bool isFirstColumn = index.column() == EntityOutlinerListModel::ColumnName;
        const bool isLastColumn = index.column() == EntityOutlinerListModel::ColumnLockToggle;
        QModelIndex firstColumnIndex = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        const bool hasVisibleChildren = firstColumnIndex.data(EntityOutlinerListModel::ExpandedRole).value<bool>() &&
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

        QRect columnRect = option.rect;
        columnRect.setTop(columnRect.top() + 1);

        if (!hasVisibleChildren)
        {
            columnRect.setBottom(columnRect.bottom() - 1);
        }

        QPainterPath backgroundPath;
        backgroundPath.setFillRule(Qt::WindingFill);

        if (isFirstColumn || isLastColumn)
        {
            if (isFirstColumn)
            {
                columnRect.setLeft(columnRect.left() - 1);
            }
            else if (isLastColumn)
            {
                columnRect.setWidth(columnRect.width() - 1);
            }

            // Rounded rect to have rounded borders on top
            backgroundPath.addRoundedRect(columnRect, PrefabUIConstants::prefabCapsuleRadius, PrefabUIConstants::prefabCapsuleRadius);

            if (hasVisibleChildren)
            {
                // Regular rect, half height, to square the bottom borders
                QRect bottomRect = columnRect;
                bottomRect.setTop(bottomRect.top() + (bottomRect.height() / 2));
                backgroundPath.addRect(bottomRect);
            }

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

    void PrefabUiHandler::PaintDescendantBackground(
        QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QModelIndex& descendantIndex) const
    {
        if (!painter)
        {
            AZ_Warning("PrefabUiHandler", false, "PrefabUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        // If the owning prefab is focused, the border will be painted in the foreground.
        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            return;
        }

        QColor borderColor = m_prefabCapsuleDisabledColor;
        if (m_prefabFocusPublicInterface->IsOwningPrefabInFocusHierarchy(entityId))
        {
            borderColor = m_prefabCapsuleColor;
        }

        PaintDescendantBorder(painter, option, index, descendantIndex, borderColor);
    }

    const QPixmap& PrefabUiHandler::GetOverrideImageForEntity(AZ::EntityId entityId) const
    {
        if (auto overrideType = m_prefabOverridePublicInterface->GetEntityOverrideType(entityId); overrideType.has_value())
        {
            if (overrideType == AzToolsFramework::Prefab::OverrideType::AddEntity)
            {
                return m_addEntityOverrideImage;
            }
        }
        return m_editEntityOverrideImage;
    }

    void PrefabUiHandler::PaintDescendantForeground(
        QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QModelIndex& descendantIndex) const
    {
        if (!painter)
        {
            AZ_Warning("PrefabUiHandler", false, "PrefabUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        if (descendantIndex.column() == EntityOutlinerListModel::ColumnName)
        {
            AZ::EntityId descendantEntityId = GetEntityIdFromIndex(descendantIndex);

            // If the entity is not in the focus hierarchy, we needn't add override visualization
            // as overrides are only shown from the current focused prefab.
            if (m_prefabFocusPublicInterface->IsOwningPrefabInFocusHierarchy(descendantEntityId))
            {
                // Container entities will always have overrides because they need to maintain unique positions in the scene.
                // We are skipping checking for overrides on container entities for this reason.
                if (!m_prefabPublicInterface->IsInstanceContainerEntity(descendantEntityId) &&
                    m_prefabOverridePublicInterface->AreOverridesPresent(descendantEntityId))
                {
                    painter->save();
                    painter->setPen(Qt::NoPen);

                    // Cover the right side of the entity icon's cube with the background color
                    QColor backgroundColor = m_backgroundColor;
                    const bool isHovered = (option.state & QStyle::State_MouseOver);
                    const bool isSelected = descendantIndex.data(EntityOutlinerListModel::SelectedRole).template value<bool>();
                    if (isSelected)
                    {
                        backgroundColor = m_backgroundSelectedColor;
                    }
                    else if (isHovered)
                    {
                        backgroundColor = m_backgroundHoverColor;
                    }

                    // Create a path to fill with the background color
                    constexpr int coveredAreaLength = 7;
                    const QPoint coveredAreaOffsetFromEntityIcon(11, 12);
                    const QPoint coveredAreaTopLeft = option.rect.topLeft() + coveredAreaOffsetFromEntityIcon;
                    const QPoint coveredAreaTopRight = coveredAreaTopLeft + QPoint(coveredAreaLength, -4);
                    const QPoint coveredAreaBottomLeft = QPoint(coveredAreaTopLeft.x(), coveredAreaTopLeft.y() + coveredAreaLength);
                    const QPoint coveredAreaBottomRight = QPoint(coveredAreaBottomLeft.x() + coveredAreaLength, coveredAreaBottomLeft.y());
                    QPainterPath coveredAreaPath;
                    coveredAreaPath.moveTo(coveredAreaTopLeft);
                    coveredAreaPath.lineTo(coveredAreaTopRight);
                    coveredAreaPath.lineTo(coveredAreaBottomRight);
                    coveredAreaPath.lineTo(coveredAreaBottomLeft);
                    coveredAreaPath.lineTo(coveredAreaTopLeft);
                    painter->setRenderHint(QPainter::Antialiasing, false);
                    painter->fillPath(coveredAreaPath, backgroundColor);

                    // Paint the override image
                    const QPixmap& overrideImage = GetOverrideImageForEntity(descendantEntityId);
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->drawPixmap(option.rect.topLeft() + s_overrideImageOffset, overrideImage);

                    painter->restore();
                }
            }
        }

        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        // If the owning prefab is not focused, the border will be painted in the background.
        if (!m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            return;
        }

        PaintDescendantBorder(painter, option, index, descendantIndex, m_prefabCapsuleEditColor);
    }

    void PrefabUiHandler::PaintDescendantBorder(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index,
        const QModelIndex& descendantIndex,
        const QColor borderColor) const
    {
        const QTreeView* outlinerTreeView(qobject_cast<const QTreeView*>(option.widget));
        const int ancestorLeft = outlinerTreeView->visualRect(index).left() + (PrefabUIConstants::prefabBorderThickness / 2) - 1;
        const int curveRectSize = PrefabUIConstants::prefabCapsuleRadius * 2;
        const bool isFirstColumn = descendantIndex.column() == EntityOutlinerListModel::ColumnName;
        const bool isLastColumn = descendantIndex.column() == EntityOutlinerListModel::ColumnLockToggle;

        QPen borderLinePen(borderColor, PrefabUIConstants::prefabBorderThickness);

        // Find the rect that extends fully to the left
        QRect fullRect = option.rect;
        fullRect.setLeft(ancestorLeft);
        if (isLastColumn)
        {
            fullRect.setWidth(fullRect.width() - 1);
        }

        // Adjust option.rect to account for the border thickness
        QRect rect = option.rect;
        rect.setLeft(rect.left() + (PrefabUIConstants::prefabBorderThickness / 2));

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
                curveStart.setY(curveStart.y() - PrefabUIConstants::prefabCapsuleRadius);
                QPoint curveEnd = fullRect.bottomLeft();
                curveEnd.setX(curveEnd.x() + PrefabUIConstants::prefabCapsuleRadius);
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
                curveStart.setY(curveStart.y() - PrefabUIConstants::prefabCapsuleRadius);
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

    void PrefabUiHandler::PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);
        QModelIndex firstColumnIndex = index.siblingAtColumn(EntityOutlinerListModel::ColumnName);
        const bool isHovered = (option.state & QStyle::State_MouseOver);
        const bool isSelected = index.data(EntityOutlinerListModel::SelectedRole).template value<bool>();
        const bool isFirstColumn = index.column() == EntityOutlinerListModel::ColumnName;
        const bool isExpanded = firstColumnIndex.data(EntityOutlinerListModel::ExpandedRole).value<bool>() &&
            firstColumnIndex.model()->hasChildren(firstColumnIndex);
        const bool noChild = !index.model()->hasChildren(index);

        bool isContainerOpen = m_containerEntityInterface->IsContainerOpen(entityId);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            if (isFirstColumn && (option.state & QStyle::State_Enabled))
            {
                // Only show the close icon if the prefab is expanded or empty.
                // This allows the prefab container to be opened if it was collapsed during propagation.
                if (isExpanded || noChild)
                {
                    // Use the same color as the background.
                    QColor editIconBackgroundColor = m_backgroundColor;
                    if (isSelected)
                    {
                        editIconBackgroundColor = m_backgroundSelectedColor;
                    }
                    else if (isHovered)
                    {
                        editIconBackgroundColor = m_backgroundHoverColor;
                    }

                    // Paint a rect to cover up the expander.
                    QRect rect = QRect(0, 0, 16, 16);
                    rect.translate(option.rect.topLeft() + EditIconOffset);
                    painter->fillRect(rect, editIconBackgroundColor);

                    // Paint the icon.
                    QIcon closeIcon = QIcon(m_prefabEditCloseIconPath);
                    painter->drawPixmap(option.rect.topLeft() + EditIconOffset, closeIcon.pixmap(PrefabUIConstants::prefabEditIconSize));
                }
            }
        }
        else
        {
            // Only show the edit icon on hover.
            if (isFirstColumn && isHovered && !isContainerOpen)
            {
                QIcon openIcon = QIcon(m_prefabEditOpenIconPath);
                painter->drawPixmap(option.rect.topRight() + EditIconOffset, openIcon.pixmap(PrefabUIConstants::prefabEditIconSize));
            }
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
        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        QRect expanderRect = QRect(0, 0, 16, 16);
        expanderRect.translate(option.rect.topLeft() + EditIconOffset);

        const QPoint textOffset = QPoint(0, 3);
        QRect filenameRect = QRect(0, 0, 12, 10);
        filenameRect.translate(option.rect.topRight() + EditIconOffset + textOffset);
        if (filenameRect.contains(position))
        {
            if (!m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
            {
                // Focus on this prefab.
                m_prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
            }

            // Don't propagate event.
            return true;
        }
        else if (expanderRect.contains(position))
        {
            if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
            {
                // Close this prefab and focus on the parent
                m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(s_editorEntityContextId);
            }

            // Don't propagate event.
            return true;
        }

        return false;
    }

    void PrefabUiHandler::OnOutlinerItemCollapse(const QModelIndex& index) const
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            // Close this prefab and focus on the parent.
            // This is deferred to the end of the message queue to allow Qt code to continue
            // working with unaltered tree indices after it has sent the "on collapsed" event.
            // Changing focus can remove/add tree rows which would invalidate any tree indices
            // used by Qt when returning from the handler
            QTimer::singleShot(0, [this] {
                m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(AzToolsFramework::PrefabUiHandler::s_editorEntityContextId);
            });
        }
    }

    bool PrefabUiHandler::OnOutlinerItemDoubleClick(const QModelIndex& index) const
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        if (!m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            // Focus on this prefab
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
        }
        else
        {
            // Close this prefab and focus on the parent
            m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(s_editorEntityContextId);
        }

        // Don't propagate event.
        return true;
    }
} // namespace AzToolsFramework
