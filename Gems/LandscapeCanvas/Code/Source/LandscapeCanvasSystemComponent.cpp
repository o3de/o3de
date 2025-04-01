/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <LyViewPaneNames.h>

// LmbrCentral
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/ReferenceShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/TubeShapeComponentBus.h>

// Gradient Signal
#include <GradientSignal/Editor/EditorGradientTypeIds.h>

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
#include <Editor/Nodes/Gradients/GradientBakerNode.h>
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
#include <Editor/Nodes/Shapes/AxisAlignedBoxShapeNode.h>
#include <Editor/Nodes/Shapes/BoxShapeNode.h>
#include <Editor/Nodes/Shapes/CapsuleShapeNode.h>
#include <Editor/Nodes/Shapes/CompoundShapeNode.h>
#include <Editor/Nodes/Shapes/CylinderShapeNode.h>
#include <Editor/Nodes/Shapes/DiskShapeNode.h>
#include <Editor/Nodes/Shapes/PolygonPrismShapeNode.h>
#include <Editor/Nodes/Shapes/ReferenceShapeNode.h>
#include <Editor/Nodes/Shapes/SphereShapeNode.h>
#include <Editor/Nodes/Shapes/TubeShapeNode.h>
#include <Editor/Nodes/Terrain/PhysXHeightfieldColliderNode.h>
#include <Editor/Nodes/Terrain/TerrainHeightGradientListNode.h>
#include <Editor/Nodes/Terrain/TerrainLayerSpawnerNode.h>
#include <Editor/Nodes/Terrain/TerrainMacroMaterialNode.h>
#include <Editor/Nodes/Terrain/TerrainPhysicsHeightfieldColliderNode.h>
#include <Editor/Nodes/Terrain/TerrainSurfaceGradientListNode.h>
#include <Editor/Nodes/Terrain/TerrainSurfaceMaterialsListNode.h>

namespace LandscapeCanvas
{
    namespace Internal
    {
        // The FastNoise gem is optional, so we need to keep track of its component type ID
        // ourselves since we can't rely on the headers being there.
        static constexpr AZ::TypeId EditorFastNoiseGradientComponentTypeId{ "{FD018DE5-5EB4-4219-9D0C-CB3C55DE656B}" };

        // The Terrain gem is optional, so we need to keep track of the component type IDs
        // ourselves since we can't rely on the headers being there.
        namespace Terrain
        {
            static constexpr AZ::TypeId EditorPhysXHeightfieldColliderComponentTypeId{ "{C388C3DB-8D2E-4D26-96D3-198EDC799B77}" };
            static constexpr AZ::TypeId EditorTerrainHeightGradientListComponentTypeId{ "{2D945B90-ADAB-4F9A-A113-39E714708068}" };
            static constexpr AZ::TypeId EditorTerrainLayerSpawnerComponentTypeId{ "{9403FC94-FA38-4387-BEFD-A728C7D850C1}" };
            static constexpr AZ::TypeId EditorTerrainMacroMaterialComponentTypeId{ "{24D87D5F-6845-4F1F-81DC-05B4CEBA3EF4}" };
            static constexpr AZ::TypeId EditorTerrainPhysicsHeightfieldColliderComponentTypeId{ "{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}" };
            static constexpr AZ::TypeId EditorTerrainSurfaceGradientListComponentTypeId{ "{49831E91-A11F-4EFF-A824-6D85C284B934}" };
            static constexpr AZ::TypeId EditorTerrainSurfaceMaterialsListComponentTypeId{ "{335CDED5-2E76-4342-8675-A60F66C471BF}" };
        }

