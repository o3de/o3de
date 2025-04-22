/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructContextMenuAction.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Types/ConstructPresets.h>

namespace GraphCanvas
{
    class AddPresetMenuAction
        : public ConstructContextMenuAction
    {
    protected:
        AddPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset, AZStd::string_view subMenuPath);

    public:
        AZ_CLASS_ALLOCATOR(AddPresetMenuAction, AZ::SystemAllocator);
        
        virtual ~AddPresetMenuAction() = default;
        
        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        using ConstructContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

    private:

        virtual AZ::Entity* CreateEntityForPreset() const = 0;
        virtual void AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const = 0;        

        bool                               m_isInToolbar;
        AZStd::string                      m_subMenuPath;
        AZStd::shared_ptr<ConstructPreset> m_preset;
    };

    class ApplyPresetMenuAction
        : public ConstructContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ApplyPresetMenuAction, AZ::SystemAllocator);

        ApplyPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset, AZStd::string_view subMenuPath);
        virtual ~ApplyPresetMenuAction() = default;

        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        using ConstructContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

    private:

        AZStd::string                      m_subMenuPath;
        AZStd::shared_ptr<ConstructPreset> m_preset;
    };

    class CreatePresetFromSelection
        : public ContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreatePresetFromSelection, AZ::SystemAllocator);

        CreatePresetFromSelection(QObject* parent = nullptr);
        virtual ~CreatePresetFromSelection();

        using ContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

        static ActionGroupId GetCreateConstructContextMenuActionGroupId()
        {
            return AZ_CRC_CE("CreateConstructActionGroup");
        }

        ActionGroupId GetActionGroupId() const override
        {
            return GetCreateConstructContextMenuActionGroupId();
        }
    };

    class PresetsMenuActionGroup
        : public AssetEditorPresetNotificationBus::Handler
    {
    protected:
        PresetsMenuActionGroup(ConstructType constructType);

    public:
        ~PresetsMenuActionGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);
        void RefreshPresets();

        virtual ConstructContextMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) = 0;

        // PresetsMenuActionGroup
        void OnPresetsChanged() override;
        void OnConstructPresetsChanged(ConstructType constructType) override;
        ////

        void SetEnabled(bool enabled);

    private:

        AZStd::unordered_set< QMenu* > m_menus;
        AZStd::unordered_set< AZStd::string > m_subMenus;

        EditorContextMenu* m_contextMenu;
        ConstructType m_constructType;

        bool m_isDirty;
    };

    class ApplyPresetMenuActionGroup
        : public PresetsMenuActionGroup
    {
    protected:
        ApplyPresetMenuActionGroup(ConstructType constructType);

    public:
        ~ApplyPresetMenuActionGroup();

        void RefreshActionGroup(const GraphId& graphId, const AZ::EntityId& targetId);

        // PresetsMenuActionGroup
        ConstructContextMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) override;
        ////
    };

    ////////////////////
    // Comment Presets
    ////////////////////

    class AddCommentPresetMenuAction
        : public AddPresetMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddCommentPresetMenuAction, AZ::SystemAllocator);
        AddCommentPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset);

    private:
        AZ::Entity* CreateEntityForPreset() const override;
        void AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const override;
    };

    class CreateCommentPresetMenuActionGroup
        : public PresetsMenuActionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateCommentPresetMenuActionGroup, AZ::SystemAllocator);

        CreateCommentPresetMenuActionGroup();

        ConstructContextMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) override;
    };

    class ApplyCommentPresetMenuActionGroup
        : public ApplyPresetMenuActionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(ApplyCommentPresetMenuActionGroup, AZ::SystemAllocator);

        ApplyCommentPresetMenuActionGroup()
            : ApplyPresetMenuActionGroup(ConstructType::CommentNode)
        {
        }
    };

    ///////////////////////
    // Node Group Presets
    ///////////////////////

    class AddNodeGroupPresetMenuAction
        : public AddPresetMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddNodeGroupPresetMenuAction, AZ::SystemAllocator);
        AddNodeGroupPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset);

    private:
        AZ::Entity* CreateEntityForPreset() const override;
        void AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const override;
    };

    class CreateNodeGroupPresetMenuActionGroup
        : public PresetsMenuActionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeGroupPresetMenuActionGroup, AZ::SystemAllocator);

        CreateNodeGroupPresetMenuActionGroup();

        ConstructContextMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) override;
    };

    class ApplyNodeGroupPresetMenuActionGroup
        : public ApplyPresetMenuActionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(ApplyNodeGroupPresetMenuActionGroup, AZ::SystemAllocator);

        ApplyNodeGroupPresetMenuActionGroup()
            : ApplyPresetMenuActionGroup(ConstructType::NodeGroup)
        {
        }
    };
}
