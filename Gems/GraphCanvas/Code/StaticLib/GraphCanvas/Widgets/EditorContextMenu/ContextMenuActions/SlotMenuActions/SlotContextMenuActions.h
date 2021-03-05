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

        void RefreshAction() override;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class RemoveSlotMenuAction
        : public SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoveSlotMenuAction, AZ::SystemAllocator, 0);

        RemoveSlotMenuAction(QObject* parent);
        virtual ~RemoveSlotMenuAction() = default;

        void RefreshAction() override;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };


    class ClearConnectionsMenuAction
        : public SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ClearConnectionsMenuAction, AZ::SystemAllocator, 0);
        
        ClearConnectionsMenuAction(QObject* parent);
        virtual ~ClearConnectionsMenuAction() = default;

        void RefreshAction() override;
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

        void RefreshAction() override;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class ToggleReferenceStateAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ToggleReferenceStateAction, AZ::SystemAllocator, 0);

        ToggleReferenceStateAction(QObject* parent);
        virtual ~ToggleReferenceStateAction() = default;

        void RefreshAction() override;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class PromoteToVariableAction
        : public GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(PromoteToVariableAction, AZ::SystemAllocator, 0);

        PromoteToVariableAction(QObject* parent);
        virtual ~PromoteToVariableAction() = default;

        void RefreshAction() override;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

}