        // The Vegetation gem is optional, so we need to keep track of the component type IDs
        // ourselves since we can't rely on the headers being there.
        namespace Vegetation
        {
            static constexpr AZ::TypeId EditorAreaBlenderComponentTypeId{ "{374A5C69-A252-4C4B-AE10-A673EF7AFE82}" };
            static constexpr AZ::TypeId EditorBlockerComponentTypeId{ "{9E765835-9CEB-4AEC-A913-787D3D21451D}" };
            static constexpr AZ::TypeId EditorMeshBlockerComponentTypeId{ "{130F5DFF-EF6F-4B37-8717-194876DE12DB}" };
            static constexpr AZ::TypeId EditorSpawnerComponentTypeId{ "{DD96FD51-A86B-48BC-A6AB-89183B538269}" };
            static constexpr AZ::TypeId EditorDistanceBetweenFilterComponentTypeId{ "{78DE1245-7023-40D6-B365-CC45EB4CE622}" };
            static constexpr AZ::TypeId EditorDistributionFilterComponentTypeId{ "{8EDD1DA2-B597-4BCE-9285-C68886504EC7}" };
            static constexpr AZ::TypeId EditorShapeIntersectionFilterComponentTypeId{ "{8BCE1190-6681-4C27-834A-AFFC8FBBDCD1}" };
            static constexpr AZ::TypeId EditorSurfaceAltitudeFilterComponentTypeId{ "{CD722D14-9C3B-4F89-B695-65B584279EB3}" };
            static constexpr AZ::TypeId EditorSurfaceMaskDepthFilterComponentTypeId{ "{A5441713-89DF-49C1-BA4E-3429FF23B43F}" };
            static constexpr AZ::TypeId EditorSurfaceMaskFilterComponentTypeId{ "{D2F223B4-60BE-4AC5-A1AA-260B91119918}" };
            static constexpr AZ::TypeId EditorSurfaceSlopeFilterComponentTypeId{ "{5130DA4B-6586-4249-9B86-6496EB2B1A78}" };
            static constexpr AZ::TypeId EditorPositionModifierComponentTypeId{ "{E1A2D544-B54A-437F-A40D-1FA5C5999D1C}" };
            static constexpr AZ::TypeId EditorRotationModifierComponentTypeId{ "{6E4B91BC-DAD7-4630-A78C-261D96EEA979}" };
            static constexpr AZ::TypeId EditorScaleModifierComponentTypeId{ "{D2391F8A-BB54-463E-9691-9290A802C6DE}" };
            static constexpr AZ::TypeId EditorSlopeAlignmentModifierComponentTypeId{ "{B0C62968-562B-4A8C-9969-E2AAB5379F66}" };
            static constexpr AZ::TypeId EditorDescriptorWeightSelectorComponentTypeId{ "{0FB90550-149B-4E05-B22C-2753F6526E97}" };
        }
    }

