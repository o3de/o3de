
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QMenu>
#include <QMessageBox>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Slots/Property/PropertySlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    /////////////////////////
    // RemoveSlotMenuAction
    /////////////////////////

    RemoveSlotMenuAction::RemoveSlotMenuAction(QObject* parent)
        : SlotContextMenuAction("Remove slot", parent)
    {
    }

    void RemoveSlotMenuAction::RefreshAction()
    {        
        bool enableAction = false;
        bool isUserSlot = false;

        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();

        if (GraphUtils::IsSlot(targetId))
        {
            if (!GraphUtils::IsSlotType(targetId, SlotTypes::ExtenderSlot)
                && !GraphUtils::IsSlotType(targetId, SlotTypes::PropertySlot))
            {
                Endpoint endpoint;
                SlotRequestBus::EventResult(endpoint, targetId, &SlotRequests::GetEndpoint);

                GraphModelRequestBus::EventResult(enableAction, graphId, &GraphModelRequests::IsSlotRemovable, endpoint);
            }

            if (GraphUtils::IsSlotType(targetId, SlotTypes::DataSlot))
            {
                DataSlotRequestBus::EventResult(isUserSlot, targetId, &DataSlotRequests::IsUserSlot);
            }
        }

        setEnabled(enableAction || isUserSlot);
    }

    ContextMenuAction::SceneReaction RemoveSlotMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();
        
        if (GraphUtils::IsSlot(targetId))
        {
            bool hasConnections = false;
            SlotRequestBus::EventResult(hasConnections, targetId, &SlotRequests::HasConnections);

            NodeId nodeId;
            SlotRequestBus::EventResult(nodeId, targetId, &SlotRequests::GetNode);

            GraphId graphId2;
            SceneMemberRequestBus::EventResult(graphId2, nodeId, &SceneMemberRequests::GetScene);

            if (hasConnections)
            {

                GraphCanvasGraphicsView* graphicsView = nullptr;

                ViewId viewId;
                SceneRequestBus::EventResult(viewId, graphId2, &SceneRequests::GetViewId);

                ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

                QMessageBox::StandardButton result = QMessageBox::question(graphicsView, "Slot has active connections", "The selected slot has active connections, as you sure you wish to remove it?");

                if (result == QMessageBox::StandardButton::Cancel
                    || result == QMessageBox::StandardButton::No)
                {
                    return SceneReaction::Nothing;
                }
            }

            GraphModelRequestBus::Event(graphId2, &GraphModelRequests::RemoveSlot, Endpoint(nodeId, targetId));

            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    ///////////////////////////////
    // ClearConnectionsMenuAction
    ///////////////////////////////
    
    ClearConnectionsMenuAction::ClearConnectionsMenuAction(QObject* parent)
        : SlotContextMenuAction("Clear connections", parent)
    {
    }
    
    void ClearConnectionsMenuAction::RefreshAction()
    {
        bool enableAction = false;

        const AZ::EntityId& targetId = GetTargetId();
        
        if (GraphUtils::IsSlot(targetId))
        {
            SlotRequestBus::EventResult(enableAction, targetId, &SlotRequests::HasConnections);
        }
        
        setEnabled(enableAction);
    }

    ContextMenuAction::SceneReaction ClearConnectionsMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();
        
        if (GraphUtils::IsSlot(targetId))
        {
            SlotRequestBus::Event(targetId, &SlotRequests::ClearConnections);
            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    //////////////////////////////////
    // ResetToDefaultValueMenuAction
    //////////////////////////////////

    ResetToDefaultValueMenuAction::ResetToDefaultValueMenuAction(QObject* parent)
        : SlotContextMenuAction("Reset Value", parent)
    {

    }

    void ResetToDefaultValueMenuAction::RefreshAction()
    {
        bool enableAction = false;

        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();

        if (GraphUtils::IsSlot(targetId))
        {            
            SlotType slotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(slotType, targetId, &SlotRequests::GetSlotType);

            if (slotType == SlotTypes::DataSlot)
            {
                enableAction = true;

                Endpoint endpoint;
                SlotRequestBus::EventResult(endpoint, targetId, &SlotRequests::GetEndpoint);

                GraphModelRequestBus::EventResult(enableAction, graphId, &GraphModelRequests::AllowReset, endpoint);

                DataSlotType slotType2 = DataSlotType::Unknown;
                DataSlotRequestBus::EventResult(slotType2, targetId, &DataSlotRequests::GetDataSlotType);

                if (slotType2 == DataSlotType::Reference)
                {
                    setText("Reset Reference");
                }
                else
                {
                    setText("Reset Value");
                }
            }
            else if (slotType == SlotTypes::PropertySlot)
            {
                enableAction = true;
                setText("Reset Property");
            }
            else
            {
                setText("Reset Value");
            }
        }

        setEnabled(enableAction);
    }

    ContextMenuAction::SceneReaction ResetToDefaultValueMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();
        
        Endpoint endpoint;
        SlotRequestBus::EventResult(endpoint, targetId, &SlotRequests::GetEndpoint);

        SlotType slotType = SlotTypes::Invalid;
        SlotRequestBus::EventResult(slotType, targetId, &SlotRequests::GetSlotType);

        if (slotType == SlotTypes::DataSlot)
        {
            DataSlotType slotType2 = DataSlotType::Unknown;
            DataSlotRequestBus::EventResult(slotType2, targetId, &DataSlotRequests::GetDataSlotType);

            if (slotType2 == DataSlotType::Value)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::ResetSlotToDefaultValue, endpoint);
            }
            else
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::ResetReference, endpoint);
            }
        }
        else if (slotType == SlotTypes::PropertySlot)
        {
            AZ::Crc32 propertyId;
            PropertySlotRequestBus::EventResult(propertyId, targetId, &PropertySlotRequests::GetPropertyId);

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::ResetProperty, endpoint.GetNodeId(), propertyId);
        }

        return SceneReaction::PostUndo;
    }

    ///////////////////////////////
    // ToggleReferenceStateAction
    ///////////////////////////////

    ToggleReferenceStateAction::ToggleReferenceStateAction(QObject* parent)
        : GraphCanvas::SlotContextMenuAction("Toggle Reference", parent)
    {

    }

    void ToggleReferenceStateAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();

        if (GraphUtils::IsSlot(targetId))
        {
            SlotType slotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(slotType, targetId, &SlotRequests::GetSlotType);

            if (slotType == SlotTypes::DataSlot)
            {
                DataSlotType dataSlotType = DataSlotType::Unknown;
                DataSlotRequestBus::EventResult(dataSlotType, targetId, &DataSlotRequests::GetDataSlotType);

                bool canToggleState = false;

                if (DataSlotUtils::IsValueDataSlotType(dataSlotType))
                {
                    setText("Convert to Reference");
                    DataSlotRequestBus::EventResult(canToggleState, targetId, &DataSlotRequests::CanConvertToReference);
                }
                else
                {
                    setText("Convert to Value");
                    DataSlotRequestBus::EventResult(canToggleState, targetId, &DataSlotRequests::CanConvertToValue);
                }
                

                setEnabled(canToggleState);
            }
            else
            {
                setEnabled(false);
            }
        }
        else
        {
            setEnabled(false);
        }
    }

    ContextMenuAction::SceneReaction ToggleReferenceStateAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {        
        bool toggledState = false;

        const AZ::EntityId& targetId = GetTargetId();

        DataSlotType dataSlotType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(dataSlotType, targetId, &DataSlotRequests::GetDataSlotType);

        if (DataSlotUtils::IsValueDataSlotType(dataSlotType))
        {
            DataSlotRequestBus::EventResult(toggledState, targetId, &DataSlotRequests::ConvertToReference);
        }
        else
        {
            DataSlotRequestBus::EventResult(toggledState, targetId, &DataSlotRequests::ConvertToValue);
        }        

        if (toggledState)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }

    ////////////////////////////
    // PromoteToVariableAction
    ////////////////////////////

    PromoteToVariableAction::PromoteToVariableAction(QObject* parent)
        : GraphCanvas::SlotContextMenuAction("Promote to Variable", parent)
    {

    }

    void PromoteToVariableAction::RefreshAction()
    {
        bool enableAction = false;
        bool isUserSlot = false;

        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();

        if (GraphUtils::IsSlot(targetId))
        {
            SlotType slotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(slotType, targetId, &SlotRequests::GetSlotType);

            DataSlotRequestBus::EventResult(isUserSlot, targetId, &DataSlotRequests::IsUserSlot);

            if (slotType == SlotTypes::DataSlot)
            {
                ConnectionType connectionType = CT_Invalid;
                SlotRequestBus::EventResult(connectionType, targetId, &SlotRequests::GetConnectionType);

                if (connectionType == CT_Input)
                {
                    DataSlotType dataSlotType = DataSlotType::Unknown;
                    DataSlotRequestBus::EventResult(dataSlotType, targetId, &DataSlotRequests::GetDataSlotType);

                    if (DataSlotUtils::IsValueDataSlotType(dataSlotType))
                    {
                        DataSlotRequestBus::EventResult(enableAction, targetId, &DataSlotRequests::CanConvertToReference);

                        if (enableAction)
                        {
                            Endpoint endpoint;
                            SlotRequestBus::EventResult(endpoint, targetId, &SlotRequests::GetEndpoint);

                            GraphModelRequestBus::EventResult(enableAction, graphId, &GraphModelRequests::CanPromoteToVariable, endpoint);
                        }
                    }
                }
            }
        }

        setEnabled(enableAction && !isUserSlot);
    }

    GraphCanvas::ContextMenuAction::SceneReaction PromoteToVariableAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();
        
        Endpoint endpoint;
        SlotRequestBus::EventResult(endpoint, targetId, &SlotRequests::GetEndpoint);

        bool promotedElement = false;
        GraphModelRequestBus::EventResult(promotedElement, graphId, &GraphModelRequests::PromoteToVariableAction, endpoint);

        if (promotedElement)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }
}
