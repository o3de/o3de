/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <LyViewPaneNames.h>

// LmbrCentral
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/TubeShapeComponentBus.h>

// Gradient Signal
#include <GradientSignal/Editor/EditorGradientTypeIds.h>

// Vegetation
#include <Vegetation/Editor/EditorVegetationComponentTypeIds.h>

// Graph Model
#include <GraphModel/Integration/NodePalette/StandardNodePaletteItem.h>

// Landscape Canvas
#include "LandscapeCanvasSystemComponent.h"
#include <Editor/MainWindow.h>
#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/Areas/AreaBlenderNode.h>
#include <Editor/Nodes/Areas/BlockerAreaNode.h>
#include <Editor/Nodes/Areas/MeshBlockerAreaNode.h>
#include <Editor/Nodes/Areas/SpawnerAreaNode.h>
#include <Editor/Nodes/AreaFilters/AltitudeFilterNode.h>
#include <Editor/Nodes/AreaFilters/DistanceBetweenFilterNode.h>
#include <Editor/Nodes/AreaFilters/DistributionFilterNode.h>
#include <Editor/Nodes/AreaFilters/ShapeIntersectionFilterNode.h>
#include <Editor/Nodes/AreaFilters/SlopeFilterNode.h>
#include <Editor/Nodes/AreaFilters/SurfaceMaskDepthFilterNode.h>
#include <Editor/Nodes/AreaFilters/SurfaceMaskFilterNode.h>
#include <Editor/Nodes/AreaModifiers/PositionModifierNode.h>
#include <Editor/Nodes/AreaModifiers/RotationModifierNode.h>
#include <Editor/Nodes/AreaModifiers/ScaleModifierNode.h>
#include <Editor/Nodes/AreaModifiers/SlopeAlignmentModifierNode.h>
#include <Editor/Nodes/AreaSelectors/AssetWeightSelectorNode.h>
#include <Editor/Nodes/Gradients/AltitudeGradientNode.h>
#include <Editor/Nodes/Gradients/ConstantGradientNode.h>
#include <Editor/Nodes/Gradients/FastNoiseGradientNode.h>
#include <Editor/Nodes/Gradients/ImageGradientNode.h>
#include <Editor/Nodes/Gradients/PerlinNoiseGradientNode.h>
#include <Editor/Nodes/Gradients/RandomNoiseGradientNode.h>
#include <Editor/Nodes/Gradients/ShapeAreaFalloffGradientNode.h>
#include <Editor/Nodes/Gradients/SlopeGradientNode.h>
#include <Editor/Nodes/Gradients/SurfaceMaskGradientNode.h>
#include <Editor/Nodes/GradientModifiers/DitherGradientModifierNode.h>
#include <Editor/Nodes/GradientModifiers/GradientMixerNode.h>
#include <Editor/Nodes/GradientModifiers/InvertGradientModifierNode.h>
#include <Editor/Nodes/GradientModifiers/LevelsGradientModifierNode.h>
#include <Editor/Nodes/GradientModifiers/PosterizeGradientModifierNode.h>
#include <Editor/Nodes/GradientModifiers/SmoothStepGradientModifierNode.h>
#include <Editor/Nodes/GradientModifiers/ThresholdGradientModifierNode.h>
#include <Editor/Nodes/Shapes/BoxShapeNode.h>
#include <Editor/Nodes/Shapes/CapsuleShapeNode.h>
#include <Editor/Nodes/Shapes/CompoundShapeNode.h>
#include <Editor/Nodes/Shapes/CylinderShapeNode.h>
#include <Editor/Nodes/Shapes/DiskShapeNode.h>
#include <Editor/Nodes/Shapes/PolygonPrismShapeNode.h>
#include <Editor/Nodes/Shapes/SphereShapeNode.h>
#include <Editor/Nodes/Shapes/TubeShapeNode.h>

