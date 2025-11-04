/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvasModule.h>
#include <GraphCanvas.h>

#include <Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorLayerControllerComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorVisualComponent.h>
#include <Components/BookmarkManagerComponent.h>
#include <Components/GeometryComponent.h>
#include <Components/GridComponent.h>
#include <Components/GridVisualComponent.h>
#include <Components/PersistentIdComponent.h>
#include <Components/SceneComponent.h>
#include <Components/SceneMemberComponent.h>
#include <Components/StylingComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Connections/ConnectionLayerControllerComponent.h>
#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/Connections/DataConnections/DataConnectionComponent.h>
#include <Components/Connections/DataConnections/DataConnectionVisualComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/NodeLayerControllerComponent.h>
#include <Components/Nodes/Comment/CommentLayerControllerComponent.h>
#include <Components/Nodes/Comment/CommentNodeFrameComponent.h>
#include <Components/Nodes/Comment/CommentNodeLayoutComponent.h>
#include <Components/Nodes/Comment/CommentNodeTextComponent.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeTitleComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/Group/CollapsedNodeGroupComponent.h>
#include <Components/Nodes/Group/NodeGroupLayoutComponent.h>
#include <Components/Nodes/Group/NodeGroupLayerControllerComponent.h>
#include <Components/Nodes/Group/NodeGroupFrameComponent.h>
#include <Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <Components/Slots/SlotComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <Components/Slots/Data/DataSlotComponent.h>
#include <Components/Slots/Data/DataSlotLayoutComponent.h>
#include <Components/Slots/Default/DefaultSlotLayoutComponent.h>
#include <Components/Slots/Execution/ExecutionSlotComponent.h>
#include <Components/Slots/Execution/ExecutionSlotLayoutComponent.h>
#include <Components/Slots/Extender/ExtenderSlotComponent.h>
#include <Components/Slots/Extender/ExtenderSlotLayoutComponent.h>
#include <Components/Slots/Property/PropertySlotComponent.h>
#include <Components/Slots/Property/PropertySlotLayoutComponent.h>

#include <GraphCanvas/Components/ColorPaletteManager/ColorPaletteManagerComponent.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Styling/PseudoElement.h>

namespace GraphCanvas
{
    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    GraphCanvasModule::GraphCanvasModule()
    {
        m_descriptors.insert(m_descriptors.end(), {

            // Components
            BookmarkManagerComponent::CreateDescriptor(),
            GraphCanvasPropertyComponent::CreateDescriptor(),
            GraphCanvasSystemComponent::CreateDescriptor(),
            LayerControllerComponent::CreateDescriptor(),
            PersistentIdComponent::CreateDescriptor(),
            SceneComponent::CreateDescriptor(),
            SceneMemberComponent::CreateDescriptor(),            

            // Background Grid
            GridComponent::CreateDescriptor(),
            GridVisualComponent::CreateDescriptor(),

            // BookmarkAnchor
            BookmarkAnchorComponent::CreateDescriptor(),
            BookmarkAnchorLayerControllerComponent::CreateDescriptor(),
            BookmarkAnchorVisualComponent::CreateDescriptor(),

            // General
            GeometryComponent::CreateDescriptor(),

            // Connections
            ConnectionComponent::CreateDescriptor(),
            ConnectionLayerControllerComponent::CreateDescriptor(),
            ConnectionVisualComponent::CreateDescriptor(),

            // Connections::DataConnections
            DataConnectionComponent::CreateDescriptor(),
            DataConnectionVisualComponent::CreateDescriptor(),

            // Nodes
            NodeComponent::CreateDescriptor(),
            NodeLayerControllerComponent::CreateDescriptor(),
            NodeLayoutComponent::CreateDescriptor(),

            // CommentNode
            CommentLayerControllerComponent::CreateDescriptor(),
            CommentNodeFrameComponent::CreateDescriptor(),
            CommentNodeLayoutComponent::CreateDescriptor(),
            CommentNodeTextComponent::CreateDescriptor(),

            // GeneralNode
            GeneralNodeTitleComponent::CreateDescriptor(),
            GeneralSlotLayoutComponent::CreateDescriptor(),
            GeneralNodeFrameComponent::CreateDescriptor(),
            GeneralNodeLayoutComponent::CreateDescriptor(),

            // NodeGroup
            CollapsedNodeGroupComponent::CreateDescriptor(),
            NodeGroupLayerControllerComponent::CreateDescriptor(),
            NodeGroupLayoutComponent::CreateDescriptor(),
            NodeGroupFrameComponent::CreateDescriptor(),

            // Wrapper Node
            WrapperNodeLayoutComponent::CreateDescriptor(),

            // Slots
            SlotComponent::CreateDescriptor(),
            SlotConnectionFilterComponent::CreateDescriptor(),
            DefaultSlotLayoutComponent::CreateDescriptor(),

            // Data Slots
            DataSlotComponent::CreateDescriptor(),
            DataSlotLayoutComponent::CreateDescriptor(),

            // Execution Slots
            ExecutionSlotComponent::CreateDescriptor(),
            ExecutionSlotLayoutComponent::CreateDescriptor(),

            // Extender Slots
            ExtenderSlotComponent::CreateDescriptor(),
            ExtenderSlotLayoutComponent::CreateDescriptor(),

            // Property Slots
            PropertySlotComponent::CreateDescriptor(),
            PropertySlotLayoutComponent::CreateDescriptor(),

            // Styling
            StylingComponent::CreateDescriptor(),

            Styling::ComputedStyle::CreateDescriptor(),
            Styling::VirtualChildElement::CreateDescriptor(),

            // Deprecated Components
            Deprecated::ColorPaletteManagerComponent::CreateDescriptor(),
            Deprecated::StyleSheetComponent::CreateDescriptor()
        });
    }

    //! Request system components on the system entity.
    //! These components' memory is owned by the system entity.
    AZ::ComponentTypeList GraphCanvasModule::GetRequiredSystemComponents() const
    {
        return{
            azrtti_typeid<GraphCanvasSystemComponent>(),
        };
    }
    
}

// Qt resources are defined in the GraphCanvas static library, so we must
// initialize them manually
extern int qInitResources_GraphCanvasEditorResources();
extern int qCleanupResources_GraphCanvasEditorResources();
namespace {
    struct initializer {
        initializer() { qInitResources_GraphCanvasEditorResources(); }
        ~initializer() { qCleanupResources_GraphCanvasEditorResources(); }
    } dummy;
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), GraphCanvas::GraphCanvasModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_GraphCanvas_Editor, GraphCanvas::GraphCanvasModule)
#endif