    // Define all of our supported nodes with their corresponding Component TypeId
    // so we can use these mappings for registration and factory method creation
#define LANDSCAPE_CANVAS_NODE_TABLE(VISITOR_FUNCTION, ...)    \
    /* Area nodes */    \
    VISITOR_FUNCTION<AreaBlenderNode>(Internal::Vegetation::EditorAreaBlenderComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<BlockerAreaNode>(Internal::Vegetation::EditorBlockerComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<MeshBlockerAreaNode>(Internal::Vegetation::EditorMeshBlockerComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SpawnerAreaNode>(Internal::Vegetation::EditorSpawnerComponentTypeId, ##__VA_ARGS__);     \
    /* Area filter nodes */ \
    VISITOR_FUNCTION<AltitudeFilterNode>(Internal::Vegetation::EditorSurfaceAltitudeFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<DistanceBetweenFilterNode>(Internal::Vegetation::EditorDistanceBetweenFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<DistributionFilterNode>(Internal::Vegetation::EditorDistributionFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ShapeIntersectionFilterNode>(Internal::Vegetation::EditorShapeIntersectionFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SlopeFilterNode>(Internal::Vegetation::EditorSurfaceSlopeFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SurfaceMaskDepthFilterNode>(Internal::Vegetation::EditorSurfaceMaskDepthFilterComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SurfaceMaskFilterNode>(Internal::Vegetation::EditorSurfaceMaskFilterComponentTypeId, ##__VA_ARGS__);     \
    /* Area modifier nodes */    \
    VISITOR_FUNCTION<PositionModifierNode>(Internal::Vegetation::EditorPositionModifierComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<RotationModifierNode>(Internal::Vegetation::EditorRotationModifierComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ScaleModifierNode>(Internal::Vegetation::EditorScaleModifierComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SlopeAlignmentModifierNode>(Internal::Vegetation::EditorSlopeAlignmentModifierComponentTypeId, ##__VA_ARGS__);     \
    /* Area selector nodes */    \
    VISITOR_FUNCTION<AssetWeightSelectorNode>(Internal::Vegetation::EditorDescriptorWeightSelectorComponentTypeId, ##__VA_ARGS__);     \
    /* Shape nodes */    \
    VISITOR_FUNCTION<AxisAlignedBoxShapeNode>(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<BoxShapeNode>(LmbrCentral::EditorBoxShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<CapsuleShapeNode>(LmbrCentral::EditorCapsuleShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<CompoundShapeNode>(LmbrCentral::EditorCompoundShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<CylinderShapeNode>(LmbrCentral::EditorCylinderShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<DiskShapeNode>(LmbrCentral::EditorDiskShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<PolygonPrismShapeNode>(LmbrCentral::EditorPolygonPrismShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ReferenceShapeNode>(LmbrCentral::EditorReferenceShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<SphereShapeNode>(LmbrCentral::EditorSphereShapeComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TubeShapeNode>(LmbrCentral::EditorTubeShapeComponentTypeId, ##__VA_ARGS__);     \
    /* Terrain nodes */    \
    VISITOR_FUNCTION<PhysXHeightfieldColliderNode>(Internal::Terrain::EditorPhysXHeightfieldColliderComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TerrainHeightGradientListNode>(Internal::Terrain::EditorTerrainHeightGradientListComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TerrainLayerSpawnerNode>(Internal::Terrain::EditorTerrainLayerSpawnerComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TerrainMacroMaterialNode>(Internal::Terrain::EditorTerrainMacroMaterialComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TerrainPhysicsHeightfieldColliderNode>(Internal::Terrain::EditorTerrainPhysicsHeightfieldColliderComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TerrainSurfaceGradientListNode>(Internal::Terrain::EditorTerrainSurfaceGradientListComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<TerrainSurfaceMaterialsListNode>(Internal::Terrain::EditorTerrainSurfaceMaterialsListComponentTypeId, ##__VA_ARGS__);     \
    /* Gradient generator nodes */    \
    VISITOR_FUNCTION<FastNoiseGradientNode>(Internal::EditorFastNoiseGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<PerlinNoiseGradientNode>(GradientSignal::EditorPerlinGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<RandomNoiseGradientNode>(GradientSignal::EditorRandomGradientComponentTypeId, ##__VA_ARGS__);     \
    /* Gradient nodes */    \
    VISITOR_FUNCTION<AltitudeGradientNode>(GradientSignal::EditorSurfaceAltitudeGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<ConstantGradientNode>(GradientSignal::EditorConstantGradientComponentTypeId, ##__VA_ARGS__);     \
    VISITOR_FUNCTION<GradientBakerNode>(GradientSignal::EditorGradientBakerComponentTypeId, ##__VA_ARGS__);     \
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

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    LandscapeCanvasSystemComponent::~LandscapeCanvasSystemComponent()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        LandscapeCanvas::GraphContext::SetInstance(nullptr);
        AzToolsFramework::CloseViewPane(LyViewPane::LandscapeCanvas);
        AzToolsFramework::UnregisterViewPane(LyViewPane::LandscapeCanvas);
    }

    void LandscapeCanvasSystemComponent::OnActionContextRegistrationHook()
    {
        static constexpr AZStd::string_view LandscapeCanvasActionContextIdentifier = "o3de.context.editor.landscapecanvas";

        if (auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
        {
            AzToolsFramework::ActionContextProperties contextProperties;
            contextProperties.m_name = "O3DE Landscape Canvas";

            // Register a custom action context to allow duplicated shortcut hotkeys to work
            actionManagerInterface->RegisterActionContext(LandscapeCanvasActionContextIdentifier, contextProperties);
        }
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
        provided.push_back(AZ_CRC_CE("LandscapeCanvasService"));
    }

    void LandscapeCanvasSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LandscapeCanvasService"));
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

        return it->second.first;
    }

    int LandscapeCanvasSystemComponent::GetNodeRegisteredIndex(const AZ::TypeId& nodeTypeId) const
    {
        auto it = m_nodeComponentTypeIds.find(nodeTypeId);
        if (it == m_nodeComponentTypeIds.end())
        {
            return -1;
        }

        return it->second.second;
    }

    const LandscapeCanvasSerialization& LandscapeCanvasSystemComponent::GetSerializedMappings()
    {
        return m_serialization;
    }

    void LandscapeCanvasSystemComponent::SetDeserializedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& entities)
    {
        m_serialization.m_deserializedEntities = entities;
    }
}
