/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuAction.h>

namespace GraphCanvas
{
    class AddSlotMenuAction
        : public SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddSlotMenuAction, AZ::SystemAllocator, 0);

        AddSlotMenuAction(QObject* parent);
        virtual ~AddSlotMenuAction() = default;

        using SlotContextMenuAction::RefreshAction;
        void RefreshAction() override;

        using SlotContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class RemoveSlotMenuAction
        : public SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoveSlotMenuAction, AZ::SystemAllocator, 0);

        RemoveSlotMenuAction(QObject* parent);
        virtual ~RemoveSlotMenuAction() = default;

        using SlotContextMenuAction::RefreshAction;
        void RefreshAction() override;

        using SlotContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };


    class ClearConnectionsMenuAction
        : public SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ClearConnectionsMenuAction, AZ::SystemAllocator, 0);
        
        ClearConnectionsMenuAction(QObject* parent);
        virtual ~ClearConnectionsMenuAction() = default;

        using SlotContextMenuAction::RefreshAction;
        void RefreshAction() override;

        using SlotContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    //////////////////////
    // Data Slot Actions
    //////////////////////

    class ResetToDefaultValueMenuAction
        : public SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ResetToDefaultValueMenuAction, AZ::SystemAllocator, 0);

        ResetToDefaultValueMenuAction(QObject* parent);
        virtual ~ResetToDefaultValueMenuAction() = default;

        using SlotContextMenuAction::RefreshAction;
        void RefreshAction() override;

        using SlotContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class ToggleReferenceStateAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ToggleReferenceStateAction, AZ::SystemAllocator, 0);

        ToggleReferenceStateAction(QObject* parent);
        virtual ~ToggleReferenceStateAction() = default;

        using SlotContextMenuAction::RefreshAction;
        void RefreshAction() override;

        using SlotContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class PromoteToVariableAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(PromoteToVariableAction, AZ::SystemAllocator, 0);

        PromoteToVariableAction(QObject* parent);
        virtual ~PromoteToVariableAction() = default;

        using SlotContextMenuAction::RefreshAction;
        void RefreshAction() override;

        using SlotContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

}
