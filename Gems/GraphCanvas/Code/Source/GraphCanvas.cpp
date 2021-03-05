/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <precompiled.h>
#include <GraphCanvas.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Components/BookmarkManagerComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorLayerControllerComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorVisualComponent.h>
#include <Components/GeometryComponent.h>
#include <Components/GridComponent.h>
#include <Components/GridVisualComponent.h>
#include <Components/PersistentIdComponent.h>
#include <Components/SceneComponent.h>
#include <Components/SceneMemberComponent.h>
#include <Components/StylingComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/Connections/DataConnections/DataConnectionComponent.h>
#include <Components/Connections/DataConnections/DataConnectionVisualComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/Comment/CommentNodeLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/Group/CollapsedNodeGroupComponent.h>
#include <Components/Nodes/Group/NodeGroupFrameComponent.h>
#include <Components/Nodes/Group/NodeGroupLayoutComponent.h>
#include <Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <Components/NodePropertyDisplays/AssetIdNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/BooleanNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/ComboBoxNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/EntityIdNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/NumericNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/ReadOnlyNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/StringNodePropertyDisplay.h>
#include <Components/NodePropertyDisplays/VectorNodePropertyDisplay.h>

#include <Components/Slots/Data/DataSlotComponent.h>
#include <Components/Slots/Execution/ExecutionSlotComponent.h>
#include <Components/Slots/Extender/ExtenderSlotComponent.h>
#include <Components/Slots/Property/PropertySlotComponent.h>

#include <GraphCanvas/Styling/Selector.h>
#include <GraphCanvas/Styling/SelectorImplementations.h>
#include <GraphCanvas/Styling/PseudoElement.h>

#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/TranslationTypes.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <GraphCanvas/Widgets/MimeEvents/CreateSplicingNodeMimeEvent.h>

namespace GraphCanvas
{
    bool EntitySaveDataContainerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            AZ::Crc32 componentDataId = AZ_CRC("ComponentData", 0xa6acc628);
            AZStd::unordered_map< AZ::Uuid, GraphCanvas::ComponentSaveData* > componentSaveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(componentDataId);

            if (dataNode)
            {
                dataNode->GetDataHierarchy(context, componentSaveData);
            }

            classElement.RemoveElementByName(componentDataId);

            AZStd::unordered_map< AZ::Uuid, GraphCanvas::ComponentSaveData* > remappedComponentSaveData;

            for (const auto& mapPair : componentSaveData)
            {
                auto mapIter = remappedComponentSaveData.find(azrtti_typeid(mapPair.second));

                if (mapIter == remappedComponentSaveData.end())
                {
                    remappedComponentSaveData[azrtti_typeid(mapPair.second)] = mapPair.second;
                }
            }

