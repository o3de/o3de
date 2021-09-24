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
        class PrefabPublicInterface;
    };

    class LevelRootUiHandler
        : public EditorEntityUiHandlerBase
    {
    public:
        AZ_CLASS_ALLOCATOR(LevelRootUiHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::LevelRootUiHandler, "{B1D3B270-CD29-4033-873A-D78E76AB24A4}", EditorEntityUiHandlerBase);

        LevelRootUiHandler();
        ~LevelRootUiHandler() override = default;

        // EditorEntityUiHandler...
        QIcon GenerateItemIcon(AZ::EntityId entityId) const override;
        QString GenerateItemInfoString(AZ::EntityId entityId) const override;
        bool CanToggleLockVisibility(AZ::EntityId entityId) const override;
        bool CanRename(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;

        static constexpr int m_levelRootBorderThickness = 1;
        static const QColor m_levelRootBorderColor;
        static const QString m_levelRootIconPath;

    };
} // namespace AzToolsFramework
