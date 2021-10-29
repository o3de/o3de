/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

namespace AzToolsFramework
{

    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabPublicInterface;
    };

    class PrefabUiHandler
        : public EditorEntityUiHandlerBase
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabUiHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::PrefabUiHandler, "{598154A2-89E3-45CB-A3CB-337CB1C73DE7}", EditorEntityUiHandlerBase);

        PrefabUiHandler();
        ~PrefabUiHandler() override = default;

        // EditorEntityUiHandler overrides ...
        QString GenerateItemInfoString(AZ::EntityId entityId) const override;
        QString GenerateItemTooltip(AZ::EntityId entityId) const override;
        QIcon GenerateItemIcon(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;
        void PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool OnOutlinerItemClick(const QPoint& position, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void OnOutlinerItemCollapse(const QModelIndex& index) const override;
        bool OnEntityDoubleClick(AZ::EntityId entityId) const override;

    private:
        Prefab::PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;

        static bool IsLastVisibleChild(const QModelIndex& parent, const QModelIndex& child);
        static QModelIndex GetLastVisibleChild(const QModelIndex& parent);
        static QModelIndex Internal_GetLastVisibleChild(const QAbstractItemModel* model, const QModelIndex& index);

        static constexpr int m_prefabCapsuleRadius = 6;
        static constexpr int m_prefabBorderThickness = 2;
        static const QColor m_backgroundColor;
        static const QColor m_backgroundHoverColor;
        static const QColor m_backgroundSelectedColor;
        static const QColor m_prefabCapsuleColor;
        static const QColor m_prefabCapsuleDisabledColor;
        static const QColor m_prefabCapsuleEditColor;
        static const QString m_prefabIconPath;
        static const QString m_prefabEditIconPath;
        static const QString m_prefabEditOpenIconPath;
        static const QString m_prefabEditCloseIconPath;
    };
} // namespace AzToolsFramework
