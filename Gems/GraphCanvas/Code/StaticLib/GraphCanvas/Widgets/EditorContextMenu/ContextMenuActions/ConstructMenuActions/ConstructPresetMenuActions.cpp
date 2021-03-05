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
#include <QInputDialog>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    ////////////////////////
    // AddPresetMenuAction
    ////////////////////////

    static const size_t s_maximumDisplaySize = 150;

    AddPresetMenuAction::AddPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset, AZStd::string_view subMenuPath)
        : ConstructContextMenuAction("Add Preset", contextMenu)
        , m_subMenuPath(subMenuPath)
        , m_preset(preset)
    {
        if (preset->GetDisplayName().size() > s_maximumDisplaySize)
        {
            AZStd::string substring = preset->GetDisplayName();
            substring = substring.substr(0, s_maximumDisplaySize - 3);

            substring.append("...");

            setText(substring.c_str());
        }
        else
        {
            setText(preset->GetDisplayName().c_str());
        }

        QPixmap* pixmap = preset->GetDisplayIcon(contextMenu->GetEditorId());

        if (pixmap)
        {
            setIcon(QIcon((*pixmap)));
        }

        m_isInToolbar = contextMenu->IsToolBarMenu();
    }

    bool AddPresetMenuAction::IsInSubMenu() const
    {
        return !m_isInToolbar;
    }

    AZStd::string AddPresetMenuAction::GetSubMenuPath() const
    {
        return m_subMenuPath;
    }

    ContextMenuAction::SceneReaction AddPresetMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        AZ::Entity* graphCanvasEntity = CreateEntityForPreset();
        
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Preset Entity");

        if (graphCanvasEntity)
        {
            if (m_preset)
            {
                m_preset->ApplyPreset(graphCanvasEntity->GetId());
            }

            AddEntityToGraph(graphId, graphCanvasEntity, scenePos);
        }

        if (graphCanvasEntity != nullptr)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }

    //////////////////////////////
    // CreatePresetFromSelection
    //////////////////////////////

    CreatePresetFromSelection::CreatePresetFromSelection(QObject* parent)
        : ContextMenuAction("Create Preset From", parent)
    {
    }

    CreatePresetFromSelection::~CreatePresetFromSelection()
    {
    }

    ContextMenuAction::SceneReaction CreatePresetFromSelection::TriggerAction([[maybe_unused]] const AZ::Vector2& scenePos)
    {
        bool acceptText = true;

        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();

        EditorId editorId = GetEditorId();

        ViewId viewId;
        SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

        GraphCanvasGraphicsView* graphicsView = nullptr;
        ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

        QString presetName;

        if (graphicsView)
        {
            while (true)
            {
                presetName = QInputDialog::getText(graphicsView, tr("Set Preset Name"), tr("Preset Name"), QLineEdit::Normal, "", &acceptText);

                if (acceptText)
                {
                    if (!presetName.isEmpty())
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        if (presetName.isEmpty())
        {
            return SceneReaction::Nothing;
        }

        EditorConstructPresets* presets = nullptr;
        AssetEditorSettingsRequestBus::EventResult(presets, editorId, &AssetEditorSettingsRequests::GetConstructPresets);

        if (presets)
        {
            presets->CreatePresetFrom(targetId, presetName.toUtf8().data());
        }

        return SceneReaction::Nothing;
    }

    ///////////////////////////
    // PresetsMenuActionGroup
    ///////////////////////////

    PresetsMenuActionGroup::PresetsMenuActionGroup(ConstructType constructType)
        : m_contextMenu(nullptr)
        , m_constructType(constructType)
        , m_isDirty(false)
    {
    }

    PresetsMenuActionGroup::~PresetsMenuActionGroup()
    {
    }

    void PresetsMenuActionGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        m_contextMenu = contextMenu;
        AssetEditorPresetNotificationBus::Handler::BusConnect(m_contextMenu->GetEditorId());

        m_isDirty = true;
        RefreshPresets();
    }

    void PresetsMenuActionGroup::RefreshPresets()
    {
        if (m_isDirty && m_contextMenu)
        {
            // Bit of a hack right now.
            // Need to rework the base menu a little bit more to make it
            // more dynamic. For now I'll just bypass most of the underlying logic
            // and treat it like a normal QMenu.
            bool isFinalized = m_contextMenu->IsFinalized();
            
            if (isFinalized)
            {
                if (m_contextMenu->IsToolBarMenu())
                {
                    m_contextMenu->clear();
                }
                else
                {
                    for (const AZStd::string& subMenu : m_subMenus)
                    {
                        QMenu* menu = m_contextMenu->FindSubMenu(subMenu);

                        // Remove all of the previous preset actions to avoid needing to figure out what
                        // actually changed.
                        if (menu)
                        {
                            menu->clear();
                        }
                    }
                }
            }

            const ConstructTypePresetBucket* presetBucket = nullptr;
            AssetEditorSettingsRequestBus::EventResult(presetBucket, m_contextMenu->GetEditorId(), &AssetEditorSettingsRequests::GetConstructTypePresetBucket, m_constructType);

            if (presetBucket)
            {
                for (auto preset : presetBucket->GetPresets())
                {
                    AddPresetMenuAction* menuAction = CreatePresetMenuAction(m_contextMenu, preset);

                    if (isFinalized)
                    {
                        if (m_contextMenu->IsToolBarMenu())
                        {
                            m_contextMenu->addAction(menuAction);
                        }
                        else
                        {
                            QMenu* menu = m_contextMenu->FindSubMenu(menuAction->GetSubMenuPath());
                            menu->addAction(menuAction);

                            m_menus.insert(menu);
                        }
                    }
                    else
                    {
                        m_subMenus.insert(menuAction->GetSubMenuPath());
                        m_contextMenu->AddMenuAction(menuAction);
                    }
                }
            }

            m_isDirty = false;
        }
    }

    void PresetsMenuActionGroup::OnPresetsChanged()
    {
        m_isDirty = true;
    }

    void PresetsMenuActionGroup::OnConstructPresetsChanged(ConstructType constructType)
    {
        if (m_constructType == constructType)
        {
            m_isDirty = true;
        }
    }

    void PresetsMenuActionGroup::SetEnabled(bool enabled)
    {
        if (m_contextMenu->IsToolBarMenu())
        {
            m_contextMenu->setEnabled(false);
        }
        else
        {
            for (QMenu* menu : m_menus)
            {
                menu->setEnabled(enabled);
            }

            for (AZStd::string subMenu : m_subMenus)
            {
                QMenu* menu = m_contextMenu->FindSubMenu(subMenu);
                menu->setEnabled(enabled);
            }
        }
    }

    ///////////////////////////////
    // AddCommentPresetMenuAction
    ///////////////////////////////

    AddCommentPresetMenuAction::AddCommentPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
        : AddPresetMenuAction(contextMenu, preset, "Add Comment")
    {
    }

    AZ::Entity* AddCommentPresetMenuAction::CreateEntityForPreset() const
    {
        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvasRequests::CreateCommentNodeAndActivate);

        return graphCanvasEntity;
    }

    void AddCommentPresetMenuAction::AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const
    {
        SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);
        SceneRequestBus::Event(graphId, &SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos);

        if (IsInSubMenu())
        {
            CommentUIRequestBus::Event(graphCanvasEntity->GetId(), &CommentUIRequests::SetEditable, true);
        }
        else
        {
            CommentRequestBus::Event(graphCanvasEntity->GetId(), &CommentRequests::SetComment, "New Comment");
            SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &SceneMemberUIRequests::SetSelected, true);
        }
    }

    //////////////////////////////////
    // CommentPresetsMenuActionGroup
    //////////////////////////////////

    CommentPresetsMenuActionGroup::CommentPresetsMenuActionGroup()
        : PresetsMenuActionGroup(ConstructType::CommentNode)
    {

    }

    AddPresetMenuAction* CommentPresetsMenuActionGroup::CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
    {
        return aznew AddCommentPresetMenuAction(contextMenu, preset);
    }

    /////////////////////////////////
    // AddNodeGroupPresetMenuAction
    /////////////////////////////////

    AddNodeGroupPresetMenuAction::AddNodeGroupPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
        : AddPresetMenuAction(contextMenu, preset, "Group")
    {
    }

    AZ::Entity* AddNodeGroupPresetMenuAction::CreateEntityForPreset() const
    {
        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvasRequests::CreateNodeGroupAndActivate);

        return graphCanvasEntity;
    }

    void AddNodeGroupPresetMenuAction::AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const
    {
        SceneRequestBus::Event(graphId, &SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos);

        AZStd::vector< AZ::EntityId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

        GraphCanvas::GraphUtils::ResizeGroupToElements(graphCanvasEntity->GetId(), selectedNodes);

        SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);

        CommentRequestBus::Event(graphCanvasEntity->GetId(), &CommentRequests::SetComment, "New Group");

        if (IsInSubMenu())
        {
            CommentUIRequestBus::Event(graphCanvasEntity->GetId(), &CommentUIRequests::SetEditable, true);
        }
        else
        {
            SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &SceneMemberUIRequests::SetSelected, true);
        }
    }

    ////////////////////////////////////
    // NodeGroupPresetsMenuActionGroup
    ////////////////////////////////////

    NodeGroupPresetsMenuActionGroup::NodeGroupPresetsMenuActionGroup()
        : PresetsMenuActionGroup(ConstructType::NodeGroup)
    {
    }

    AddPresetMenuAction* NodeGroupPresetsMenuActionGroup::CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
    {
        return aznew AddNodeGroupPresetMenuAction(contextMenu, preset);
    }
}
