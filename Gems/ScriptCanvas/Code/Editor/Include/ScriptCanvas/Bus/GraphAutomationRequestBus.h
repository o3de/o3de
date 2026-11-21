/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>

namespace ScriptCanvasEditor
{
    //! A named set of result values. EditorPythonBindings marshals it like any other editor map of AZStd::any values.
    using AutomationValueMap = AZStd::unordered_map<AZStd::string, AZStd::any>;

    //! A list of result value maps, used for the nodes, connections, variables and palette entries in a result.
    using AutomationValueList = AZStd::vector<AutomationValueMap>;

    //! The result of a graph automation request: the result values on success, or an error message on failure.
    using AutomationResult = AZ::Outcome<AutomationValueMap, AZStd::string>;

    //! Edits Script Canvas graphs through the Script Canvas editor without opening any dialog.
    //!
    //! Requests drive the same editor document, GraphCanvas scene and undo stack as the editor UI, so every edit
    //! can be undone in the editor. Requests must be made on the editor's main thread, and they fail while the
    //! editor is in game mode or while a save started by Save has not been consumed by PollSave or CancelSave.
    //!
    //! Graph, node and connection IDs are the decimal text of native Script Canvas entity IDs, not editor entity
    //! references. Requests that take a graph ID only accept the graph of the active editor tab.
    //! Slot and variable IDs are UUID strings. Source paths must be absolute .scriptcanvas paths inside the project.
    class GraphAutomationRequests : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Documents

        //! Creates a new graph that will be saved at path, and opens it in a new tab.
        //! Fails if a file already exists at path. Returns the identity of the new graph.
        virtual AutomationResult Create(const AZStd::string& path) = 0;

        //! Opens the graph source at path, which Asset Processor must have registered. Returns its identity.
        virtual AutomationResult Open(const AZStd::string& path) = 0;

        //! Closes the graph and opens its saved source again. Fails if the graph has unsaved changes unless
        //! discard is true, in which case those changes are lost.
        virtual AutomationResult Reload(const AZStd::string& graph, bool discard) = 0;

        //! Returns the identity of the graph together with its nodes, connections and variables.
        //! Fails for graphs too large to describe in one result.
        virtual AutomationResult Describe(const AZStd::string& graph) = 0;

        // Nodes and connections

        //! Returns one page of the node palette entries whose palette path contains query, ignoring case.
        //! Skips the first after matches and returns at most limit entries, along with the total match count and
        //! the after value of the next page. Palette IDs are only valid until the node palette changes.
        virtual AutomationResult Palette(const AZStd::string& query, AZ::u32 after, AZ::u32 limit) = 0;

        //! Adds the node created by a palette entry at position in the graph's scene, as one undoable edit.
        //! Returns the description of the new node.
        virtual AutomationResult AddNode(const AZStd::string& graph, const AZStd::string& palette, const AZ::Vector2& position) = 0;

        //! Connects two slots, if the graph allows that connection, as one undoable edit.
        //! Returns the GraphCanvas ID of the new connection.
        virtual AutomationResult Connect(
            const AZStd::string& graph,
            const AZStd::string& sourceNode,
            const AZStd::string& sourceSlot,
            const AZStd::string& targetNode,
            const AZStd::string& targetSlot) = 0;

        //! Removes nodes from the graph as one undoable edit. Fails without removing anything if any node is not
        //! in the graph or the list is empty.
        virtual AutomationResult RemoveNodes(const AZStd::string& graph, const AZStd::vector<AZStd::string>& nodes) = 0;

        //! Removes connections from the graph as one undoable edit. Fails without removing anything if any
        //! connection is not in the graph or the list is empty.
        virtual AutomationResult RemoveConnections(const AZStd::string& graph, const AZStd::vector<AZStd::string>& connections) = 0;

        // Values

        //! Sets the value of an unconnected data input slot that does not reference a variable, as one undoable edit.
        //! The value must have exactly the slot's type; values are never converted. Returns the stored value.
        virtual AutomationResult SetValue(
            const AZStd::string& graph, const AZStd::string& node, const AZStd::string& slot, const AZStd::any& value) = 0;

        //! Sets the value of a graph variable, as one undoable edit. The value must have exactly the variable's
        //! type; values are never converted. Returns the stored value.
        virtual AutomationResult SetVariable(const AZStd::string& graph, const AZStd::string& variable, const AZStd::any& value) = 0;

        // Saving

        //! Starts the editor's own save of the graph to path and returns at once with pending set.
        //! Fails if path holds a different graph. Only one save can be observed at a time.
        virtual AutomationResult Save(const AZStd::string& graph, const AZStd::string& path) = 0;

        //! Returns pending while the save started by Save runs. Once it finishes, returns its result, which ends
        //! the observed save. Fails if no save is being observed.
        virtual AutomationResult PollSave() = 0;

        //! Stops observing the save started by Save. The save itself still runs to completion.
        virtual void CancelSave() = 0;

        // Components

        //! Selects the graph source at path on a Script Canvas editor component, as one undoable edit.
        //! Asset Processor must have registered the source. Returns the component's selection, as Attachment does.
        virtual AutomationResult Attach(const AZ::EntityComponentIdPair& component, const AZStd::string& path) = 0;

        //! Returns the graph source selected on a Script Canvas editor component.
        virtual AutomationResult Attachment(const AZ::EntityComponentIdPair& component) = 0;

        // Undo

        //! Undoes the graph's last change. Fails if there is nothing to undo. Returns the graph's description.
        virtual AutomationResult Undo(const AZStd::string& graph) = 0;

        //! Redoes the graph's last undone change. Fails if there is nothing to redo. Returns the graph's description.
        virtual AutomationResult Redo(const AZStd::string& graph) = 0;
    };

    using GraphAutomationRequestBus = AZ::EBus<GraphAutomationRequests>;

    //! Notifications that the Script Canvas editor sends for graph automation.
    class GraphAutomationNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Sent on the main thread when a save started by GraphAutomationRequests::Save finishes.
        //! error is empty if the save succeeded.
        virtual void OnAutomationSaveComplete(const AZStd::string& path, const AZStd::string& error) = 0;
    };

    using GraphAutomationNotificationBus = AZ::EBus<GraphAutomationNotifications>;
} // namespace ScriptCanvasEditor
