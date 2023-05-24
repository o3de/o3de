/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuAction.h>

namespace GraphCanvas
{
    class CreateNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeGroupMenuAction, AZ::SystemAllocator);

        CreateNodeGroupMenuAction(QObject* parent, bool collapseGroup);
        virtual ~CreateNodeGroupMenuAction() = default;

        using ContextMenuAction::RefreshAction;

        void RefreshAction() override;

        using NodeGroupContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

    private:
        bool m_collapseGroup;
    };

    class UngroupNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(UngroupNodeGroupMenuAction, AZ::SystemAllocator);

        UngroupNodeGroupMenuAction(QObject* parent);
        virtual ~UngroupNodeGroupMenuAction() = default;

        using ContextMenuAction::RefreshAction;

        void RefreshAction() override;

        using NodeGroupContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class CollapseNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CollapseNodeGroupMenuAction, AZ::SystemAllocator);

        CollapseNodeGroupMenuAction(QObject* parent);
        virtual ~CollapseNodeGroupMenuAction() = default;

        using ContextMenuAction::RefreshAction;

        void RefreshAction() override;

        using NodeGroupContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class ExpandNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ExpandNodeGroupMenuAction, AZ::SystemAllocator);

        ExpandNodeGroupMenuAction(QObject* parent);
        virtual ~ExpandNodeGroupMenuAction() = default;

        using ContextMenuAction::RefreshAction;

        void RefreshAction() override;

        using NodeGroupContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class EditGroupTitleMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(EditGroupTitleMenuAction, AZ::SystemAllocator);

        EditGroupTitleMenuAction(QObject* parent);
        virtual ~EditGroupTitleMenuAction() = default;

        using ContextMenuAction::RefreshAction;

        void RefreshAction() override;

        using NodeGroupContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}
