/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    class ContainerEntityInterface;

    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabOverridePublicInterface;
        class PrefabPublicInterface;
    }; // namespace Prefab

    class PrefabUiHandler : public EditorEntityUiHandlerBase
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabUiHandler, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::PrefabUiHandler, "{598154A2-89E3-45CB-A3CB-337CB1C73DE7}", EditorEntityUiHandlerBase);

        PrefabUiHandler();
        ~PrefabUiHandler() override = default;

        // EditorEntityUiHandler overrides ...
        QString GenerateItemInfoString(AZ::EntityId entityId) const override;
        QString GenerateItemTooltip(AZ::EntityId entityId) const override;
        QIcon GenerateItemIcon(AZ::EntityId entityId) const override;
        bool CanToggleLockVisibility(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantBackground(
            QPainter* painter,
            const QStyleOptionViewItem& option,
            const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;
        void PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantForeground(
            QPainter* painter,
            const QStyleOptionViewItem& option,
            const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;
        bool OnOutlinerItemClick(const QPoint& position, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool OnOutlinerItemDoubleClick(const QModelIndex& index) const override;
        void OnOutlinerItemCollapse(const QModelIndex& index) const override;

    protected:
        ContainerEntityInterface* m_containerEntityInterface = nullptr;
        Prefab::PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;
        Prefab::PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;

        static bool IsLastVisibleChild(const QModelIndex& parent, const QModelIndex& child);
        static QModelIndex GetLastVisibleChild(const QModelIndex& parent);
        static QModelIndex Internal_GetLastVisibleChild(const QAbstractItemModel* model, const QModelIndex& index);

        const QPixmap& GetOverrideImageForEntity(AZ::EntityId entityId) const;

        void PaintDescendantBorder(
            QPainter* painter,
            const QStyleOptionViewItem& option,
            const QModelIndex& index,
            const QModelIndex& descendantIndex,
            const QColor borderColor) const;

        static AzFramework::EntityContextId s_editorEntityContextId;

        QColor m_backgroundColor = QColor("#444444");
        QColor m_backgroundHoverColor = QColor("#5A5A5A");
        QColor m_backgroundSelectedColor = QColor("#656565");
        QColor m_prefabCapsuleColor = QColor("#1E252F");
        QColor m_prefabCapsuleDisabledColor = QColor("#35383C");
        QColor m_prefabCapsuleEditColor = QColor("#4A90E2");
        QString m_prefabIconPath = QString(":/Entity/prefab.svg");
        QString m_prefabEditIconPath = QString(":/Entity/prefab_edit.svg");
        QString m_prefabEditOpenIconPath = QString(":/Entity/prefab_edit_open.svg");
        QString m_prefabEditCloseIconPath = QString(":/Entity/prefab_edit_close.svg");

        inline static const QPoint s_overrideImageOffset = QPoint(13, 12);
        inline static const QSize s_overrideImageSize = QSize(7, 7);
        QString m_editEntityOverrideImagePath = QString(":/Entity/entity_modified_as_override.svg");
        QString m_addEntityOverrideImagePath = QString(":/Entity/entity_added_as_override.svg");
        QPixmap m_editEntityOverrideImage;
        QPixmap m_addEntityOverrideImage;
    };
} // namespace AzToolsFramework
