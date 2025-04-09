/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabPublicInterface;
    };

    class LevelRootUiHandler
        : public EditorEntityUiHandlerBase
    {
    public:
        AZ_CLASS_ALLOCATOR(LevelRootUiHandler, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::LevelRootUiHandler, "{B1D3B270-CD29-4033-873A-D78E76AB24A4}", EditorEntityUiHandlerBase);

        LevelRootUiHandler();
        ~LevelRootUiHandler() override = default;

        // EditorEntityUiHandler...
        QIcon GenerateItemIcon(AZ::EntityId entityId) const override;
        QString GenerateItemInfoString(AZ::EntityId entityId) const override;
        bool CanToggleLockVisibility(AZ::EntityId entityId) const override;
        bool CanRename(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const override;
        bool OnOutlinerItemClick(const QPoint& position, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool OnOutlinerItemDoubleClick(const QModelIndex& index) const override;

    private:
        Prefab::PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;

        QString m_levelRootIconPath = QString(":/Level/level.svg");
        QColor m_levelRootBorderColor = QColor("#656565");

        QColor m_prefabCapsuleColor = QColor("#1E252F");
        QColor m_prefabCapsuleDisabledColor = QColor("#35383C");
        QColor m_prefabCapsuleEditColor = QColor("#4A90E2");
    };
} // namespace AzToolsFramework
