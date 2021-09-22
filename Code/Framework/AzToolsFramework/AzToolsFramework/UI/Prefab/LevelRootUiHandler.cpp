/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/LevelRootUiHandler.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPainterPath>
#include <QTreeView>

namespace AzToolsFramework
{
    const QColor LevelRootUiHandler::m_levelRootBorderColor = QColor("#656565");
    const QString LevelRootUiHandler::m_levelRootIconPath = QString(":/Level/level.svg");

    LevelRootUiHandler::LevelRootUiHandler()
    {
        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();

        if (m_prefabPublicInterface == nullptr)
        {
            AZ_Assert(false, "LevelRootUiHandler - could not get PrefabPublicInterface on LevelRootUiHandler construction.");
            return;
        }
    }

    QIcon LevelRootUiHandler::GenerateItemIcon(AZ::EntityId /*entityId*/) const
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

    bool LevelRootUiHandler::CanToggleLockVisibility(AZ::EntityId /*entityId*/) const
    {
        return false;
    }

    bool LevelRootUiHandler::CanRename(AZ::EntityId /*entityId*/) const
    {
        return false;
    }

    void LevelRootUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
    {
        if (!painter)
        {
            AZ_Warning("LevelRootUiHandler", false, "LevelRootUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        QPen borderLinePen(m_levelRootBorderColor, m_levelRootBorderThickness);

        QRect rect = option.rect;
        rect.setLeft(rect.left() + (m_levelRootBorderThickness / 2));

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(borderLinePen);
        
        // Draw border at the bottom
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        painter->restore();
    }
}
