/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ScriptCanvas/Bus/GraphAutomationRequestBus.h>
#include <ScriptCanvas/Core/Core.h>

namespace GraphCanvas
{
    class GraphCanvasMimeEvent;
    class GraphCanvasTreeItem;
} // namespace GraphCanvas

namespace ScriptCanvas
{
    class ModifiableDatumView;
    class Node;
} // namespace ScriptCanvas

namespace ScriptCanvasEditor
{
    class EditorScriptCanvasComponent;
    class MainWindow;

    //! Handles GraphAutomationRequestBus by driving the Script Canvas editor window.
    //!
    //! The Script Canvas editor SystemComponent owns one instance, and creates it only in the Editor application.
    //! Every request runs on the editor's main thread and edits the graph of the active editor tab through the
    //! same MainWindow operations the editor UI uses, so edits share the editor's undo stack.
    class GraphAutomation final
        : private GraphAutomationRequestBus::Handler
        , private GraphAutomationNotificationBus::Handler
    {
    public:
        //! Reflects the request bus to the behavior context, and the result containers to the serialize context.
        static void Reflect(AZ::ReflectContext* context);

        //! Connects to the request and notification buses.
        void Activate();

        //! Disconnects from the request and notification buses, and stops observing any save.
        void Deactivate();

    private:
        friend class GraphAutomationTests;

        //! Groups the changes of one request into a single undoable edit of the active graph.
        class GraphEdit;

        using WindowResult = AZ::Outcome<MainWindow*, AZStd::string>;
        using PaletteChoices = AZStd::vector<AZStd::unique_ptr<GraphCanvas::GraphCanvasMimeEvent>>;

        //////////////////////////////////////////////////////////////////////////
        // GraphAutomationRequestBus::Handler overrides
        AutomationResult Create(const AZStd::string& path) override;
        AutomationResult Open(const AZStd::string& path) override;
        AutomationResult Reload(const AZStd::string& graph, bool discard) override;
        AutomationResult Describe(const AZStd::string& graph) override;
        AutomationResult Palette(const AZStd::string& query, AZ::u32 after, AZ::u32 limit) override;
        AutomationResult AddNode(const AZStd::string& graph, const AZStd::string& palette, const AZ::Vector2& position) override;
        AutomationResult Connect(
            const AZStd::string& graph,
            const AZStd::string& sourceNode,
            const AZStd::string& sourceSlot,
            const AZStd::string& targetNode,
            const AZStd::string& targetSlot) override;
        AutomationResult RemoveNodes(const AZStd::string& graph, const AZStd::vector<AZStd::string>& nodes) override;
        AutomationResult RemoveConnections(const AZStd::string& graph, const AZStd::vector<AZStd::string>& connections) override;
        AutomationResult SetValue(
            const AZStd::string& graph, const AZStd::string& node, const AZStd::string& slot, const AZStd::any& value) override;
        AutomationResult SetVariable(const AZStd::string& graph, const AZStd::string& variable, const AZStd::any& value) override;
        AutomationResult Save(const AZStd::string& graph, const AZStd::string& path) override;
        AutomationResult PollSave() override;
        void CancelSave() override;
        AutomationResult Attach(const AZ::EntityComponentIdPair& component, const AZStd::string& path) override;
        AutomationResult Attachment(const AZ::EntityComponentIdPair& component) override;
        AutomationResult Undo(const AZStd::string& graph) override;
        AutomationResult Redo(const AZStd::string& graph) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // GraphAutomationNotificationBus::Handler overrides
        void OnAutomationSaveComplete(const AZStd::string& path, const AZStd::string& error) override;
        //////////////////////////////////////////////////////////////////////////

        // Editor access

        //! Returns the Script Canvas editor window, opening its view pane if needed. Fails off the main thread,
        //! in game mode, and while a save is in progress.
        WindowResult Window();

        //! Returns the editor window as Window does, and also fails unless graph is the graph of the active tab.
        WindowResult GraphWindow(const AZStd::string& graph);

        //! Returns the Script Canvas editor component named by the pair, if it is on an editor entity.
        static AZ::Outcome<EditorScriptCanvasComponent*, AZStd::string> ScriptComponent(const AZ::EntityComponentIdPair& component);

        // Identifiers

        //! Validates and normalizes a graph source path: absolute, inside the project, with the Script Canvas
        //! source extension, and not leaving the project through a symbolic link.
        static AZ::Outcome<AZ::IO::Path, AZStd::string> SourcePath(const AZStd::string& path);

        //! Returns the decimal text of an entity ID, the form used for IDs in requests and results.
        static AZStd::string IdText(AZ::EntityId id);

        //! Parses the decimal text of an entity ID. Returns an invalid ID if the text is not a whole number.
        static AZ::EntityId EntityId(const AZStd::string& text);

        // Descriptions

        //! Describes the active document: its IDs, path, source registration, unsaved state and undo state.
        static AutomationValueMap Identity(MainWindow& window);

        //! Describes a node and its slots.
        static AutomationValueMap DescribeNode(ScriptCanvas::Node& node, AZ::EntityId graph);

        // Palette and values

        //! Takes ownership of a palette entry's event and returns the events that each create one node.
        //! A multiple-choice event is replaced by its choices without being executed.
        static PaletteChoices ExpandPaletteChoices(AZStd::unique_ptr<GraphCanvas::GraphCanvasMimeEvent> event);

        //! Returns a copy of value that owns its data. Python lends strings as string views; datums must own them.
        static AZStd::any CopyDatumValue(const AZStd::any& value);

        //! Assigns value to the datum if it has exactly the datum's type. Returns the stored value.
        static AutomationResult AssignValue(ScriptCanvas::ModifiableDatumView& view, const AZStd::any& value);

        //! True from Save until PollSave returns the save's result or CancelSave is called.
        bool m_saving = false;

        //! The result of the observed save once it finishes. PollSave consumes it.
        AZStd::optional<AutomationResult> m_saveResult;
    };
} // namespace ScriptCanvasEditor
