/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/GraphAutomation.h>

#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/utility/charconv.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>
#include <Editor/View/Windows/MainWindow.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/ConnectionBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvasEditor
{
    namespace
    {
        // A palette ID is a tree path followed by a factory choice, such as "/2/0/5#1": the row of the entry at each
        // level below the palette root, then the index of one of the node factories that entry offers.

        //! Precedes the row at each level of a palette tree path.
        constexpr char PaletteTreeSeparator = '/';

        //! Separates a palette tree path from its factory choice.
        constexpr char PaletteChoiceSeparator = '#';

        //! Parses text that holds only a decimal number.
        template<typename Number>
        bool ParseWholeNumber(AZStd::string_view text, Number& value)
        {
            const char* const end = text.data() + text.size();
            const auto parsed = AZStd::from_chars(text.data(), end, value);
            return parsed.ec == AZStd::errc{} && parsed.ptr == end;
        }

        //! Returns the palette tree item that a palette tree path names.
        AZ::Outcome<const GraphCanvas::GraphCanvasTreeItem*, AZStd::string> FindPaletteItem(
            const GraphCanvas::GraphCanvasTreeItem* root, AZStd::string_view treePath)
        {
            const GraphCanvas::GraphCanvasTreeItem* item = root;
            if (treePath.empty())
            {
                return AZ::Success(item);
            }

            // Every row follows a separator, so a path neither starts without one nor ends with one.
            if (treePath.front() != PaletteTreeSeparator || treePath.back() == PaletteTreeSeparator)
            {
                return AZ::Failure(AZStd::string("Invalid palette tree path"));
            }

            AZStd::string_view rows = treePath.substr(1);
            while (item)
            {
                const auto rowText = AZ::StringFunc::TokenizeNext(rows, PaletteTreeSeparator);
                if (!rowText)
                {
                    break;
                }

                int row = -1;
                if (!ParseWholeNumber(*rowText, row) || row < 0 || row >= item->GetChildCount())
                {
                    return AZ::Failure(AZStd::string("Palette identifier is invalid or stale"));
                }

                item = item->FindChildByRow(row);
            }

            return AZ::Success(item);
        }
    } // namespace

    class GraphAutomation::GraphEdit
    {
    public:
        GraphEdit(MainWindow& owner, AZStd::string_view label)
            : m_owner(owner)
            , m_editorBatch(AZStd::string(label).c_str())
            , m_graphBatch(label)
        {
            m_owner.PushPreventUndoStateUpdate();
        }

        ~GraphEdit()
        {
            m_owner.PopPreventUndoStateUpdate();
            m_owner.PostUndoPoint(m_owner.GetActiveScriptCanvasId());
            m_owner.SignalSceneDirty(m_owner.m_activeGraph);
        }

    private:
        MainWindow& m_owner;
        AzToolsFramework::ScopedUndoBatch m_editorBatch;
        ScriptCanvasEditor::ScopedUndoBatch m_graphBatch;
    };

    AutomationValueMap GraphAutomation::DescribeNode(ScriptCanvas::Node& node, AZ::EntityId graph)
    {
        AutomationValueList slotDescriptions;
        for (const auto& slot : node.GetSlots())
        {
            const ScriptCanvas::Endpoint endpoint(node.GetEntityId(), slot.GetId());
            GraphCanvas::Endpoint canvasEndpoint;
            EditorGraphRequestBus::EventResult(canvasEndpoint, graph, &EditorGraphRequests::ConvertToGraphCanvasEndpoint, endpoint);

            AutomationValueMap entry{ { "id", slot.GetId().ToString() },
                                      { "name", slot.GetName() },
                                      { "direction", AZStd::string(slot.IsInput() ? "in" : "out") },
                                      { "kind", AZStd::string(slot.IsExecution() ? "execution" : "data") },
                                      { "graphCanvasNode", IdText(canvasEndpoint.GetNodeId()) },
                                      { "graphCanvasSlot", IdText(canvasEndpoint.GetSlotId()) },
                                      { "type", ScriptCanvas::Data::ToAZType(slot.GetDataType()).ToString<AZStd::string>() },
                                      { "typeName", ScriptCanvas::Data::GetName(slot.GetDataType()) } };

            if (const auto* datum = node.FindDatum(slot.GetId()))
            {
                entry.emplace("value", datum->ToAny());
            }

            slotDescriptions.push_back(AZStd::move(entry));
        }

        AZ::EntityId canvas;
        SceneMemberMappingRequestBus::EventResult(canvas, node.GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        return { { "id", IdText(node.GetEntityId()) },
                 { "graphCanvas", IdText(canvas) },
                 { "name", node.GetNodeName() },
                 { "type", node.RTTI_GetType().ToString<AZStd::string>() },
                 { "slots", AZStd::move(slotDescriptions) } };
    }

    AutomationResult GraphAutomation::Describe(const AZStd::string& graph)
    {
        // Bounds on one description, so a huge graph fails instead of producing an unbounded result.
        static constexpr size_t MaxDescribedNodes = 4096;
        static constexpr size_t MaxDescribedConnections = 8192;

        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        const AZ::EntityId graphId = EntityId(graph);

        AZStd::vector<AZ::EntityId> nodeIds;
        ScriptCanvas::GraphRequestBus::EventResult(nodeIds, graphId, &ScriptCanvas::GraphRequests::GetNodes);

        AZStd::vector<AZ::EntityId> connectionIds;
        ScriptCanvas::GraphRequestBus::EventResult(connectionIds, graphId, &ScriptCanvas::GraphRequests::GetConnections);

        if (nodeIds.size() > MaxDescribedNodes || connectionIds.size() > MaxDescribedConnections)
        {
            return AZ::Failure(AZStd::string("Graph exceeds the bounded automation description limit"));
        }

        AutomationValueList nodes;
        for (const AZ::EntityId id : nodeIds)
        {
            ScriptCanvas::Node* node = nullptr;
            ScriptCanvas::GraphRequestBus::EventResult(node, graphId, &ScriptCanvas::GraphRequests::FindNode, id);
            if (!node)
            {
                return AZ::Failure(AZStd::string("Native graph contains an unavailable node"));
            }

            nodes.push_back(DescribeNode(*node, graphId));
        }

        AutomationValueList connections;
        for (const AZ::EntityId id : connectionIds)
        {
            ScriptCanvas::Endpoint source;
            ScriptCanvas::ConnectionRequestBus::EventResult(source, id, &ScriptCanvas::ConnectionRequests::GetSourceEndpoint);

            ScriptCanvas::Endpoint target;
            ScriptCanvas::ConnectionRequestBus::EventResult(target, id, &ScriptCanvas::ConnectionRequests::GetTargetEndpoint);

            AZ::EntityId canvas;
            SceneMemberMappingRequestBus::EventResult(canvas, id, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

            connections.push_back({ { "id", IdText(id) },
                                    { "graphCanvas", IdText(canvas) },
                                    { "sourceNode", IdText(source.GetNodeId()) },
                                    { "sourceSlot", source.GetSlotId().ToString() },
                                    { "targetNode", IdText(target.GetNodeId()) },
                                    { "targetSlot", target.GetSlotId().ToString() } });
        }

        AutomationValueList variables;
        const ScriptCanvas::GraphVariableMapping* mapping = nullptr;
        ScriptCanvas::GraphRequestBus::EventResult(mapping, graphId, &ScriptCanvas::GraphRequests::GetVariables);
        if (mapping)
        {
            for (const auto& [id, variable] : *mapping)
            {
                variables.push_back({ { "id", id.ToString() },
                                      { "name", AZStd::string(variable.GetVariableName()) },
                                      { "type", ScriptCanvas::Data::ToAZType(variable.GetDataType()).ToString<AZStd::string>() },
                                      { "value", variable.GetDatum()->ToAny() } });
            }
        }

        AutomationValueMap result = Identity(*window.GetValue());
        result.emplace("nodes", AZStd::move(nodes));
        result.emplace("connections", AZStd::move(connections));
        result.emplace("variables", AZStd::move(variables));
        return AZ::Success(AZStd::move(result));
    }

    GraphAutomation::PaletteChoices GraphAutomation::ExpandPaletteChoices(AZStd::unique_ptr<GraphCanvas::GraphCanvasMimeEvent> event)
    {
        PaletteChoices choices;
        if (const auto* multi = azrtti_cast<const MultiCreateNodeMimeEvent*>(event.get()))
        {
            // These are the choices the editor offers in a menu, such as the send, receive, get and set nodes of an event.
            for (GraphCanvas::GraphCanvasMimeEvent* choice : multi->CreateMimeEvents())
            {
                choices.emplace_back(choice);
            }
        }
        else if (event)
        {
            choices.push_back(AZStd::move(event));
        }

        return choices;
    }

    AutomationResult GraphAutomation::Palette(const AZStd::string& query, AZ::u32 after, AZ::u32 limit)
    {
        static constexpr AZ::u32 MaxPalettePageSize = 256;

        auto window = Window();
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        if (limit == 0 || limit > MaxPalettePageSize)
        {
            return AZ::Failure(AZStd::string::format("Palette page limit must be 1..%u", MaxPalettePageSize));
        }

        const GraphCanvas::GraphCanvasTreeItem* root = window.GetValue()->GetNodePaletteRoot();
        if (!root)
        {
            return AZ::Failure(AZStd::string("Native node palette is unavailable"));
        }

        AutomationValueList entries;
        AZ::u32 matched = 0;

        // Visits the palette tree depth first. key is the item's palette tree path, and parent is the display path
        // of its parent, which the query is matched against.
        AZStd::function<void(const GraphCanvas::GraphCanvasTreeItem*, const AZStd::string&, const AZStd::string&)> visit;
        visit = [&](const GraphCanvas::GraphCanvasTreeItem* item, const AZStd::string& key, const AZStd::string& parent)
        {
            const auto* paletteItem = azrtti_cast<const GraphCanvas::NodePaletteTreeItem*>(item);
            const AZStd::string name = paletteItem ? AZStd::string(paletteItem->GetName().toUtf8().constData()) : AZStd::string();
            const AZStd::string path = parent + PaletteTreeSeparator + name;

            constexpr bool caseSensitive = false;
            if (AZ::StringFunc::Contains(path, query, caseSensitive))
            {
                PaletteChoices choices =
                    ExpandPaletteChoices(AZStd::unique_ptr<GraphCanvas::GraphCanvasMimeEvent>(item->CreateMimeEvent()));
                if (!paletteItem || paletteItem->IsEnabled())
                {
                    for (size_t index = 0; index < choices.size(); ++index)
                    {
                        if (!choices[index])
                        {
                            continue;
                        }

                        // Every choice counts toward the total; only those on the requested page are returned.
                        const AZ::u32 position = matched;
                        ++matched;
                        if (position < after || entries.size() >= limit)
                        {
                            continue;
                        }

                        entries.push_back({ { "id", key + PaletteChoiceSeparator + AZStd::to_string(index) },
                                            { "name", name },
                                            { "path", path },
                                            { "factory", AZStd::string(choices[index]->RTTI_GetTypeName()) } });
                    }
                }
            }

            for (int row = 0; row < item->GetChildCount(); ++row)
            {
                visit(item->FindChildByRow(row), key + PaletteTreeSeparator + AZStd::to_string(row), path);
            }
        };
        visit(root, "", "");

        // The next page starts after the entries returned here, or at the end once every match has been returned.
        const AZ::u32 next = after >= matched ? matched : after + static_cast<AZ::u32>(entries.size());

        return AZ::Success(AutomationValueMap{ { "entries", AZStd::move(entries) }, { "total", matched }, { "next", next } });
    }

    AutomationResult GraphAutomation::AddNode(const AZStd::string& graph, const AZStd::string& key, const AZ::Vector2& position)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        if (!position.IsFinite())
        {
            return AZ::Failure(AZStd::string("Node position must be finite"));
        }

        MainWindow& owner = *window.GetValue();

        // Split the palette ID into its tree path and factory choice.
        const size_t separator = key.find(PaletteChoiceSeparator);
        if (separator == AZStd::string::npos)
        {
            return AZ::Failure(AZStd::string("Use a complete palette id including its factory choice"));
        }

        size_t choiceIndex = 0;
        if (!ParseWholeNumber(AZStd::string_view(key).substr(separator + 1), choiceIndex))
        {
            return AZ::Failure(AZStd::string("Invalid palette factory choice"));
        }

        const auto found = FindPaletteItem(owner.GetNodePaletteRoot(), AZStd::string_view(key).substr(0, separator));
        if (!found)
        {
            return AZ::Failure(found.GetError());
        }

        const GraphCanvas::GraphCanvasTreeItem* treeItem = found.GetValue();
        PaletteChoices choices =
            ExpandPaletteChoices(AZStd::unique_ptr<GraphCanvas::GraphCanvasMimeEvent>(treeItem ? treeItem->CreateMimeEvent() : nullptr));
        const auto* paletteItem = azrtti_cast<const GraphCanvas::NodePaletteTreeItem*>(treeItem);
        if (choiceIndex >= choices.size() || !choices[choiceIndex] || (paletteItem && !paletteItem->IsEnabled()))
        {
            return AZ::Failure(AZStd::string("Palette entry has no enabled native node factory"));
        }

        GraphEdit edit(owner, "ScriptCanvas automation: add node");

        owner.SignalAutomationBegin();
        const NodeIdPair created =
            owner.ProcessCreateNodeMimeEvent(choices[choiceIndex].get(), owner.GetActiveGraphCanvasGraphId(), position);
        owner.SignalAutomationEnd();

        AZ::EntityId nativeId = created.m_scriptCanvasId;
        if (!nativeId.IsValid())
        {
            // Event handler factories return a visual child of the handler node, and the handler node holds the native ID.
            AZ::EntityId wrapper;
            GraphCanvas::NodeRequestBus::EventResult(wrapper, created.m_graphCanvasId, &GraphCanvas::NodeRequests::GetWrappingNode);

            const AZ::EntityId owningNode = wrapper.IsValid() ? wrapper : created.m_graphCanvasId;
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, owningNode, &GraphCanvas::NodeRequests::GetUserData);
            if (const auto* id = AZStd::any_cast<AZ::EntityId>(userData))
            {
                nativeId = *id;
            }
        }

        ScriptCanvas::Node* node = nullptr;
        ScriptCanvas::GraphRequestBus::EventResult(node, EntityId(graph), &ScriptCanvas::GraphRequests::FindNode, nativeId);
        if (!node || !created.m_graphCanvasId.IsValid())
        {
            return AZ::Failure(AZStd::string("Native node factory failed; inspect the graph for partial changes"));
        }

        AutomationValueMap description = DescribeNode(*node, EntityId(graph));
        description.emplace("createdGraphCanvas", IdText(created.m_graphCanvasId));
        return AZ::Success(AZStd::move(description));
    }

    AutomationResult GraphAutomation::Connect(
        const AZStd::string& graph,
        const AZStd::string& sourceNode,
        const AZStd::string& sourceSlot,
        const AZStd::string& targetNode,
        const AZStd::string& targetSlot)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        const AZ::EntityId graphId = EntityId(graph);
        const ScriptCanvas::Endpoint source(EntityId(sourceNode), ScriptCanvas::SlotId(AZ::Uuid::CreateString(sourceSlot.c_str())));
        const ScriptCanvas::Endpoint target(EntityId(targetNode), ScriptCanvas::SlotId(AZ::Uuid::CreateString(targetSlot.c_str())));

        AZ::Outcome<void, AZStd::string> valid = AZ::Failure(AZStd::string("Graph has no connection validator"));
        ScriptCanvas::GraphRequestBus::EventResult(
            valid, graphId, &ScriptCanvas::GraphRequests::CanCreateConnectionBetween, source, target);
        if (!valid)
        {
            return AZ::Failure(valid.GetError());
        }

        GraphCanvas::Endpoint canvasSource;
        EditorGraphRequestBus::EventResult(canvasSource, graphId, &EditorGraphRequests::ConvertToGraphCanvasEndpoint, source);

        GraphCanvas::Endpoint canvasTarget;
        EditorGraphRequestBus::EventResult(canvasTarget, graphId, &EditorGraphRequests::ConvertToGraphCanvasEndpoint, target);

        if (!canvasSource.IsValid() || !canvasTarget.IsValid())
        {
            return AZ::Failure(AZStd::string("Native endpoint conversion refused a slot"));
        }

        GraphEdit edit(*window.GetValue(), "ScriptCanvas automation: connect");

        AZ::EntityId canvasConnection;
        GraphCanvas::SceneRequestBus::EventResult(
            canvasConnection,
            window.GetValue()->GetActiveGraphCanvasGraphId(),
            &GraphCanvas::SceneRequests::CreateConnectionBetween,
            canvasSource,
            canvasTarget);
        if (!canvasConnection.IsValid())
        {
            return AZ::Failure(AZStd::string("Native GraphCanvas connection creation failed"));
        }

        return AZ::Success(AutomationValueMap{ { "graphCanvas", IdText(canvasConnection) } });
    }

    AZStd::any GraphAutomation::CopyDatumValue(const AZStd::any& value)
    {
        if (const auto* text = AZStd::any_cast<AZStd::string_view>(&value))
        {
            return AZStd::any(AZStd::string(*text));
        }

        return value;
    }

    AutomationResult GraphAutomation::AssignValue(ScriptCanvas::ModifiableDatumView& view, const AZStd::any& value)
    {
        if (!view.IsValid() || value.empty())
        {
            return AZ::Failure(AZStd::string("Datum is not writable or value is empty"));
        }

        if (value.type() != ScriptCanvas::Data::ToAZType(view.GetDataType()))
        {
            return AZ::Failure(AZStd::string("Value must have the datum's native type; no lossy coercion"));
        }

        const ScriptCanvas::Data::Type type = ScriptCanvas::Data::FromAZType(value.type());
        ScriptCanvas::Datum replacement(type, ScriptCanvas::Datum::eOriginality::Copy, AZStd::any_cast<void>(&value), value.type());
        view.AssignToDatum(AZStd::move(replacement));

        return AZ::Success(AutomationValueMap{ { "value", view.GetDatum()->ToAny() } });
    }

    AutomationResult GraphAutomation::SetValue(
        const AZStd::string& graph, const AZStd::string& nodeId, const AZStd::string& slotId, const AZStd::any& value)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        ScriptCanvas::Node* node = nullptr;
        ScriptCanvas::GraphRequestBus::EventResult(node, EntityId(graph), &ScriptCanvas::GraphRequests::FindNode, EntityId(nodeId));

        const ScriptCanvas::SlotId id(AZ::Uuid::CreateString(slotId.c_str()));
        const ScriptCanvas::Slot* slot = node ? node->GetSlot(id) : nullptr;
        if (!slot || !slot->IsData() || !slot->IsInput() || slot->IsVariableReference())
        {
            return AZ::Failure(AZStd::string("Expected an unreferenced data input slot in this graph"));
        }

        bool connected = false;
        ScriptCanvas::GraphRequestBus::EventResult(
            connected, EntityId(graph), &ScriptCanvas::GraphRequests::IsEndpointConnected, slot->GetEndpoint());
        if (connected)
        {
            return AZ::Failure(AZStd::string("Disconnect the data input before setting its literal value"));
        }

        const AZStd::any nativeValue = CopyDatumValue(value);
        ScriptCanvas::ModifiableDatumView view;
        if (!node->FindModifiableDatumView(id, view) || nativeValue.type() != ScriptCanvas::Data::ToAZType(view.GetDataType()))
        {
            return AZ::Failure(AZStd::string("Slot has no matching writable native datum"));
        }

        GraphEdit edit(*window.GetValue(), "ScriptCanvas automation: set value");
        return AssignValue(view, nativeValue);
    }

    AutomationResult GraphAutomation::SetVariable(const AZStd::string& graph, const AZStd::string& variableId, const AZStd::any& value)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        ScriptCanvas::GraphVariable* variable = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(
            variable,
            EntityId(graph),
            &ScriptCanvas::GraphVariableManagerRequests::FindVariableById,
            ScriptCanvas::VariableId(AZ::Uuid::CreateString(variableId.c_str())));

        const AZStd::any nativeValue = CopyDatumValue(value);
        if (!variable || nativeValue.type() != ScriptCanvas::Data::ToAZType(variable->GetDataType()))
        {
            return AZ::Failure(AZStd::string("Variable is absent or value has a different native type"));
        }

        ScriptCanvas::ModifiableDatumView view;
        variable->ConfigureDatumView(view);

        GraphEdit edit(*window.GetValue(), "ScriptCanvas automation: set variable");
        return AssignValue(view, nativeValue);
    }

    AutomationResult GraphAutomation::RemoveNodes(const AZStd::string& graph, const AZStd::vector<AZStd::string>& nodes)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        // Check every node before removing any, so a bad ID removes nothing.
        AZStd::vector<AZ::EntityId> canvasIds;
        for (const AZStd::string& text : nodes)
        {
            ScriptCanvas::Node* node = nullptr;
            ScriptCanvas::GraphRequestBus::EventResult(node, EntityId(graph), &ScriptCanvas::GraphRequests::FindNode, EntityId(text));
            if (!node)
            {
                return AZ::Failure(AZStd::string("Node is absent from this graph"));
            }

            AZ::EntityId canvas;
            SceneMemberMappingRequestBus::EventResult(canvas, node->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);
            if (!canvas.IsValid())
            {
                return AZ::Failure(AZStd::string("Node has no GraphCanvas mapping"));
            }

            canvasIds.push_back(canvas);
        }

        if (canvasIds.empty())
        {
            return AZ::Failure(AZStd::string("Provide at least one node"));
        }

        GraphEdit edit(*window.GetValue(), "ScriptCanvas automation: remove nodes");
        window.GetValue()->DeleteNodes(window.GetValue()->GetActiveGraphCanvasGraphId(), canvasIds);

        return AZ::Success(AutomationValueMap{ { "removed", static_cast<AZ::u32>(canvasIds.size()) } });
    }

    AutomationResult GraphAutomation::RemoveConnections(const AZStd::string& graph, const AZStd::vector<AZStd::string>& connections)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        AZStd::vector<AZ::EntityId> graphConnections;
        ScriptCanvas::GraphRequestBus::EventResult(graphConnections, EntityId(graph), &ScriptCanvas::GraphRequests::GetConnections);

        // Check every connection before removing any, so a bad ID removes nothing.
        AZStd::vector<AZ::EntityId> canvasIds;
        for (const AZStd::string& text : connections)
        {
            const AZ::EntityId id = EntityId(text);
            if (AZStd::find(graphConnections.begin(), graphConnections.end(), id) == graphConnections.end())
            {
                return AZ::Failure(AZStd::string("Connection is absent from this graph"));
            }

            AZ::EntityId canvas;
            SceneMemberMappingRequestBus::EventResult(canvas, id, &SceneMemberMappingRequests::GetGraphCanvasEntityId);
            if (!canvas.IsValid())
            {
                return AZ::Failure(AZStd::string("Connection has no GraphCanvas mapping"));
            }

            canvasIds.push_back(canvas);
        }

        if (canvasIds.empty())
        {
            return AZ::Failure(AZStd::string("Provide at least one connection"));
        }

        GraphEdit edit(*window.GetValue(), "ScriptCanvas automation: remove connections");
        window.GetValue()->DeleteConnections(window.GetValue()->GetActiveGraphCanvasGraphId(), canvasIds);

        return AZ::Success(AutomationValueMap{ { "removed", static_cast<AZ::u32>(canvasIds.size()) } });
    }
} // namespace ScriptCanvasEditor
