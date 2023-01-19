/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QLineEdit>
#include <QMenu>
#include <QString>
#include <QTableView>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/CreateElementsStates.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorautomationTest that will test out the AltClick to delete elements functionality for nodes, connected nodes, and connections
    */
    class AltClickDeleteTest
        : public EditorAutomationTest
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AltClickDeleteTest();
        ~AltClickDeleteTest() override = default;
    };

    /**
        EditorautomationTest that will test out the cut/copy/paste functions
    */
    class CutCopyPasteDuplicateTest
        : public EditorAutomationTest
        , public GraphCanvas::SceneNotificationBus::Handler
    {
        class CheckpointState
            : public CustomActionState
        {
        public:
            CheckpointState(AZStd::string checkpoint);
            ~CheckpointState() override = default;

            void OnCustomAction() override {}
        };

    public:
        AZ_CLASS_ALLOCATOR(CutCopyPasteDuplicateTest, AZ::SystemAllocator)
        CutCopyPasteDuplicateTest(QString nodeName);
        ~CutCopyPasteDuplicateTest() override = default;

        // SceneNotificationBus
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        void OnNodeRemoved(const AZ::EntityId& nodeId) override;
        ///

        // EditorAutomationTests
        void OnStateComplete(int stateId) override;
        ////

    private:

        void ProcessCreationSet();

        AutomationStateModelId m_originalNodeId;

        GraphCanvas::NodeId   m_removalTarget;
        AZStd::unordered_set< GraphCanvas::NodeId > m_createdSet;

        CreateNodeFromContextMenuState* m_createNodeState = nullptr;

        CheckpointState* m_cutPasteCheckpoint = nullptr;
        CheckpointState* m_copyPasteCheckpoint = nullptr;
        CheckpointState* m_copyPasteCopyCheckpoint = nullptr;
        CheckpointState* m_duplicateCheckpoint = nullptr;
    };
}
