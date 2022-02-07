/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuAction.h>

namespace GraphCanvas
{
    class RemoveUnusedElementsMenuAction
        : public SceneContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoveUnusedElementsMenuAction, AZ::SystemAllocator, 0);
        
        RemoveUnusedElementsMenuAction(QObject* parent);
        virtual ~RemoveUnusedElementsMenuAction() = default;

        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        using SceneContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class RemoveUnusedNodesMenuAction
        : public SceneContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoveUnusedNodesMenuAction, AZ::SystemAllocator, 0);

        RemoveUnusedNodesMenuAction(QObject* parent);
        virtual ~RemoveUnusedNodesMenuAction() = default;

        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        using SceneContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}
