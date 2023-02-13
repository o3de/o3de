/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QMimeData>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/Endpoint.h>

class QMimeData;

namespace GraphCanvas
{    
    class NodePropertyDisplay;
    struct Endpoint;
    
    class GraphSettingsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = GraphId;
    };
    
    using GraphSettingsRequestBus = AZ::EBus<GraphSettingsRequests>;
    
    class GraphModelRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = GraphId;

        //! Callback for requesting an Undo Point to be posted.
        virtual void RequestUndoPoint() = 0;

        //! Callback for requesting the incrementation of the value of the ignore undo point tracker
        virtual void RequestPushPreventUndoStateUpdate() = 0;

        //! Callback for requesting the decrementation of the value of the ignore undo point tracker
        virtual void RequestPopPreventUndoStateUpdate() = 0;

        //! Request to trigger an undo
        virtual void TriggerUndo() = 0;

        //! Request to trigger a redo
        virtual void TriggerRedo() = 0;

        // Enable the specified nodes
        virtual void EnableNodes(const AZStd::unordered_set< NodeId >& /*nodeIds*/)
        {
        }

        // Disables the specified nodes
        virtual void DisableNodes(const AZStd::unordered_set< NodeId >& /*nodeIds*/)
        {
        }

        //! Request to create a NodePropertyDisplay class for a particular DataSlot.
        virtual NodePropertyDisplay* CreateDataSlotPropertyDisplay([[maybe_unused]] const AZ::Uuid& dataType, [[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const SlotId& slotId) const { return nullptr; }
        virtual NodePropertyDisplay* CreateDataSlotVariablePropertyDisplay([[maybe_unused]] const AZ::Uuid& dataType, [[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const SlotId& slotId) const { return nullptr; }
        virtual NodePropertyDisplay* CreatePropertySlotPropertyDisplay([[maybe_unused]] const AZ::Crc32& propertyId, [[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const SlotId& slotId) const { return nullptr; }

        //! This is sent when a connection is disconnected.
        virtual void DisconnectConnection(const ConnectionId& connectionId) = 0;

        //! This is sent when attempting to create a given connection.
        virtual bool CreateConnection(const ConnectionId& connectionId, const Endpoint& sourcePoint, const Endpoint& targetPoint) = 0;

        //! This is sent to confirm whether or not a connection can take place.
        virtual bool IsValidConnection(const Endpoint& sourcePoint, const Endpoint& targetPoint) const = 0;

        //! This will return the structure needed to display why a connection could not be created between the specified endpoints.
        virtual ConnectionValidationTooltip GetConnectionValidityTooltip(const Endpoint& sourcePoint, const Endpoint& targetPoint) const
        {
            ConnectionValidationTooltip result;

            result.m_isValid = IsValidConnection(sourcePoint, targetPoint);

            return result;
        }

        //! Get the Display Type name for the given AZ type
        virtual AZStd::string GetDataTypeString(const AZ::Uuid& typeId) = 0;

        // Signals out that the specified elements save data is dirty.
        virtual void OnSaveDataDirtied(const AZ::EntityId& savedElement) = 0;

        // Signals out that the graph was signeld to clean itself up.
        virtual void OnRemoveUnusedNodes() = 0;
        virtual void OnRemoveUnusedElements() = 0;

        virtual bool AllowReset([[maybe_unused]] const Endpoint& endpoint) const
        {
            return true;
        }

        virtual void ResetSlotToDefaultValue(const Endpoint& endpoint) = 0;

        virtual void ResetReference([[maybe_unused]] const Endpoint& endpoint)
        {
        }

        virtual void ResetProperty([[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const AZ::Crc32& propertyId)
        {
        }

        virtual void RemoveSlot([[maybe_unused]] const Endpoint& endpoint)
        {
        }

        virtual bool IsSlotRemovable([[maybe_unused]] const Endpoint& endpoint) const
        {
            return false;
        }

        virtual bool ConvertSlotToReference([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] bool isNewSlot)
        {
            return false;
        }

        virtual bool CanConvertSlotToReference([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] bool isNewSlot)
        {
            return false;
        }

        virtual CanHandleMimeEventOutcome CanHandleReferenceMimeEvent([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] const QMimeData* mimeData)
        {
           return AZ::Failure(AZStd::string(""));
        }

        virtual bool HandleReferenceMimeEvent([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] const QMimeData* mimeData)
        {
            return false;
        }

        virtual bool CanPromoteToVariable([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] bool isNewSlot = false) const
        {
            return false;
        }

        virtual bool PromoteToVariableAction([[maybe_unused]] const Endpoint& endpoint
            , [[maybe_unused]] bool isNewSlot)
        {
            return false;
        }

        virtual bool SynchronizeReferences([[maybe_unused]] const Endpoint& sourceEndpoint, [[maybe_unused]] const Endpoint& targetEndpoint)
        {
            return false;
        }

        virtual bool ConvertSlotToValue([[maybe_unused]] const Endpoint& endpoint)
        {
            return false;
        }

        virtual bool CanConvertSlotToValue([[maybe_unused]] const Endpoint& endpoint)
        {
            return false;
        }

        virtual bool CanConvertSlotAndConnect([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] const Endpoint& synchronizeEndpoint)
        {
            return false;
        }

        virtual CanHandleMimeEventOutcome CanHandleValueMimeEvent([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] const QMimeData* mimeData)
        {
            return AZ::Failure(AZStd::string(""));
        }

        // Returns whether or not the mime event was successsfully handled.
        virtual bool HandleValueMimeEvent([[maybe_unused]] const Endpoint& endpoint, [[maybe_unused]] const QMimeData* mimeData)
        {
            return false;
        }

        //////////////////////////////////////
        // Extender Slot Optional Overrides

        enum class ExtensionRequestReason
        {
            Internal,
            UserRequest,
            ConnectionProposal
        };

        // Request an extension to the node for the specified group from the specific Node and ExtenderId.
        //
        // Should return the appropriate slotId for the newly added slots.
        virtual SlotId RequestExtension([[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const ExtenderId& extenderId, ExtensionRequestReason)
        {
            return SlotId();
        }

        virtual void ExtensionCancelled([[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const ExtenderId& extenderId)
        {
        }

        virtual void FinalizeExtension([[maybe_unused]] const NodeId& nodeId, [[maybe_unused]] const ExtenderId& extenderId)
        {
        }
        ////
        
        //////////////////////////////////////
        // Node Wrapper Optional Overrides

        // Returns whether or not the specified wrapper node should accept the given drop
        virtual bool ShouldWrapperAcceptDrop([[maybe_unused]] const NodeId& wrapperNode, [[maybe_unused]] const QMimeData* mimeData) const
        {
            AZ_Error("GraphCanvas", false, "Trying to use Node Wrappers without providing model information. Please implement 'ShouldWrapperAcceptDrop' on the GraphModelRequestBus.");
            return false;
        }

        // Signals out that we want to drop onto the specified wrapper node
        virtual void AddWrapperDropTarget([[maybe_unused]] const NodeId& wrapperNode)
        {
            AZ_Error("GraphCanvas", false, "Trying to use Node Wrappers without providing model information. Please implement 'AddWrapperDropTarget' on the GraphModelRequestBus.");
        };

        // Signals out that we no longer wish to drop onto the specified wrapper node
        virtual void RemoveWrapperDropTarget(const NodeId& wrapperNode)
        {
            AZ_UNUSED(wrapperNode);
            AZ_Error("GraphCanvas", false, "Trying to use Node Wrappers without providing model information. Please implement 'RemoveWrapperDropTarget' on the GraphModelRequestBus.");
        }
        //////////////////////////////////////
    };

    using GraphModelRequestBus = AZ::EBus<GraphModelRequests>;

    class GraphModelNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = GraphId;        
    };

    using GraphModelNotificationBus = AZ::EBus<GraphModelNotifications>;
}

DECLARE_EBUS_EXTERN(GraphCanvas::GraphModelRequests);
