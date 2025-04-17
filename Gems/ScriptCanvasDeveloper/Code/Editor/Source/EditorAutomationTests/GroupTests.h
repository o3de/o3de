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
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/CreateElementsActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/GraphActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/VariableActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorautomationTest that will test out various methods creating a group
    */
    class CreateGroupTest
        : public EditorAutomationTest
    {
    public:
          CreateGroupTest(CreateGroupAction::CreationType creationType = CreateGroupAction::CreationType::Hotkey);
          ~CreateGroupTest() override = default;
    };

    /**
        EditorautomationTest that will test out how elements are added/removed from groups in several situations(Addition to group via context menu, drag/drop, connection proposal, or resizing. Removal from group through movement and resizing).
    */
    class GroupManipulationTest
        : public EditorAutomationTest
    {
    private:

        class OffsetPositionByNodeDimension
            : public CustomActionState
        {
        public:
            AZ_CLASS_ALLOCATOR(OffsetPositionByNodeDimension, AZ::SystemAllocator)
            // -1 to 1 will decide how much and which direction we manipulate the specified value by our width/height.
            OffsetPositionByNodeDimension(float horizontalDimension, float verticalDimension, AutomationStateModelId nodeId, AutomationStateModelId position);
            ~OffsetPositionByNodeDimension() override = default;

            void OnCustomAction() override;

        private:

            float m_horizontalDimension = 0.0f;
            float m_verticalDimension = 0.0f;

            AutomationStateModelId m_nodeId;
            AutomationStateModelId m_positionId;
        };

    public:
        GroupManipulationTest(GraphCanvas::NodePaletteWidget* nodePaletteWidget);
        ~GroupManipulationTest() override = default;
    };
}
