/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>

#include <GraphCanvas/Components/NodePropertyDisplay/AssetIdDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/BooleanDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/EntityIdDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/ReadOnlyDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/StringDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }
}

class QWidget;

namespace GraphCanvas
{
    struct Endpoint;

    static const AZ::Crc32 GraphCanvasRequestsServiceId = AZ_CRC_CE("GraphCanvasService");
    static constexpr const char* EditorGraphModuleName = "editor.graph";

    //! GraphCanvasRequests
    //! Factory methods that allow default configurations of the Graph Canvas entities to be created, ready for
    //! customization, e.g. in the setup methods for user-defined custom entities.
    class GraphCanvasRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        using EntityGroup = AZStd::vector<AZ::EntityId>;

        //! Create a Bookmark Anchor
        virtual AZ::Entity* CreateBookmarkAnchor() const = 0;

        //! Create and activate a Bookmark Anchor.
        virtual AZ::Entity* CreateBookmarkAnchorAndActivate() const
        {
            return InitActivateEntity(CreateBookmarkAnchor());
        }

        //! Create an empty scene.
        virtual AZ::Entity* CreateScene() const = 0;

        AZ::Entity* CreateSceneAndActivate() const
        {
            return InitActivateEntity(CreateScene());
        }

        //! Create a core node.
        //! This node will not be activated, and will be missing visual components. This contains just the logical
        //! backbone that we can share across nodes.
        virtual AZ::Entity* CreateCoreNode() const = 0;

        AZ::Entity* CreateCoreNodeAndActivate() const
        {
            return InitActivateEntity(CreateCoreNode());
        }

        //! Create a general node.
        //! The node will have a general layout, visual (including title) and no slots. It will be styled.
        //! This acts as a generic node, and a good starting point for most functionality.
        virtual AZ::Entity* CreateGeneralNode(const char* nodeType) const = 0;

        AZ::Entity* CreateGeneralNodeAndActivate(const char* nodeType) const
        {
            return InitActivateEntity(CreateGeneralNode(nodeType));
        }
        
        //! Create a comment node.
        virtual AZ::Entity* CreateCommentNode() const = 0;

        AZ::Entity* CreateCommentNodeAndActivate() const
        {
            return InitActivateEntity(CreateCommentNode());
        }

        virtual AZ::Entity* CreateNodeGroup() const = 0;

        AZ::Entity* CreateNodeGroupAndActivate() const
        {
            return InitActivateEntity(CreateNodeGroup());
        }

        virtual AZ::Entity* CreateCollapsedNodeGroup(const CollapsedNodeGroupConfiguration& groupedNodeConfiguration) const = 0;

        AZ::Entity* CreateCollapsedNodeGroupAndActivate(const CollapsedNodeGroupConfiguration& groupedNodeConfiguration)
        {
            return InitActivateEntity(CreateCollapsedNodeGroup(groupedNodeConfiguration));
        }

        //! Create a wrapper node.
        //! A wrapped node is node that can wrap other nodes to provide some extension of functionality.
        virtual AZ::Entity* CreateWrapperNode(const char* nodeType) const = 0;
        
        //! Create a wrapper node and activate it.
        //! A wrapped node is node that can wrap other nodes to provide some extension of functionality.
        AZ::Entity* CreateWrapperNodeAndActivate(const char* nodeType) const
        {
            return InitActivateEntity(CreateWrapperNode(nodeType));
        }

        virtual AZ::Entity* CreateSlot(const AZ::EntityId& nodeId, const SlotConfiguration& slotConfiguration) const = 0;

        //! Creates a BooleanNodeProperty display using the specified BooleanDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateBooleanNodePropertyDisplay(BooleanDataInterface* dataInterface) const = 0;

        //! Creates a DoubleNodeProperty display using the specified NumericDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateNumericNodePropertyDisplay(NumericDataInterface* dataInterface) const = 0;

        //! Creates a ComboBoxNodePropertyDisplay using the specified ComboBoxInterface
        //! param: dataInterface is the interface to the local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface) const = 0;

        //! Creates an EntityIdNodeProperty display using the specified EntityIdDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateEntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface) const = 0;

        //! Creates a ReadOnlyNodeProperty display using the specified ReadOnlyDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface) const = 0;

        //! Creates a StringNodeSlotProperty display using the specified StringDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateStringNodePropertyDisplay(StringDataInterface* dataInterface) const = 0;

        //! Creates a VectorNodeProperty display using the specified VectorDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateVectorNodePropertyDisplay(VectorDataInterface* dataInterface) const = 0;

        //! Creates an AssetIdNodeProperty display using the specified AssetIdDataInterface
        //! param: dataInterface is the interface to local data to be used in the operation of the NodePropertyDisplay.
        //! The PropertyDisplay will take ownership of the DataInterface
        virtual NodePropertyDisplay* CreateAssetIdNodePropertyDisplay(AssetIdDataInterface* dataInterface) const = 0;

        //! Create a property slot
        //! param: nodeId is the parent node
        //! param: propertyId is the id to identify the property
        //! param: slotConfiguration is the various configurable aspects of the slot.
        virtual AZ::Entity* CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& slotConfiguration) const = 0;

    private:
        AZ::Entity* InitActivateEntity(AZ::Entity* entity) const
        {
            entity->Init();
            entity->Activate();
            return entity;
        }
    };

    using GraphCanvasRequestBus = AZ::EBus<GraphCanvasRequests>;
}
