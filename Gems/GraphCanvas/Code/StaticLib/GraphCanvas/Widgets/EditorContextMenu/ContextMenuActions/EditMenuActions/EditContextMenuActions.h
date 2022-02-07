/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditContextMenuAction.h>

namespace GraphCanvas
{
    class CutGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CutGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        CutGraphSelectionMenuAction(QObject* parent);
        virtual ~CutGraphSelectionMenuAction() = default;

        using EditContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class CopyGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CopyGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        CopyGraphSelectionMenuAction(QObject* parent);
        virtual ~CopyGraphSelectionMenuAction() = default;

        using EditContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class PasteGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        using ContextMenuAction::RefreshAction;

        AZ_CLASS_ALLOCATOR(PasteGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        PasteGraphSelectionMenuAction(QObject* parent);
        virtual ~PasteGraphSelectionMenuAction() = default;

        void RefreshAction() override;

        using EditContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
    
    class DeleteGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(DeleteGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        DeleteGraphSelectionMenuAction(QObject* parent);
        virtual ~DeleteGraphSelectionMenuAction() = default;

        using EditContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class DuplicateGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(DuplicateGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        DuplicateGraphSelectionMenuAction(QObject* parent);
        virtual ~DuplicateGraphSelectionMenuAction() = default;

        using EditContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}
