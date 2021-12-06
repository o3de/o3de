/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <ScriptCanvas/Bus/UndoBus.h>

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class GraphItemCommandNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        virtual void PreRestore(const UndoData& /*oldData*/) {};
        virtual void PostRestore(const UndoData& /* restoredData*/) {};
    };

    using GraphItemCommandNotificationBus = AZ::EBus<GraphItemCommandNotifications>;

    class ScriptCanvasMemoryAsset;

    // This command is the base URSequencePoint command from which all Script Canvas undo/redo commands derive
    class GraphItemCommand
        : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(GraphItemCommand, "{94B50FAC-ACAF-4B9B-BA3C-9F3EE36854BA}", AzToolsFramework::UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(GraphItemCommand, AZ::SystemAllocator, 0);

        explicit GraphItemCommand(AZStd::string_view friendlyName);
        ~GraphItemCommand() override = default;

        void Undo() override;
        void Redo() override;

        virtual void Capture(Graph* graph, bool captureUndo);

        bool Changed() const override;

    protected:
        GraphItemCommand(const GraphItemCommand&) = delete;
        const GraphItemCommand& operator=(const GraphItemCommand&) = delete;

        virtual void RestoreItem(const AZStd::vector<AZ::u8>& restoreBuffer);

        AZ::EntityId m_graphCanvasGraphId;
        AZ::EntityId                 m_scriptCanvasEntityId; ///< The id of the ScriptCanvas Entity with the Script Canvas Graph and Graph Canvas Scene
        ScriptCanvas::ScriptCanvasId m_scriptCanvasId; 
        AZStd::vector<AZ::u8> m_undoState;
        AZStd::vector<AZ::u8> m_redoState;
    };

    // This commands captures a modification to a Script Canvas graph involving either a ScriptCanvas Node/Connection or a GraphCanvas Node/Connection
    class GraphItemChangeCommand
        : public GraphItemCommand
    {
    public:
        AZ_RTTI(GraphItemChangeCommand, "{9F8805F7-61CD-40FC-B426-020925F4E3DB}", GraphItemCommand);
        AZ_CLASS_ALLOCATOR(GraphItemChangeCommand, AZ::SystemAllocator, 0);

        GraphItemChangeCommand(AZStd::string_view friendlyName);
        ~GraphItemChangeCommand() = default;

        void Undo() override;
        void Redo() override;
        void Capture(Graph* graph, bool captureUndo) override;

        void RestoreItem(const AZStd::vector<AZ::u8>& restoreBuffer) override;

    protected:
        GraphItemChangeCommand(const GraphItemChangeCommand&) = delete;
        const GraphItemChangeCommand& operator=(const GraphItemChangeCommand&) = delete;

        void DeleteOldGraphData(const UndoData& oldData);
        void ActivateRestoredGraphData(const UndoData& restoredData);
    };


    // This command captures when a node or connection is added to the Script Canvas graph.
    class GraphItemAddCommand
        : public GraphItemChangeCommand
    {
    public:
        AZ_RTTI(GraphItemAddCommand, "{01E6BC39-0A2C-4C05-9384-804A63321D62}", GraphItemChangeCommand);
        AZ_CLASS_ALLOCATOR(GraphItemAddCommand, AZ::SystemAllocator, 0);

        explicit GraphItemAddCommand(AZStd::string_view friendlyName);
        ~GraphItemAddCommand() override = default;

        void Undo() override;
        void Redo() override;

        void Capture(Graph* graph, bool captureUndo) override;

    protected:
        GraphItemAddCommand(const GraphItemAddCommand&) = delete;
        const GraphItemAddCommand& operator=(const GraphItemAddCommand&) = delete;
    };

    // This command captures when a node or connection is removed from the Script Canvas graph
    class GraphItemRemovalCommand
        : public GraphItemChangeCommand
    {
    public:
        AZ_RTTI(GraphItemRemovalCommand, "{6257B3EC-E9E8-4419-AA25-2A768C21B635}", GraphItemChangeCommand);
        AZ_CLASS_ALLOCATOR(GraphItemRemovalCommand, AZ::SystemAllocator, 0);

        explicit GraphItemRemovalCommand(AZStd::string_view friendlyName);
        ~GraphItemRemovalCommand() override = default;

        void Undo() override;
        void Redo() override;

        void Capture(Graph* graph, bool captureUndo) override;

    protected:
        GraphItemRemovalCommand(const GraphItemRemovalCommand&) = delete;
        const GraphItemRemovalCommand& operator=(const GraphItemRemovalCommand&) = delete;
    };
}
