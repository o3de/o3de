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

// AZ
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// Qt
#include <QAction>

// GraphModel
#include <GraphModel/GraphModelBus.h>

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>
#include <Editor/Menus/NodeContextMenu.h>

namespace LandscapeCanvasEditor
{
    // Select the corresponding Entities in the Editor based on the selected nodes in our scene graph
    QAction* GetNodeSelectInEditorAction(const AZ::EntityId& sceneId, QObject* parent)
    {
        // Retrieve the selected nodes in our scene
        GraphModel::NodePtrList nodeList;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeList, sceneId, &GraphModelIntegration::GraphControllerRequests::GetSelectedNodes);

        // Iterate through the selected nodes to find their corresponding vegetation entities
        AzToolsFramework::EntityIdList vegetationEntityIdsToSelect;
        for (const auto& node : nodeList)
        {
            if (!node)
            {
                continue;
            }

            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            vegetationEntityIdsToSelect.push_back(baseNodePtr->GetVegetationEntityId());
        }

        QAction* action = new QAction({ vegetationEntityIdsToSelect.size() > 1 ? QObject::tr("Select Entities in Editor") : QObject::tr("Select Entity in Editor") }, parent);

        QString tooltip = QObject::tr("Select the corresponding Entity/Entities in the Editor for the selected node(s) in the graph");
        action->setToolTip(tooltip);
        action->setStatusTip(tooltip);

        QObject::connect(action,
            &QAction::triggered,
            [vegetationEntityIdsToSelect](bool)
        {
            // Set the new selection
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, vegetationEntityIdsToSelect);
        });

        return action;
    }

    NodeContextMenu::NodeContextMenu(const AZ::EntityId& sceneId, QWidget* parent)
        : GraphCanvas::NodeContextMenu(LandscapeCanvas::LANDSCAPE_CANVAS_EDITOR_ID, parent)
    {
        AddMenuAction(GetNodeSelectInEditorAction(sceneId, this));
    }

    void NodeContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::NodeContextMenu::OnRefreshActions(graphId, targetMemberId);

        // Don't allow cut/copy/paste/duplicate on our area extender nodes because they can't
        // exist without being wrapped on an area (e.g. spawner) node
        GraphModel::NodePtr node;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(node, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, targetMemberId);
        if (node)
        {
            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            if (baseNodePtr->IsAreaExtender())
            {
                m_editActionGroup.SetCopyEnabled(false);
                m_editActionGroup.SetCutEnabled(false);
                m_editActionGroup.SetDuplicateEnabled(false);
                m_editActionGroup.SetPasteEnabled(false);
            }
        }
    }
}
