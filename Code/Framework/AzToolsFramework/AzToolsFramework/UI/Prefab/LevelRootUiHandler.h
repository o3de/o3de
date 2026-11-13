/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>
#include <AzToolsFramework/UI/Prefab/PrefabUiHandler.h>

namespace AzToolsFramework
{
    // =======================================================================
    // Outliner Level Root Colours
    // =======================================================================

    //! The colour the Entity Outliner paints the level root row's divider with, named, so the legacy and the flattened GUI read one
    //! value. The capsule colours are the prefab row's (PrefabUiHandlerFlat, PrefabUiHandler.h). A named namespace, not an anonymous one:
    //! a header is read by many files, and a unity build joins files into one translation unit, where anonymous namespaces merge.
    namespace LevelRootUiHandlerFlat
    {
        //! LevelRootUiHandler.h:48. TOKENIZE: OutlinerLevelRootBorder
        inline constexpr QColor Border(0x65, 0x65, 0x65);
    }

    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabPublicInterface;
    };

    class AZTF_API LevelRootUiHandler
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
        QColor m_levelRootBorderColor = LevelRootUiHandlerFlat::Border;

        QColor m_prefabCapsuleColor = PrefabUiHandlerFlat::Capsule;
        QColor m_prefabCapsuleDisabledColor = PrefabUiHandlerFlat::CapsuleDisabled;
        QColor m_prefabCapsuleEditColor = PrefabUiHandlerFlat::CapsuleEdit;
    };
} // namespace AzToolsFramework