namespace LandscapeCanvas
{
    // Define all of our supported nodes with their corresponding Component TypeId
    // so we can use these mappings for registration and factory method creation
#define LANDSCAPE_CANVAS_NODE_TABLE(VISITOR_FUNCTION, ...)    \
    /* Area nodes */    \
    VISITOR_FUNCTION<AreaBlenderNode>(Vegetation::EditorAreaBlenderComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<BlockerAreaNode>(Vegetation::EditorBlockerComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<MeshBlockerAreaNode>(Vegetation::EditorMeshBlockerComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SpawnerAreaNode>(Vegetation::EditorSpawnerComponentTypeId, ##__VA_ARGS__);     \
    /* Area filter nodes */ \
    VISITOR_FUNCTION<AltitudeFilterNode>(Vegetation::EditorSurfaceAltitudeFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<DistanceBetweenFilterNode>(Vegetation::EditorDistanceBetweenFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<DistributionFilterNode>(Vegetation::EditorDistributionFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ShapeIntersectionFilterNode>(Vegetation::EditorShapeIntersectionFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SlopeFilterNode>(Vegetation::EditorSurfaceSlopeFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SurfaceMaskDepthFilterNode>(Vegetation::EditorSurfaceMaskDepthFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SurfaceMaskFilterNode>(Vegetation::EditorSurfaceMaskFilterComponentTypeId, ##__VA_ARGS__);     \
    /* Area modifier nodes */    \
    VISITOR_FUNCTION<PositionModifierNode>(Vegetation::EditorPositionModifierComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<RotationModifierNode>(Vegetation::EditorRotationModifierComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ScaleModifierNode>(Vegetation::EditorScaleModifierComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SlopeAlignmentModifierNode>(Vegetation::EditorSlopeAlignmentModifierComponentTypeId, ##__VA_ARGS__);     \
    /* Area selector nodes */    \
    VISITOR_FUNCTION<AssetWeightSelectorNode>(Vegetation::EditorDescriptorWeightSelectorComponentTypeId, ##__VA_ARGS__);     \
    /* Shape nodes */    \
    VISITOR_FUNCTION<BoxShapeNode>(LmbrCentral::EditorBoxShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<CapsuleShapeNode>(LmbrCentral::EditorCapsuleShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<CompoundShapeNode>(LmbrCentral::EditorCompoundShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<CylinderShapeNode>(LmbrCentral::EditorCylinderShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<DiskShapeNode>(LmbrCentral::EditorDiskShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<PolygonPrismShapeNode>(LmbrCentral::EditorPolygonPrismShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SphereShapeNode>(LmbrCentral::EditorSphereShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TubeShapeNode>(LmbrCentral::EditorTubeShapeComponentTypeId, ##__VA_ARGS__);     \
    /* Gradient generator nodes */    \
    VISITOR_FUNCTION<FastNoiseGradientNode>(EditorFastNoiseGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<PerlinNoiseGradientNode>(GradientSignal::EditorPerlinGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<RandomNoiseGradientNode>(GradientSignal::EditorRandomGradientComponentTypeId, ##__VA_ARGS__);     \
    /* Gradient nodes */    \
    VISITOR_FUNCTION<AltitudeGradientNode>(GradientSignal::EditorSurfaceAltitudeGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ConstantGradientNode>(GradientSignal::EditorConstantGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ImageGradientNode>(GradientSignal::EditorImageGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ShapeAreaFalloffGradientNode>(GradientSignal::EditorShapeAreaFalloffGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SlopeGradientNode>(GradientSignal::EditorSurfaceSlopeGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SurfaceMaskGradientNode>(GradientSignal::EditorSurfaceMaskGradientComponentTypeId, ##__VA_ARGS__);     \
    /* Gradient modifier nodes */    \
    VISITOR_FUNCTION<DitherGradientModifierNode>(GradientSignal::EditorDitherGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<GradientMixerNode>(GradientSignal::EditorMixedGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<InvertGradientModifierNode>(GradientSignal::EditorInvertGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<LevelsGradientModifierNode>(GradientSignal::EditorLevelsGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<PosterizeGradientModifierNode>(GradientSignal::EditorPosterizeGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SmoothStepGradientModifierNode>(GradientSignal::EditorSmoothStepGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ThresholdGradientModifierNode>(GradientSignal::EditorThresholdGradientComponentTypeId, ##__VA_ARGS__);     \

    template<typename NodeType>
    void RegisterNode([[maybe_unused]] const AZ::TypeId& typeId, AZ::ReflectContext* context)
    {
        GraphModelIntegration::ReflectAndCreateNodeMimeEvent<NodeType>(context);
    }

    void LandscapeCanvasSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 1280, 1024);
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/Menu/landscape_canvas_editor.svg";

        AzToolsFramework::RegisterViewPane<LandscapeCanvasEditor::MainWindow>(LyViewPane::LandscapeCanvas, LyViewPane::CategoryTools, options);
    }

    LandscapeCanvasSystemComponent::LandscapeCanvasSystemComponent()
    {
        LandscapeCanvas::GraphContext::SetInstance(AZStd::make_shared<LandscapeCanvas::GraphContext>());

        // Register factory methods for creating for all supported nodes
        LANDSCAPE_CANVAS_NODE_TABLE(RegisterFactoryMethod);
    }

    LandscapeCanvasSystemComponent::~LandscapeCanvasSystemComponent()
    {
        LandscapeCanvas::GraphContext::SetInstance(nullptr);
        AzToolsFramework::CloseViewPane(LyViewPane::LandscapeCanvas);
        AzToolsFramework::UnregisterViewPane(LyViewPane::LandscapeCanvas);
    }

    void LandscapeCanvasSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect all our base node types so they can be serialized/deserialized, since
        // the below LANDSCAPE_CANVAS_NODE_TABLE macro only reflects the actual concrete classes
        BaseNode::Reflect(context);
        BaseAreaFilterNode::Reflect(context);
        BaseAreaModifierNode::Reflect(context);
        BaseAreaNode::Reflect(context);
        BaseGradientModifierNode::Reflect(context);
        BaseGradientNode::Reflect(context);
        BaseShapeNode::Reflect(context);

        // Reflect and create the node mime events for all our supported nodes
        LANDSCAPE_CANVAS_NODE_TABLE(RegisterNode, context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LandscapeCanvasSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LandscapeCanvasSystemComponent>("LandscapeCanvas", "Graph canvas representation of Dynamic Vegetation")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("LANDSCAPE_CANVAS_EDITOR_ID", []() { return LANDSCAPE_CANVAS_EDITOR_ID; })
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

            behaviorContext->EBus<LandscapeCanvasNodeFactoryRequestBus>("LandscapeCanvasNodeFactoryRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "landscapecanvas")
                ->Event("CreateNodeForTypeName", &LandscapeCanvasNodeFactoryRequests::CreateNodeForTypeName)
                ;

            behaviorContext->EBus<LandscapeCanvasRequestBus>("LandscapeCanvasRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "landscapecanvas")
                ->Event("OnGraphEntity", &LandscapeCanvasRequests::OnGraphEntity)
                ->Event("GetNodeMatchingEntityInGraph", &LandscapeCanvasRequests::GetNodeMatchingEntityInGraph)
                ->Event("GetNodeMatchingEntityComponentInGraph", &LandscapeCanvasRequests::GetNodeMatchingEntityComponentInGraph)
                ->Event("GetAllNodesMatchingEntity", &LandscapeCanvasRequests::GetAllNodesMatchingEntity)
                ->Event("GetAllNodesMatchingEntityComponent", &LandscapeCanvasRequests::GetAllNodesMatchingEntityComponent)
                ;
        }
    }

#undef LANDSCAPE_CANVAS_NODE_TABLE

    void LandscapeCanvasSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LandscapeCanvasService", 0x31668887));
    }

    void LandscapeCanvasSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LandscapeCanvasService", 0x31668887));
    }

    void LandscapeCanvasSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void LandscapeCanvasSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void LandscapeCanvasSystemComponent::Init()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
    }

    void LandscapeCanvasSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        LandscapeCanvasNodeFactoryRequestBus::Handler::BusConnect();
        LandscapeCanvasSerializationRequestBus::Handler::BusConnect();
    }

    void LandscapeCanvasSystemComponent::Deactivate()
    {
        LandscapeCanvasSerializationRequestBus::Handler::BusDisconnect();
        LandscapeCanvasNodeFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    BaseNode::BaseNodePtr LandscapeCanvasSystemComponent::CreateNodeForType(GraphModel::GraphPtr graph, const AZ::TypeId& typeId)
    {
        auto it = m_nodeFactory.find(typeId);
        if (it == m_nodeFactory.end())
        {
            return nullptr;
        }

        return it->second(graph);
    }

    GraphModel::NodePtr LandscapeCanvasSystemComponent::CreateNodeForTypeName(GraphModel::GraphPtr graph, AZStd::string_view nodeName)
    {
        AZ::TypeId nodeTypeId = AZ::TypeId::CreateNull();

        // Search through all registered Nodes to find the TypeId for one that
        // matches the requested class name.
        m_serializeContext->EnumerateDerived<GraphModel::Node>(
            [&nodeTypeId, nodeName](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
        {
            AZ_UNUSED(knownType);

            if (componentClass->m_name == nodeName)
            {
                nodeTypeId = componentClass->m_typeId;
                return false;
            }

            return true;
        });

        if (!nodeTypeId.IsNull())
        {
            AZ::TypeId componentTypeId = GetComponentTypeId(nodeTypeId);
            return CreateNodeForType(graph, componentTypeId);
        }

        return nullptr;
    }

    const AZ::TypeId LandscapeCanvasSystemComponent::GetComponentTypeId(const AZ::TypeId& nodeTypeId)
    {
        auto it = m_nodeComponentTypeIds.find(nodeTypeId);
        if (it == m_nodeComponentTypeIds.end())
        {
            return AZ::TypeId();
        }

        return it->second;
    }

    const LandscapeCanvasSerialization& LandscapeCanvasSystemComponent::GetSerializedMappings()
    {
        return m_serialization;
    }

    void LandscapeCanvasSystemComponent::SetSerializedNodeEntities(const AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& nodeEntities)
    {
        // Delete any entities we had previously serialized before updating our mappings
        for (auto it : m_serialization.m_serializedNodeEntities)
        {
            delete it.second;
        }

        m_serialization.m_serializedNodeEntities = nodeEntities;
    }

    void LandscapeCanvasSystemComponent::SetDeserializedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& entities)
    {
        m_serialization.m_deserializedEntities = entities;
    }
}
