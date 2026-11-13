/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    // =======================================================================
    // Outliner Prefab Colours
    // =======================================================================

    //! The colours the Entity Outliner paints a prefab row with, named, so the legacy and the flattened GUI read one value. The level
    //! root row (LevelRootUiHandler.h) reads the capsule colours too. A named namespace, not an anonymous one: a header is read by many
    //! files, and a unity build joins files into one translation unit, where anonymous namespaces merge.
    namespace PrefabUiHandlerFlat
    {
        //! PrefabUiHandler.h:79. TOKENIZE: OutlinerPrefabBackground
        inline constexpr QColor Background(0x44, 0x44, 0x44);
        //! PrefabUiHandler.h:80. TOKENIZE: OutlinerPrefabBackgroundHover
        inline constexpr QColor BackgroundHover(0x5A, 0x5A, 0x5A);
        //! PrefabUiHandler.h:81. TOKENIZE: OutlinerPrefabBackgroundSelected
        inline constexpr QColor BackgroundSelected(0x65, 0x65, 0x65);
        //! PrefabUiHandler.h:82, LevelRootUiHandler.h:50. TOKENIZE: OutlinerPrefabCapsule
        inline constexpr QColor Capsule(0x1E, 0x25, 0x2F);
        //! PrefabUiHandler.h:83, LevelRootUiHandler.h:51. TOKENIZE: OutlinerPrefabCapsuleDisabled
        inline constexpr QColor CapsuleDisabled(0x35, 0x38, 0x3C);
        //! PrefabUiHandler.h:84, LevelRootUiHandler.h:52. TOKENIZE: OutlinerPrefabCapsuleEdit
        inline constexpr QColor CapsuleEdit(0x4A, 0x90, 0xE2);
    }

    class ContainerEntityInterface;

    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabOverridePublicInterface;
        class PrefabPublicInterface;
    }; // namespace Prefab

    class AZTF_API PrefabUiHandler : public EditorEntityUiHandlerBase
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

        QColor m_backgroundColor = PrefabUiHandlerFlat::Background;
        QColor m_backgroundHoverColor = PrefabUiHandlerFlat::BackgroundHover;
        QColor m_backgroundSelectedColor = PrefabUiHandlerFlat::BackgroundSelected;
        QColor m_prefabCapsuleColor = PrefabUiHandlerFlat::Capsule;
        QColor m_prefabCapsuleDisabledColor = PrefabUiHandlerFlat::CapsuleDisabled;
        QColor m_prefabCapsuleEditColor = PrefabUiHandlerFlat::CapsuleEdit;
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
