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
    const QColor LevelRootUiHandler::s_levelRootBorderColor = QColor("#656565");
    const QString LevelRootUiHandler::s_levelRootIconPath = QString(":/Level/level.svg");

    LevelRootUiHandler::LevelRootUiHandler()
    {
        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        if (m_prefabPublicInterface == nullptr)
        {
            AZ_Assert(false, "LevelRootUiHandler - could not get PrefabPublicInterface on LevelRootUiHandler construction.");
            return;
        }
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

    void LevelRootUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
    {
        if (!painter)
        {
            AZ_Warning("LevelRootUiHandler", false, "LevelRootUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        QPen borderLinePen(s_levelRootBorderColor, s_levelRootBorderThickness);
        QRect rect = option.rect;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Draw border at the bottom
        painter->setPen(borderLinePen);
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());

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

        if (auto prefabFocusPublicInterface = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
            !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
        }

        // Don't propagate event.
        return true;
    }
}
