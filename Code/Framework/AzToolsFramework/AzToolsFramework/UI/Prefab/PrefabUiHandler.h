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
#include <AzToolsFramework/UI/Prefab/PrefabEditInterface.h>

namespace AzToolsFramework
{
    class PrefabUiHandler
        : public EditorEntityUiHandlerBase
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabUiHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::PrefabUiHandler, "{598154A2-89E3-45CB-A3CB-337CB1C73DE7}", EditorEntityUiHandlerBase);

        PrefabUiHandler();
        ~PrefabUiHandler() override = default;

        // EditorEntityUiHandler...
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;

    private:
        static bool IsLastVisibleChild(const QModelIndex& parent, const QModelIndex& child);
        static QModelIndex GetLastVisibleChild(const QModelIndex& parent);
        static QModelIndex Internal_GetLastVisibleChild(const QAbstractItemModel* model, const QModelIndex& index);

        static constexpr int m_prefabCapsuleRadius = 6;
        static constexpr int m_prefabBorderThickness = 2;
        static const QColor m_prefabCapsuleColor;
        static const QColor m_prefabCapsuleEditColor;

        Prefab::PrefabEditInterface* m_prefabEditInterface = nullptr;
    };
} // namespace AzToolsFramework
