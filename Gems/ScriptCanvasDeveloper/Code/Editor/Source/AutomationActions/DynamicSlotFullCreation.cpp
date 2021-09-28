/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QMenu>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>

#include <ScriptCanvasDeveloperEditor/AutomationActions/DynamicSlotFullCreation.h>
#include <ScriptCanvasDeveloperEditor/DeveloperUtils.h>

namespace ScriptCanvasDeveloperEditor
{
    namespace DynamicSlotFullCreation
    {
        class DynamicSlotFullCreationInterface
            : public ProcessNodePaletteInterface
        {
        public:

            DynamicSlotFullCreationInterface(DeveloperUtils::ConnectionStyle connectionStyle)
            {
                m_chainConfig.m_connectionStyle = connectionStyle;
                m_chainConfig.m_skipHandlers = true;
            }
            virtual ~DynamicSlotFullCreationInterface() = default;

            void SetupInterface(const AZ::EntityId& activeGraphCanvasGraphId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId) override
            {
                m_graphCanvasGraphId = activeGraphCanvasGraphId;
                m_scriptCanvasId = scriptCanvasId;

                GraphCanvas::SceneRequestBus::EventResult(m_viewId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);
                GraphCanvas::SceneRequestBus::EventResult(m_gridId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

                GraphCanvas::GridRequestBus::EventResult(m_minorPitch, m_gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                QGraphicsScene* graphicsScene = nullptr;
                GraphCanvas::SceneRequestBus::EventResult(graphicsScene, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::AsQGraphicsScene);

                if (graphicsScene)
                {
                    QRectF sceneArea = graphicsScene->sceneRect();
                    sceneArea.adjust(m_minorPitch.GetX(), m_minorPitch.GetY(), -m_minorPitch.GetX(), -m_minorPitch.GetY());
                    GraphCanvas::ViewRequestBus::Event(m_viewId, &GraphCanvas::ViewRequests::CenterOnArea, sceneArea);
                    QApplication::processEvents();
                }

                GraphCanvas::ViewRequestBus::EventResult(m_nodeCreationPos, m_viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

                GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
                GraphCanvas::ViewRequestBus::EventResult(graphicsView, m_viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

                m_viewportRectangle = graphicsView->mapToScene(graphicsView->viewport()->geometry()).boundingRect();

                ScriptCanvas::GraphRequestBus::EventResult(m_graph, m_scriptCanvasId, &ScriptCanvas::GraphRequests::GetGraph);

                ScriptCanvas::GraphVariableManagerRequests* variableRequests = ScriptCanvas::GraphVariableManagerRequestBus::FindFirstHandler(m_scriptCanvasId);

                if (variableRequests)
                {
                    const ScriptCanvas::GraphVariableMapping* mapping = variableRequests->GetVariables();

                    AZStd::unordered_set< ScriptCanvas::Data::Type > dataTypes;

                    for (const auto& variablePair : (*mapping))
                    {
                        auto insertResult = dataTypes.insert(variablePair.second.GetDataType());

                        if (insertResult.second)
                        {
                            m_availableVariableIds.emplace_back(variablePair.first);
                            m_variableTypeMapping[variablePair.first] = variablePair.second.GetDataType();
                        }
                    }
                }

                // Temporary work around until the extra automation tools can be merged over that have better ways of doing this.
                const GraphCanvas::GraphCanvasTreeItem* treeItem = nullptr;
                ScriptCanvasEditor::AutomationRequestBus::BroadcastResult(treeItem, &ScriptCanvasEditor::AutomationRequests::GetNodePaletteRoot);

                const GraphCanvas::NodePaletteTreeItem* onGraphStartItem = nullptr;

                if (treeItem)
                {
                    AZStd::unordered_set< const GraphCanvas::GraphCanvasTreeItem* > unexploredSet = { treeItem };

                    while (!unexploredSet.empty())
                    {
                        const GraphCanvas::GraphCanvasTreeItem* treeItem2 = (*unexploredSet.begin());
                        unexploredSet.erase(unexploredSet.begin());

                        const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem = azrtti_cast<const GraphCanvas::NodePaletteTreeItem*>(treeItem2);

                        if (nodePaletteTreeItem && nodePaletteTreeItem->GetName().compare("On Graph Start") == 0)
                        {
                            onGraphStartItem = nodePaletteTreeItem;
                            break;
                        }

                        for (int i = 0; i < treeItem2->GetChildCount(); ++i)
                        {
                            const GraphCanvas::GraphCanvasTreeItem* childItem = treeItem2->FindChildByRow(i);

                            if (childItem)
                            {
                                unexploredSet.insert(childItem);
                            }
                        }
                    }
                }

                if (onGraphStartItem)
                {
                    GraphCanvas::GraphCanvasMimeEvent* mimeEvent = onGraphStartItem->CreateMimeEvent();

                    if (mimeEvent)
                    {
                        ScriptCanvasEditor::NodeIdPair createdPair = DeveloperUtils::HandleMimeEvent(mimeEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);
                        DeveloperUtils::CreateConnectedChain(createdPair, m_chainConfig);
                    }
                }
            }

            bool ShouldProcessItem([[maybe_unused]] const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) const override
            {
                return !m_availableVariableIds.empty();
            }

            void ProcessItem(const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) override
            {
                AZStd::unordered_set<ScriptCanvasEditor::NodeIdPair> createdPairs;
                GraphCanvas::GraphCanvasMimeEvent* mimeEvent = nodePaletteTreeItem->CreateMimeEvent();

                AZStd::unordered_set< AZ::EntityId > nodesToDelete;

                if (ScriptCanvasEditor::MultiCreateNodeMimeEvent* multiCreateMimeEvent = azrtti_cast<ScriptCanvasEditor::MultiCreateNodeMimeEvent*>(mimeEvent))
                {
                    AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > mimeEvents = multiCreateMimeEvent->CreateMimeEvents();                    

                    for (GraphCanvas::GraphCanvasMimeEvent* currentEvent : mimeEvents)
                    {
                        ScriptCanvasEditor::NodeIdPair processPair = ProcessMimeEvent(currentEvent);

                        nodesToDelete.insert(GraphCanvas::GraphUtils::FindOutermostNode(processPair.m_graphCanvasId));

                        delete currentEvent;
                    }
                }
                else if (mimeEvent)
                {
                    ScriptCanvasEditor::NodeIdPair processPair = ProcessMimeEvent(mimeEvent);

                    nodesToDelete.insert(GraphCanvas::GraphUtils::FindOutermostNode(processPair.m_graphCanvasId));
                }

                delete mimeEvent;

                GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, nodesToDelete);
            }

        private:

            bool HasDynamicSlots(ScriptCanvasEditor::NodeIdPair creationPair) const
            {
                ScriptCanvas::Node* node = m_graph->FindNode(creationPair.m_scriptCanvasId);

                if (node)
                {
                    for (const ScriptCanvas::Slot* slot : node->GetAllSlots())
                    {
                        if (slot->IsDynamicSlot())
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            void PopulateSlots(GraphCanvas::GraphCanvasMimeEvent* currentEvent, const ScriptCanvasEditor::NodeIdPair& prototypeNode)
            {
                int origWidth = m_widthOffset;
                int origHeight = m_heightOffset;
                int origRowHeight = m_maxRowHeight;

                // Creating enough data to do a brute-force lexographical ordering of all availble
                // variable types to all slots. In every combination.
                //
                // Create a mapping of available types to each slot.
                // Along with an order or the slots.
                // Will consume data from the outermost slot, and as it empties, will continue to consume downward.
                // With each empty consumption triggering a refilling of all the after slots.
                AZStd::vector<ScriptCanvas::SlotId> slotOrdering;
                AZStd::unordered_map<ScriptCanvas::SlotId, AZStd::vector<ScriptCanvas::VariableId> > groupDataTypes;
                AZStd::unordered_set<AZ::Crc32> usedSlotGroups;

                AZStd::unordered_map<ScriptCanvas::SlotId, ScriptCanvas::TransientSlotIdentifier> prototypeIdentifiers;

                int tempWidth = m_widthOffset;
                int tempHeight = m_heightOffset;
                int tempRowHeight = m_maxRowHeight;

                // Generate a prototype node which will we scrape for data.
                ScriptCanvas::Node* protoTypeNode = m_graph->FindNode(prototypeNode.m_scriptCanvasId);

                for (const ScriptCanvas::Slot* slot : protoTypeNode->GetAllSlots())
                {
                    prototypeIdentifiers[slot->GetId()] = slot->GetTransientIdentifier();

                    if (slot->IsDynamicSlot())
                    {
                        AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

                        if (usedSlotGroups.find(dynamicGroup) != usedSlotGroups.end())
                        {
                            continue;
                        }

                        slotOrdering.emplace_back(slot->GetId());
                        groupDataTypes.insert_key(slot->GetId());

                        if (dynamicGroup != AZ::Crc32())
                        {
                            usedSlotGroups.insert(dynamicGroup);
                        }
                    }
                }


                // Populate all of the groups with all the data types they need to have.
                for (auto& keyPair : groupDataTypes)
                {
                    keyPair.second = m_availableVariableIds;
                }

                AZStd::vector<AZ::EntityId> usedGraphCanvasNodeIds;

                // Construct a single node to use, until we need to swap it out for a new one.
                // Then we can just delete this node at the end.
                auto nodeIdPair = DeveloperUtils::HandleMimeEvent(currentEvent, m_graphCanvasGraphId, m_viewportRectangle, tempWidth, tempHeight, tempRowHeight, m_minorPitch);
                ScriptCanvas::Node* node = m_graph->FindNode(nodeIdPair.m_scriptCanvasId);

                // Begin brute force lexical permutations!
                while (node)
                {
                    int i = 0;

                    // Attempt to assign the current set of variable types.
                    for (; i < slotOrdering.size(); ++i)
                    {
                        auto groupDataIter = groupDataTypes.find(slotOrdering[i]);

                        auto transientSlotIter = prototypeIdentifiers.find(slotOrdering[i]);

                        if (transientSlotIter != prototypeIdentifiers.end())
                        {
                            ScriptCanvas::TransientSlotIdentifier slotIdentifier = transientSlotIter->second;

                            ScriptCanvas::Slot* slot = node->GetSlotByTransientId(slotIdentifier);

                            const ScriptCanvas::VariableId nextVariableId = groupDataIter->second.front();

                            auto variableTypeIter = m_variableTypeMapping.find(nextVariableId);

                            if (variableTypeIter == m_variableTypeMapping.end() || !slot->IsTypeMatchFor(variableTypeIter->second))
                            {
                                break;
                            }

                            if (!slot->IsVariableReference())
                            {
                                GraphCanvas::Endpoint endpoint = ConvertToGraphCanvasEndpoint(slot->GetEndpoint());

                                bool canConvertToReference = false;
                                GraphCanvas::DataSlotRequestBus::EventResult(canConvertToReference, endpoint.GetSlotId(), &GraphCanvas::DataSlotRequests::CanConvertToReference);

                                if (canConvertToReference)
                                {
                                    GraphCanvas::DataSlotRequestBus::Event(endpoint.GetSlotId(), &GraphCanvas::DataSlotRequests::ConvertToReference);
                                }
                            }

                            if (!slot->IsVariableReference())
                            {
                                break;
                            }

                            node->SetSlotVariableId(slot->GetId(), nextVariableId);
                        }
                    }

                    // If we succeeded, we need to update our offsets, and create a new node.
                    if (i >= slotOrdering.size())
                    {
                        DeveloperUtils::CreateConnectedChain(nodeIdPair, m_chainConfig);
                        usedGraphCanvasNodeIds.emplace_back(nodeIdPair.m_graphCanvasId);

                        DeveloperUtils::UpdateViewportPositionOffsetForNode(nodeIdPair.m_graphCanvasId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);

                        tempWidth = m_widthOffset;
                        tempHeight = m_heightOffset;
                        tempRowHeight = m_maxRowHeight;

                        nodeIdPair = DeveloperUtils::HandleMimeEvent(currentEvent, m_graphCanvasGraphId, m_viewportRectangle, tempWidth, tempHeight, tempRowHeight, m_minorPitch);
                        node = m_graph->FindNode(nodeIdPair.m_scriptCanvasId);
                        i = aznumeric_cast<int>(slotOrdering.size()) - 1;
                    }
                    else
                    {
                        // Reset all of the previous variable ids if we failed.
                        for (int r = 0; r < i; ++r)
                        {
                            auto transientSlotIter = prototypeIdentifiers.find(slotOrdering[r]);

                            if (transientSlotIter != prototypeIdentifiers.end())
                            {
                                ScriptCanvas::TransientSlotIdentifier slotIdentifier = transientSlotIter->second;
                                ScriptCanvas::Slot* slot = node->GetSlotByTransientId(slotIdentifier);

                                node->ClearSlotVariableId(slot->GetId());
                            }
                        }

                        tempWidth = m_widthOffset;
                        tempHeight = m_heightOffset;
                        tempRowHeight = m_maxRowHeight;
                    }

                    // Determine where we failed, and remove that value
                    // Updating everything after it to the new state.
                    //
                    // i.e. if we failed on the first element. We can drop values off of it.
                    // Then reset everything after it to the full 'state' allowing us to skip
                    // large swathes of invalid configurations.
                    while (i < slotOrdering.size())
                    {
                        const ScriptCanvas::SlotId& indexPoint = slotOrdering[i];

                        auto groupDataIter = groupDataTypes.find(indexPoint);

                        if (groupDataIter != groupDataTypes.end())
                        {
                            groupDataIter->second.erase(groupDataIter->second.begin());

                            if (groupDataIter->second.empty())
                            {
                                --i;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    // If we rolled off the front node. We are done.
                    // Otherwise we need to reset elements.
                    if (i >= 0)
                    {
                        i++;

                        while (i < slotOrdering.size())
                        {
                            const ScriptCanvas::SlotId& indexPoint = slotOrdering[i];

                            auto groupDataIter = groupDataTypes.find(indexPoint);

                            if (groupDataIter != groupDataTypes.end())
                            {
                                groupDataIter->second = m_availableVariableIds;
                            }

                            i++;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                if (!usedGraphCanvasNodeIds.empty())
                {
                    // Create a group to create some visual chunking that'll look nice.
                    //
                    // Need to force one group per element for visual chunkiness.
                    // Each group will also represent the end of a chunk for the row(otherwise weird overlap issues could visually occur)
                    AZ::EntityId groupId = GraphCanvas::GraphUtils::CreateGroupForElements(m_graphCanvasGraphId, usedGraphCanvasNodeIds);
                    GraphCanvas::CommentRequestBus::Event(groupId, &GraphCanvas::CommentRequests::SetComment, "New Group");

                    if (groupId.IsValid())
                    {
                        tempWidth = origWidth;
                        tempHeight = origHeight;
                        tempRowHeight = origRowHeight;

                        DeveloperUtils::UpdateViewportPositionOffsetForNode(groupId, m_viewportRectangle, tempWidth, tempHeight, tempRowHeight, m_minorPitch);

                        // If we got kicked down to a new row. Nothing left to do.
                        if (tempRowHeight <= 0)
                        {
                            m_widthOffset = tempWidth;
                            m_heightOffset = tempHeight;
                            m_maxRowHeight = tempRowHeight;
                        }
                        else
                        {
                            m_widthOffset = 0;
                            m_heightOffset = origHeight + tempRowHeight + aznumeric_cast<int>(m_minorPitch.GetY());
                            m_maxRowHeight = 0;
                        }

                        GraphCanvas::CommentRequestBus::Event(groupId, &GraphCanvas::CommentRequests::SetComment, protoTypeNode->GetDebugName().c_str());
                    }
                }

                AZStd::unordered_set< AZ::EntityId > deleteSet = {
                    GraphCanvas::GraphUtils::FindOutermostNode(nodeIdPair.m_graphCanvasId)
                };

                GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, deleteSet);
            }

            ScriptCanvasEditor::NodeIdPair ProcessMimeEvent(GraphCanvas::GraphCanvasMimeEvent* currentEvent)
            {
                int tempWidth = m_widthOffset;
                int tempHeight = m_heightOffset;
                int tempRowHeight = m_maxRowHeight;

                auto nodeIdPair = DeveloperUtils::HandleMimeEvent(currentEvent, m_graphCanvasGraphId, m_viewportRectangle, tempWidth, tempHeight, tempRowHeight, m_minorPitch);

                if (HasDynamicSlots(nodeIdPair))
                {
                    PopulateSlots(currentEvent, nodeIdPair);
                }

                return nodeIdPair;
            }

            GraphCanvas::Endpoint ConvertToGraphCanvasEndpoint(const ScriptCanvas::Endpoint& endpoint)
            {
                GraphCanvas::Endpoint graphCanvasEndpoint;

                ScriptCanvasEditor::SlotMappingRequestBus::EventResult(graphCanvasEndpoint.m_slotId, endpoint.GetNodeId(), &ScriptCanvasEditor::SlotMappingRequests::MapToGraphCanvasId, endpoint.GetSlotId());
                GraphCanvas::SlotRequestBus::EventResult(graphCanvasEndpoint.m_nodeId, graphCanvasEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetNode);

                return graphCanvasEndpoint;
            }

            AZ::EntityId m_graphCanvasGraphId;
            ScriptCanvas::ScriptCanvasId m_scriptCanvasId;

            AZ::Vector2 m_nodeCreationPos = AZ::Vector2::CreateZero();

            AZ::EntityId m_viewId;
            AZ::EntityId m_gridId;

            AZ::Vector2 m_minorPitch = AZ::Vector2::CreateZero();

            QRectF m_viewportRectangle;

            int m_widthOffset = 0;
            int m_heightOffset = 0;

            int m_maxRowHeight = 0;

            ScriptCanvas::Graph* m_graph = nullptr;

            DeveloperUtils::CreateConnectedChainConfig m_chainConfig;

            AZStd::vector<ScriptCanvas::VariableId> m_availableVariableIds;
            AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::Data::Type> m_variableTypeMapping;
        };

        void VariablePaletteFullCreationAction()
        {
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);

            DynamicSlotFullCreationInterface fullCreationInterface(DeveloperUtils::NoConnections);
            DeveloperUtils::ProcessNodePalette(fullCreationInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        void VariablePaletteFullyConnectionCreationAction()
        {
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);

            DynamicSlotFullCreationInterface fullCreationInterface(DeveloperUtils::SingleExecutionConnection);
            DeveloperUtils::ProcessNodePalette(fullCreationInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        QAction* CreateDynamicSlotFullCreationAction(QMenu* mainMenu)
        {
            QAction* dynamicSlotFullCreationAction = nullptr;

            if (mainMenu)
            {
                mainMenu->addSeparator();

                {
                    dynamicSlotFullCreationAction = mainMenu->addAction(QAction::tr("Mass Populate Dynamic Nodes"));
                    dynamicSlotFullCreationAction->setAutoRepeat(false);
                    dynamicSlotFullCreationAction->setToolTip("Tries to create every node in the node palette with dynamic slots.\nAnd will generate variations of the node with variables assigned to each slot in each combination depending on what variables are available in the Variable Manager.");
                    dynamicSlotFullCreationAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Shift+k", "Debug|Create Variable Palette")));

                    QObject::connect(dynamicSlotFullCreationAction, &QAction::triggered, &VariablePaletteFullCreationAction);
                }

                {
                    dynamicSlotFullCreationAction = mainMenu->addAction(QAction::tr("Mass Populate and Connect Dynamic Nodes"));
                    dynamicSlotFullCreationAction->setAutoRepeat(false);
                    dynamicSlotFullCreationAction->setToolTip("Tries to create and connect every node in the node palette with dynamic slots.\nAnd will generate variations of the node with variables assigned to each slot in each combination depending on what variables are available in the Variable Manager.");

                    QObject::connect(dynamicSlotFullCreationAction, &QAction::triggered, &VariablePaletteFullyConnectionCreationAction);
                }
            }

            return dynamicSlotFullCreationAction;
        }
    }
}
