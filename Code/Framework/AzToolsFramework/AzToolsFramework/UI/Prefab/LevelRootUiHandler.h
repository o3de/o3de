/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabEditInterface;
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
        QPixmap GenerateItemIcon(AZ::EntityId entityId) const override;
        bool CanToggleLockVisibility(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        Prefab::PrefabEditInterface* m_prefabEditInterface = nullptr;
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;

        static constexpr int m_levelRootBorderThickness = 1;
        static const QColor m_levelRootBorderColor;
        static const QString m_levelRootIconPath;

    };
} // namespace AzToolsFramework