            classElement.AddElementWithData(context, "ComponentData", remappedComponentSaveData);
        }

        return true;
    }

    void GraphCanvasSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphCanvasSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            // Reflect information for the stripped down saving
            serializeContext->Class<ComponentSaveData>()
                ->Version(1)
                ;

            serializeContext->Class<EntitySaveDataContainer>()
                ->Version(2, &EntitySaveDataContainerVersionConverter)
                ->Field("ComponentData", &EntitySaveDataContainer::m_entityData)
                ;
            ////

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphCanvasSystemComponent>(
                    "LmbrCentral", "Provides factory methods for Graph Canvas components")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }            
            
            NodeConfiguration::Reflect(serializeContext);
            Styling::SelectorImplementation::Reflect(serializeContext);
            Styling::Selector::Reflect(serializeContext);
            Styling::NullSelector::Reflect(serializeContext);
            Styling::BasicSelector::Reflect(serializeContext);
            Styling::DefaultSelector::Reflect(serializeContext);
            Styling::CompoundSelector::Reflect(serializeContext);
            Styling::NestedSelector::Reflect(serializeContext);
            TranslationKeyedString::Reflect(serializeContext);
            Styling::Style::Reflect(serializeContext);
            AssetEditorUserSettings::Reflect(serializeContext);
        }

        EditorConstructPresets::Reflect(context);
        GraphCanvasMimeEvent::Reflect(context);
        GraphCanvasTreeModel::Reflect(context);        
        CreateSplicingNodeMimeEvent::Reflect(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AssetEditorRequestBus>("AssetEditorRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "AssetEditor")
                ->Attribute(AZ::Script::Attributes::Module, EditorGraphModuleName)
                ->Event("CreateNewGraph", &AssetEditorRequests::CreateNewGraph)
                ->Event("ContainsGraph", &AssetEditorRequests::ContainsGraph)
                ->Event("CloseGraph", &AssetEditorRequests::CloseGraph)
                ;

            behaviorContext->EBus<SceneRequestBus>("SceneRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Scene")
                ->Attribute(AZ::Script::Attributes::Module, EditorGraphModuleName)
                ->Event("CutSelection", &SceneRequests::CutSelection)
                ->Event("CopySelection", &SceneRequests::CopySelection)
                ->Event("Paste", &SceneRequests::Paste)
                ->Event("DuplicateSelection", &SceneRequests::DuplicateSelection)
                ;
        }
    }

    void GraphCanvasSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(GraphCanvasRequestsServiceId);
    }

    void GraphCanvasSystemComponent::Activate()
    {
        GraphCanvasRequestBus::Handler::BusConnect();
        Styling::PseudoElementFactoryRequestBus::Handler::BusConnect();
    }

    void GraphCanvasSystemComponent::Deactivate()
    {
        Styling::PseudoElementFactoryRequestBus::Handler::BusDisconnect();
        GraphCanvasRequestBus::Handler::BusDisconnect();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateBookmarkAnchor() const
    {
        AZ::Entity* entity = aznew AZ::Entity("BookmarkAnchor");
        entity->CreateComponent<BookmarkAnchorComponent>();
        entity->CreateComponent<BookmarkAnchorVisualComponent>();
        entity->CreateComponent<SceneMemberComponent>();
        entity->CreateComponent<GeometryComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::BookmarkAnchor, AZ::EntityId());
        entity->CreateComponent<PersistentIdComponent>();
        entity->CreateComponent<BookmarkAnchorLayerControllerComponent>();

        return entity;
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateScene() const
    {
        // create a new empty canvas, give it a name to avoid serialization generating one based on
        // the ID (which in some cases caused diffs to fail in the editor)
        AZ::Entity* entity = aznew AZ::Entity("GraphCanvasScene");
        entity->CreateComponent<SceneComponent>();
        entity->CreateComponent<BookmarkManagerComponent>();
        return entity;
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCoreNode() const
    {
        return NodeComponent::CreateCoreNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateGeneralNode(const char* nodeType) const
    {
        return GeneralNodeLayoutComponent::CreateGeneralNodeEntity(nodeType);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCommentNode() const
    {
        return CommentNodeLayoutComponent::CreateCommentNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateWrapperNode(const char* nodeType) const
    {
        return WrapperNodeLayoutComponent::CreateWrapperNodeEntity(nodeType);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateNodeGroup() const
    {
        return NodeGroupLayoutComponent::CreateNodeGroupEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCollapsedNodeGroup(const CollapsedNodeGroupConfiguration& collapsedNodeGroupConfiguration) const
    {
        return CollapsedNodeGroupComponent::CreateCollapsedNodeGroupEntity(collapsedNodeGroupConfiguration);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateSlot(const AZ::EntityId& nodeId, const SlotConfiguration& slotConfiguration) const
    {
        if (const DataSlotConfiguration* dataSlotConfiguration = azrtti_cast<const DataSlotConfiguration*>(&slotConfiguration))
        {
            return DataSlotComponent::CreateDataSlot(nodeId, (*dataSlotConfiguration));
        }
        else if (const ExecutionSlotConfiguration* executionSlotConfiguration = azrtti_cast<const ExecutionSlotConfiguration*>(&slotConfiguration))
        {
            return ExecutionSlotComponent::CreateExecutionSlot(nodeId, (*executionSlotConfiguration));
        }
        else if (const ExtenderSlotConfiguration* extenderSlotConfiguration = azrtti_cast<const ExtenderSlotConfiguration*>(&slotConfiguration))
        {
            return ExtenderSlotComponent::CreateExtenderSlot(nodeId, (*extenderSlotConfiguration));
        }
        else
        {
            AZ_Error("GraphCanvas", false, "Trying to create using an unknown Slot Configuration");
        }

        return nullptr;
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateBooleanNodePropertyDisplay(BooleanDataInterface* dataInterface) const
    {
        return aznew BooleanNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateNumericNodePropertyDisplay(NumericDataInterface* dataInterface) const
    {
        return aznew NumericNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface) const
    {
        return aznew ComboBoxNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateEntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface) const
    {
        return aznew EntityIdNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface) const
    {
        return aznew ReadOnlyNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateStringNodePropertyDisplay(StringDataInterface* dataInterface) const
    {
        return aznew StringNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateVectorNodePropertyDisplay(VectorDataInterface* dataInterface) const
    {
        return aznew VectorNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateAssetIdNodePropertyDisplay(AssetIdDataInterface* dataInterface) const
    {
        return aznew AssetIdNodePropertyDisplay(dataInterface);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& configuration) const
    {
        return PropertySlotComponent::CreatePropertySlot(nodeId, propertyId, configuration);
    }

    AZ::EntityId GraphCanvasSystemComponent::CreateStyleEntity(const AZStd::string& style) const
    {
        return StylingComponent::CreateStyleEntity(style);
    }

    AZ::EntityId GraphCanvasSystemComponent::CreateVirtualChild(const AZ::EntityId& real, const AZStd::string& virtualChildElement) const
    {
        return Styling::VirtualChildElement::Create(real, virtualChildElement);
    }
}
