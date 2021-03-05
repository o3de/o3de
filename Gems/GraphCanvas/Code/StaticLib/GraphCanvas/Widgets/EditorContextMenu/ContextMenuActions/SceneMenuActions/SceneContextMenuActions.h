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

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}