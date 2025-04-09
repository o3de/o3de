/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MainWindow.h"

// AZ
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzQtComponents/Buses/ShortcutDispatch.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <IEditor.h>

// Gradient Signal
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorRequestBus.h>

// Qt
#include <QApplication>
#include <QMessageBox>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

// GraphCanvas
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Editor/EditorDockWidgetBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>

// GraphModel
#include <GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>
#include <GraphModel/Integration/NodePalette/StandardNodePaletteItem.h>
#include <GraphModel/Integration/ReadOnlyDataInterface.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Core/GraphContext.h>
#include <Editor/Menus/LayerExtenderContextMenu.h>
#include <Editor/Menus/NodeContextMenu.h>
#include <Editor/Menus/SceneContextMenuActions.h>
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
#include <Editor/Nodes/UI/GradientPreviewThumbnailItem.h>
#include <EditorLandscapeCanvasComponent.h>

#include <LmbrCentral/Shape/ReferenceShapeComponentBus.h>

namespace LandscapeCanvasEditor
{
    static const int NODE_OFFSET_X_PIXELS = 350;
    static const int NODE_OFFSET_Y_PIXELS = 450;
    static constexpr int InvalidSlotIndex = -1;
    static const char* PreviewEntityElementName = "BoundsEntity";
    static const char* GradientIdElementName = "GradientId";
    static const char* GradientEntityIdElementName = "Gradient Entity";
    static const char* ShapeEntityIdElementName = "ShapeEntityId";
    static const char* InputBoundsEntityIdElementName = "InputBounds";
    static const char* EntityIdListElementName = "element";

    static IEditor* GetLegacyEditor()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequestBus::BroadcastResult(
            editor, &AzToolsFramework::EditorRequestBus::Events::GetEditor);
        return editor;
    }

    struct NodePoint
    {
        NodePoint* parent = nullptr;
        GraphModel::NodePtr node = nullptr;
        AZ::EntityId vegetationEntityId;
        AZStd::vector<NodePoint*> children;
    };

    NodePoint* FindNodePoint(const AZStd::vector<NodePoint*>& points, const AZStd::unordered_map<AZ::EntityId, GraphModel::NodePtrList>& nodeWrappings, GraphModel::NodePtr node)
    {
        for (auto it : points)
        {
            if (it->node == node)
            {
                return it;
            }
            // Wrapped nodes don't get their own NodePoint, so if we find a wrapper node
            // we need to check if any of its wrapped nodes match the node we are
            // looking for as well, since they will be in the same position as
            // their wrapper node parent
            else if (it->node->GetNodeType() == GraphModel::NodeType::WrapperNode)
            {
                const AZ::EntityId& entityId = it->vegetationEntityId;
                auto nodeWrapIt = nodeWrappings.find(entityId);
                if (nodeWrapIt != nodeWrappings.end())
                {
                    const auto& wrappedNodes = nodeWrapIt->second;
                    for (auto wrappedNode : wrappedNodes)
                    {
                        if (wrappedNode == node)
                        {
                            return it;
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    AZ::Vector2 PlaceNodes(const AZ::EntityId& sceneId, NodePoint* point, AZ::Vector2 offset)
    {
        if (!point)
        {
            return offset;
        }

        if (point->node)
        {
            GraphModelIntegration::GraphControllerRequestBus::Event(sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, point->node, offset);
            offset.SetX(offset.GetX() + NODE_OFFSET_X_PIXELS);
        }

        size_t numChildren = point->children.size();
        if (numChildren)
        {
            for (int i = 0; i < numChildren; ++i)
            {
                // Update the y-coordinate of our offset from any nodes placed by our child so that
                // any subsequent nodes will be placed below them
                AZ::Vector2 childOffset = PlaceNodes(sceneId, point->children[i], offset);
                offset.SetY(childOffset.GetY());

                // Start a new "row" if this node has any more children that need room
                if (i < numChildren - 1)
                {
                    offset.SetY(offset.GetY() + NODE_OFFSET_Y_PIXELS);
                }
            }
        }

        return offset;
    }

    AZ::TypeId PickComponentTypeIdToAdd(const AzToolsFramework::ComponentPaletteUtil::ComponentDataTable& componentDataTable)
    {
        using namespace AzToolsFramework;

        // A map of category names with preferred component names.
        // There may be multiple component names for a category, as long as they provide different services.
        const AZStd::map<QString, AZStd::vector<QString>> preferredComponentsByCategory = { { "Shape", { "Shape Reference" } } };

        // Scan through the preferred categories to see whether any exist in the componentDataTable.
        for (const auto& preferredComponentPair : preferredComponentsByCategory)
        {
            auto candidateDataTablePair = componentDataTable.find(preferredComponentPair.first);
            if (candidateDataTablePair != componentDataTable.end())
            {
                // Now check all the preferred components for that category, and return the first one that exists in the candidate componentDataTable.
                for (const auto& preferredComponentName : preferredComponentPair.second)
                {
                    const auto& candidateComponent = candidateDataTablePair->second.find(preferredComponentName);
                    if (candidateComponent != candidateDataTablePair->second.end())
                    {
                        return candidateComponent->second->m_typeId;
                    }
                }
            }
        }

        // There are a couple of cases where we prefer certain categories of Components
        // to be added over others,
        // so if those there are components in those categories, then choose them first.
        // Otherwise, just pick the first one in the list.
        static const QStringList preferredCategories = { "Vegetation", "Graphics/Mesh" };

        ComponentPaletteUtil::ComponentDataTable::const_iterator categoryIt;
        for (const auto& categoryName : preferredCategories)
        {
            categoryIt = componentDataTable.find(categoryName);
            if (categoryIt != componentDataTable.end())
            {
                break;
            }
        }
        if (categoryIt == componentDataTable.end())
        {
            categoryIt = componentDataTable.begin();
        }

        AZ_Assert(categoryIt->second.size(), "No components found that satisfy the missing required service(s).");

        const auto& componentPair = categoryIt->second.begin();
        return componentPair->second->m_typeId;
    }

    CustomEntityPropertyEditor::CustomEntityPropertyEditor(QWidget* parent)
        : AzToolsFramework::EntityPropertyEditor(parent)
    {
    }

    void CustomEntityPropertyEditor::CloseInspectorWindow()
    {
        // Override this to be empty, since our custom instance of this pinned inspector
        // doesn't need to be closed when the context resets
    }

    QString CustomEntityPropertyEditor::GetEntityDetailsLabelText() const
    {
        return QObject::tr("Select a node to show its properties in the inspector.");
    }

    CustomNodeInspectorDockWidget::CustomNodeInspectorDockWidget(QWidget* parent)
        : AzQtComponents::StyledDockWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout();

        // Our custom Node Inspector is just a Pinned Inspector that by default is
        // pointed to an invalid EntityId, so it won't follow the Editor selection
        m_propertyEditor = new CustomEntityPropertyEditor(this);
        m_propertyEditor->SetOverrideEntityIds({ AZ::EntityId() });
        layout->addWidget(m_propertyEditor);

        QWidget* host = new QWidget(this);
        host->setLayout(layout);
        setWidget(host);

        setObjectName("TempNodeInspector");
        setWindowTitle(QObject::tr("Node Inspector"));
    }

    CustomEntityPropertyEditor* CustomNodeInspectorDockWidget::GetEntityPropertyEditor()
    {
        return m_propertyEditor;
    }

#define REGISTER_NODE_PALETTE_ITEM(category, TYPE, editorId) \
    category->CreateChildNode<GraphModelIntegration::StandardNodePaletteItem<TYPE>>(TYPE::TITLE, editorId);

    GraphCanvas::GraphCanvasTreeItem* LandscapeCanvasConfig::CreateNodePaletteRoot()
    {
        using namespace LandscapeCanvas;

        const GraphCanvas::EditorId& editorId = LANDSCAPE_CANVAS_EDITOR_ID;
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", editorId);

        // Don't give the Vegetation options if the gem isn't present.
        bool vegetationGemIsPresent = AzToolsFramework::IsComponentWithServiceRegistered(AZ_CRC_CE("VegetationSystemService"));
        if (vegetationGemIsPresent)
        {
            GraphCanvas::IconDecoratedNodePaletteTreeItem* areaCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Vegetation Areas", editorId);
            areaCategory->SetTitlePalette("VegetationAreaNodeTitlePalette");
            REGISTER_NODE_PALETTE_ITEM(areaCategory, AreaBlenderNode, editorId);
            REGISTER_NODE_PALETTE_ITEM(areaCategory, BlockerAreaNode, editorId);
            REGISTER_NODE_PALETTE_ITEM(areaCategory, MeshBlockerAreaNode, editorId);
            REGISTER_NODE_PALETTE_ITEM(areaCategory, SpawnerAreaNode, editorId);
        }

        // Gradients
        GraphCanvas::IconDecoratedNodePaletteTreeItem* gradientCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Gradients", editorId);
        gradientCategory->SetTitlePalette("GradientNodeTitlePalette");
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, AltitudeGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, ConstantGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, GradientBakerNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, ImageGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, PerlinNoiseGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, RandomNoiseGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, ShapeAreaFalloffGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, SlopeGradientNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientCategory, SurfaceMaskGradientNode, editorId);

        // Don't give the option for the Fast Noise Gradient if the gem isn't present.
        bool fastNoiseGemIsPresent = AzToolsFramework::IsComponentWithServiceRegistered(AZ_CRC_CE("FastNoiseService"));
        if (fastNoiseGemIsPresent)
        {
            REGISTER_NODE_PALETTE_ITEM(gradientCategory, FastNoiseGradientNode, editorId);
        }

        // Gradient Modifiers
        GraphCanvas::IconDecoratedNodePaletteTreeItem* gradientModifierCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Gradient Modifiers", editorId);
        gradientModifierCategory->SetTitlePalette("GradientModifierNodeTitlePalette");
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, DitherGradientModifierNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, GradientMixerNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, InvertGradientModifierNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, LevelsGradientModifierNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, PosterizeGradientModifierNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, SmoothStepGradientModifierNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(gradientModifierCategory, ThresholdGradientModifierNode, editorId);

        // Shapes
        GraphCanvas::IconDecoratedNodePaletteTreeItem* shapeCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Shapes", editorId);
        shapeCategory->SetTitlePalette("ShapeNodeTitlePalette");
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, AxisAlignedBoxShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, BoxShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, CapsuleShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, CompoundShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, CylinderShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, DiskShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, PolygonPrismShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, ReferenceShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, SphereShapeNode, editorId);
        REGISTER_NODE_PALETTE_ITEM(shapeCategory, TubeShapeNode, editorId);

        // Don't give the Terrain options if the gem isn't present.
        bool terrainGemIsPresent = AzToolsFramework::IsComponentWithServiceRegistered(AZ_CRC_CE("TerrainService"));
        if (terrainGemIsPresent)
        {
            GraphCanvas::IconDecoratedNodePaletteTreeItem* terrainCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Terrain", editorId);
            terrainCategory->SetTitlePalette("TerrainNodeTitlePalette");
            REGISTER_NODE_PALETTE_ITEM(terrainCategory, TerrainLayerSpawnerNode, editorId);
            REGISTER_NODE_PALETTE_ITEM(terrainCategory, TerrainMacroMaterialNode, editorId);
            REGISTER_NODE_PALETTE_ITEM(terrainCategory, TerrainSurfaceMaterialsListNode, editorId);
        }

        GraphModelIntegration::AddCommonNodePaletteUtilities(rootItem, editorId);

        return rootItem;
    }

#undef REGISTER_NODE_PALETTE_ITEM

    // Don't register nodes whose corresponding component already exists on the given Entity so that we
    // can prevent the user from adding extender nodes that would leave components in an incompatible state
#define REGISTER_NODE_PALETTE_ITEM_UNIQUE(category, TYPE, editorId, entityId) \
{ \
    AZ::TypeId componentTypeId; \
    LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(componentTypeId, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::GetComponentTypeId, azrtti_typeid<TYPE>()); \
    if (!AzToolsFramework::EntityHasComponentOfType(entityId, componentTypeId)) \
    { \
        category->CreateChildNode<GraphModelIntegration::StandardNodePaletteItem<TYPE>>(TYPE::TITLE, editorId); \
    } \
} \

    GraphCanvas::GraphCanvasTreeItem* GetAreaExtendersNodePaletteRoot(const GraphCanvas::EditorId& editorId, AZ::EntityId entityId)
    {
        using namespace LandscapeCanvas;
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", editorId);

        // Filters
        GraphCanvas::IconDecoratedNodePaletteTreeItem* filtersCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Filters", editorId);
        filtersCategory->SetTitlePalette("VegetationAreaNodeTitlePalette");
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, AltitudeFilterNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, DistanceBetweenFilterNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, DistributionFilterNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, ShapeIntersectionFilterNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, SlopeFilterNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, SurfaceMaskDepthFilterNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(filtersCategory, SurfaceMaskFilterNode, editorId, entityId);

        // Modifiers
        GraphCanvas::IconDecoratedNodePaletteTreeItem* modifiersCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Modifiers", editorId);
        modifiersCategory->SetTitlePalette("VegetationAreaNodeTitlePalette");
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(modifiersCategory, PositionModifierNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(modifiersCategory, RotationModifierNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(modifiersCategory, ScaleModifierNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(modifiersCategory, SlopeAlignmentModifierNode, editorId, entityId);

        // Selectors
        GraphCanvas::IconDecoratedNodePaletteTreeItem* selectorsCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Selectors", editorId);
        selectorsCategory->SetTitlePalette("VegetationAreaNodeTitlePalette");
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(selectorsCategory, AssetWeightSelectorNode, editorId, entityId);

        // Remove any category entries that wind up with no sub-items
        for (GraphCanvas::NodePaletteTreeItem* category : { filtersCategory, modifiersCategory, selectorsCategory })
        {
            if (category && category->GetChildCount() <= 0)
            {
                category->DetachItem();
            }
        }

        return rootItem;
    }

    GraphCanvas::GraphCanvasTreeItem* GetTerrainExtendersNodePaletteRoot(const GraphCanvas::EditorId& editorId, AZ::EntityId entityId)
    {
        using namespace LandscapeCanvas;
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", editorId);

        REGISTER_NODE_PALETTE_ITEM_UNIQUE(rootItem, PhysXHeightfieldColliderNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(rootItem, TerrainHeightGradientListNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(rootItem, TerrainMacroMaterialNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(rootItem, TerrainPhysicsHeightfieldColliderNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(rootItem, TerrainSurfaceGradientListNode, editorId, entityId);
        REGISTER_NODE_PALETTE_ITEM_UNIQUE(rootItem, TerrainSurfaceMaterialsListNode, editorId, entityId);

        return rootItem;
    }

#undef REGISTER_NODE_PALETTE_ITEM_UNIQUE

    LandscapeCanvasConfig* GetDefaultConfig()
    {
        LandscapeCanvasConfig* config = new LandscapeCanvasConfig();
        config->m_editorId = LandscapeCanvas::LANDSCAPE_CANVAS_EDITOR_ID;
        config->m_baseStyleSheet = "LandscapeCanvas/StyleSheet/graphcanvas_style.json";
        config->m_mimeType = LandscapeCanvas::MIME_EVENT_TYPE;
        config->m_saveIdentifier = LandscapeCanvas::SAVE_IDENTIFIER;

        return config;
    }

    AzFramework::EntityContextId MainWindow::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();
    
    MainWindow::MainWindow(QWidget* parent)
        : GraphModelIntegration::EditorMainWindow(GetDefaultConfig(), parent)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext, "Failed to acquire application serialize context.");

        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            s_editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);

        m_prefabFocusPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusPublicInterface>::Get();
        AZ_Assert(m_prefabFocusPublicInterface, "LandscapeCanvas - could not get PrefabFocusPublicInterface on construction.");

        m_prefabPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
        AZ_Assert(m_prefabPublicInterface, "LandscapeCanvas - could not get PrefabPublicInterface on construction.");

        m_readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
        AZ_Assert(m_readOnlyEntityPublicInterface, "LandscapeCanvas - could not get ReadOnlyEntityPublicInterface on construction.");

        const GraphCanvas::EditorId& editorId = GetEditorId();

        // Register unique color palettes for our connections (data types)
        GraphCanvas::StyleManagerRequestBus::Event(editorId, &GraphCanvas::StyleManagerRequests::RegisterDataPaletteStyle, LandscapeCanvas::BoundsTypeId, "BoundsDataColorPalette");
        GraphCanvas::StyleManagerRequestBus::Event(editorId, &GraphCanvas::StyleManagerRequests::RegisterDataPaletteStyle, LandscapeCanvas::GradientTypeId, "GradientDataColorPalette");
        GraphCanvas::StyleManagerRequestBus::Event(editorId, &GraphCanvas::StyleManagerRequests::RegisterDataPaletteStyle, LandscapeCanvas::AreaTypeId, "VegetationAreaDataColorPalette");
        GraphCanvas::StyleManagerRequestBus::Event(editorId, &GraphCanvas::StyleManagerRequests::RegisterDataPaletteStyle, LandscapeCanvas::PathTypeId, "PathDataColorPalette");

        LandscapeCanvas::LandscapeCanvasRequestBus::Handler::BusConnect();
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        AzToolsFramework::Prefab::PrefabFocusNotificationBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
        AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();

        // Listen for Entity notifications if a level is already loaded
        // Otherwise, we will connect/disconnect from this bus when levels are loaded/closed
        if (GetLegacyEditor()->IsLevelLoaded())
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        // Create our temporary Node Inspector using a Pinned Inspector
        m_customNodeInspector = aznew CustomNodeInspectorDockWidget(this);

        // Add our custom action to the scene context menu
        m_sceneContextMenu->AddMenuAction(aznew FindSelectedNodesAction(this));

        UpdateGraphEnabled();

        static constexpr AZStd::string_view LandscapeCanvasActionContextIdentifier = "o3de.context.editor.landscapecanvas";

        if(auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
        {
            hotKeyManagerInterface->AssignWidgetToActionContext(LandscapeCanvasActionContextIdentifier, this);
        }
    }

    MainWindow::~MainWindow()
    {
        AZ::EntitySystemBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::Prefab::PrefabFocusNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect();
        LandscapeCanvas::LandscapeCanvasRequestBus::Handler::BusDisconnect();
    }

    GraphModel::GraphContextPtr MainWindow::GetGraphContext() const
    {
        return LandscapeCanvas::GraphContext::GetInstance();
    }

    void MainWindow::OnGraphModelNodeAdded(GraphModel::NodePtr node)
    {
        // If we weren't graphing a scene, then this new node was dragged in from the Node Palette,
        // so we need to create the appropriate underlying Entity/Component(s)
        if (!m_ignoreGraphUpdates)
        {
            HandleNodeCreated(node);
        }

        // Handle any custom logic when a node is added to the graph (e.g. adding thumbnails)
        HandleNodeAdded(node);
    }

    void MainWindow::OnGraphModelNodeRemoved(GraphModel::NodePtr node)
    {
        // Remove the cached EntityId mapping for this node
        GraphCanvas::GraphId graphId = (*GraphModelIntegration::GraphControllerNotificationBus::GetCurrentBusId());
        auto nodeMap = GetEntityIdNodeMap(graphId, node);
        if (nodeMap)
        {
            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            const AZ::EntityId& entityId = baseNodePtr->GetVegetationEntityId();
            auto it = nodeMap->find(entityId);
            if (it != nodeMap->end())
            {
                nodeMap->erase(it);
            }
        }

        if (m_ignoreGraphUpdates)
        {
            return;
        }

        // Check if the deleted node was a wrapped node
        bool isNodeWrapped = false;
        auto it = AZStd::find(m_deletedWrappedNodes.begin(), m_deletedWrappedNodes.end(), node);
        if (it != m_deletedWrappedNodes.end())
        {
            isNodeWrapped = true;
            m_deletedWrappedNodes.erase(it);
        }

        // If a wrapped node is removed, then only delete the underlying component.
        // Otherwise, delete the whole underlying Entity when the node is removed.
        auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        if (isNodeWrapped)
        {
            AZ::Component* component = baseNodePtr->GetComponent();
            if (component)
            {
                m_ignoreGraphUpdates = true;

                AzToolsFramework::RemoveComponents({ component });

                m_ignoreGraphUpdates = false;
            }
        }
        else
        {
            // Don't use the m_ignoreGraphUpdates guard here because we want descendant Entities that get deleted
            // to remove their corresponding nodes from the graph as well to stay in sync
            const AZ::EntityId& entityId = baseNodePtr->GetVegetationEntityId();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::DeleteEntityAndAllDescendants, entityId);
        }
    }

    void MainWindow::PreOnGraphModelNodeRemoved(GraphModel::NodePtr node)
    {
        GraphCanvas::GraphId graphId = (*GraphModelIntegration::GraphControllerNotificationBus::GetCurrentBusId());

        // We need to track any wrapped nodes before the actually get deleted so we can handle
        // their deletion properly, because once the OnGraphModelNodeRemoved is called
        // the wrapped information is lost.
        bool isNodeWrapped = false;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(isNodeWrapped, graphId, &GraphModelIntegration::GraphControllerRequests::IsNodeWrapped, node);
        if (isNodeWrapped)
        {
            m_deletedWrappedNodes.push_back(node);
        }

        // Before a node gets removed from the graph, save off its position
        // so that we can restore it to its previous spot if it ends up
        // being added back via Undo
        auto it = m_deletedNodePositions.find(graphId);
        if (it != m_deletedNodePositions.end())
        {
            DeletedNodePositionsMap& deletedNodePositionMap = it->second;

            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            AZ::EntityComponentIdPair pair(baseNodePtr->GetVegetationEntityId(), baseNodePtr->GetComponentId());

            AZ::Vector2 position;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(position, graphId, &GraphModelIntegration::GraphControllerRequests::GetPosition, node);
            deletedNodePositionMap[pair] = position;
        }
    }

    void MainWindow::OnGraphModelConnectionAdded(GraphModel::ConnectionPtr connection)
    {
        // Don't need to act on connections that aren't added by the user
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        UpdateConnectionData(connection, true /* added */);
    }

    void MainWindow::OnGraphModelConnectionRemoved(GraphModel::ConnectionPtr connection)
    {
        // Don't need to act on connections that aren't removed by the user
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        UpdateConnectionData(connection, false /* added */);
    }

    void MainWindow::PreOnGraphModelNodeWrapped(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node)
    {
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        // Keep track when wrapped nodes are about to be added so we can prevent the logic that
        // creates new entities when nodes are added
        m_addedWrappedNodes.push_back(node);
    }

    void MainWindow::OnGraphModelNodeWrapped(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node)
    {
        // We only need to add components when nodes are created by the user,
        // not when we are parsing/graphing an existing setup
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        auto it = AZStd::find(m_addedWrappedNodes.begin(), m_addedWrappedNodes.end(), node);
        if (it != m_addedWrappedNodes.end())
        {
            m_addedWrappedNodes.erase(it);
        }

        // We don't need to create a new component for nodes that already
        // have a component tied to them, which happens when nodes get deserialized
        // and OnNodeWrapped gets invoked
        auto wrappedNode = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        if (wrappedNode->GetComponentId() != AZ::InvalidComponentId)
        {
            return;
        }

        // When a node is wrapped (e.g. filter/modifier added to a layer area), then we will
        // add the Component to the Entity of the wrapper node
        auto* sourceNode = static_cast<LandscapeCanvas::BaseNode*>(wrapperNode.get());
        AZ::EntityId vegetationEntityId = sourceNode->GetVegetationEntityId();

        m_ignoreGraphUpdates = true;

        AddComponentForNode(node, vegetationEntityId);

        m_ignoreGraphUpdates = false;
    }

    void MainWindow::OnSelectionChanged()
    {
        GraphCanvas::AssetEditorMainWindow::OnSelectionChanged();

        if (m_ignoreGraphUpdates)
        {
            return;
        }

        GraphModel::NodePtrList nodeList;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeList, GetActiveGraphCanvasGraphId(), &GraphModelIntegration::GraphControllerRequests::GetSelectedNodes);

        // Iterate through the selected nodes to find their corresponding vegetation entities
        AzToolsFramework::EntityIdSet vegetationEntityIdsToSelect;
        for (const auto& node : nodeList)
        {
            if (!node)
            {
                continue;
            }

            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            vegetationEntityIdsToSelect.insert(baseNodePtr->GetVegetationEntityId());
        }

        // If we don't have any nodes selected, or the entities selected in the graph aren't nodes (e.g. comments, node groups)
        // then show an empty Node Inspector
        if (vegetationEntityIdsToSelect.empty())
        {
            m_customNodeInspector->GetEntityPropertyEditor()->SetOverrideEntityIds({ AZ::EntityId() });
            return;
        }

        QTimer::singleShot(0, [this, vegetationEntityIdsToSelect]() {
            // If we are in object pick mode and have selected a single node, then use the Entity for that node
            // as the pick mode selection
            if (m_inObjectPickMode && vegetationEntityIdsToSelect.size() == 1)
            {
                AZ::EntityId selectedEntityId = *vegetationEntityIdsToSelect.begin();

                AzToolsFramework::EditorPickModeRequestBus::Broadcast(&AzToolsFramework::EditorPickModeRequests::PickModeSelectEntity, selectedEntityId);
                AzToolsFramework::EditorPickModeRequestBus::Broadcast(&AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);
            }
            // Otherwise, update the selection in our node inspector
            else
            {
                m_customNodeInspector->GetEntityPropertyEditor()->SetOverrideEntityIds(vegetationEntityIdsToSelect);
            }
        });
    }

    void MainWindow::OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationTarget)
    {
        using namespace AzToolsFramework;
        using namespace LandscapeCanvas;

        m_ignoreGraphUpdates = true;

        GraphCanvas::GraphId graphId = GetActiveGraphCanvasGraphId();

        LandscapeCanvasSerialization serialization;
        LandscapeCanvasSerializationRequestBus::BroadcastResult(serialization, &LandscapeCanvasSerializationRequests::GetSerializedMappings);

        EntityIdList entitiesToDuplicate;

        // Look for any nodes being serialized for which we want to duplicate the Entity
        // corresponding to our Landscape Canvas node
        for (AZ::Entity* nodeEntity : serializationTarget.GetGraphData().m_nodes)
        {
            GraphCanvas::NodeId nodeUiId = nodeEntity->GetId();

            // Ignore any nodes serialized by GraphCanvas that aren't GraphModel nodes (e.g. comments/node groups), since they
            // don't have an actual Entity/Component tied to them that we'll need to duplicate
            GraphModel::NodePtr node;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(node, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, nodeUiId);
            if (!node)
            {
                continue;
            }

            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            AZ::EntityId entityId = baseNodePtr->GetVegetationEntityId();

            entitiesToDuplicate.push_back(entityId);
        }

        // Duplicate the corresponding entities
        auto outcome = m_prefabPublicInterface->DuplicateEntitiesInInstance(entitiesToDuplicate);
        if (!outcome.IsSuccess())
        {
            AZ_Error("LandscapeCanvas", false, outcome.GetError().c_str());
            return;
        }

        auto duplicatedEntities = outcome.GetValue();

        // Create a mapping of the original EntityId's corresponding to the
        // new EntityId's that were duplicated.
        int i = 0;
        const size_t numDuplicatedEntities = duplicatedEntities.size();
        for (const auto& originalEntityId : entitiesToDuplicate)
        {
            // An EntityId might already exist in the mapping for the case where a single node (Entity)
            // has multiple wrapped nodes on it, corresponding to multiple components on a single Entity
            if (serialization.m_deserializedEntities.contains(originalEntityId))
            {
                continue;
            }

            if (i >= numDuplicatedEntities)
            {
                break;
            }

            serialization.m_deserializedEntities[originalEntityId] = duplicatedEntities[i++];
        }

        LandscapeCanvas::LandscapeCanvasSerializationRequestBus::Broadcast(&LandscapeCanvas::LandscapeCanvasSerializationRequests::SetDeserializedEntities, serialization.m_deserializedEntities);

        m_ignoreGraphUpdates = false;
    }

    void MainWindow::OnGraphModelGraphModified(GraphModel::NodePtr node)
    {
        AZ_UNUSED(node);

        if (m_ignoreGraphUpdates)
        {
            return;
        }

        // Flag the level as dirty if anything in the graph changes, since some graph actions
        // (e.g. moving nodes around, creating bookmarks, etc...) don't trigger actual Entity/Component
        // changes that would flag the level as dirty.
        if (const auto editor = GetLegacyEditor();
            !editor->IsModified())
        {
            editor->SetModifiedFlag();
            editor->SetModifiedModule(eModifiedEntities);
        }
    }

    void MainWindow::OnEditorOpened(GraphCanvas::EditorDockWidget* dockWidget)
    {
        using namespace AzFramework::Terrain;

        // Detect if it's possible to create a new entity in the current context
        AZ::EntityId focusRootEntityId = m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(s_editorEntityContextId);
        if (m_readOnlyEntityPublicInterface->IsReadOnly(focusRootEntityId))
        {
            // Abort
            CloseEditor(dockWidget->GetDockWidgetId());

            QWidget* activeWindow = AzToolsFramework::GetActiveWindow();

            QMessageBox::warning(
                activeWindow,
                QString("Landscape Canvas Asset Creation Error"),
                QString("Could not create new Landscape Canvas asset under read-only entity."),
                QMessageBox::Ok,
                QMessageBox::Ok
            );

            return;
        }

        // Invoke the GraphCanvas base instead of the GraphModelIntegration::EditorMainWindow so that we
        // can do our own custom handling when opening an existing graph
        GraphCanvas::AssetEditorMainWindow::OnEditorOpened(dockWidget);

        // If this graph was opened by File -> New or by dragging a node from
        // the Node Palette onto the empty canvas, then we first need to create
        // a root Entity for it with a Landscape Canvas component.
        if (!m_ignoreGraphUpdates)
        {
            AZ::EntityId rootEntityId;
            AzToolsFramework::EditorRequestBus::BroadcastResult(rootEntityId, &AzToolsFramework::EditorRequests::CreateNewEntity, AZ::EntityId());

            AZ::Vector3 translation = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(translation, rootEntityId, &AZ::TransformBus::Events::GetWorldTranslation);

            // Get the terrain height at the XY world coordinate where our new Entity was created
            float height = translation.GetZ();
            TerrainDataRequestBus::BroadcastResult(height, &TerrainDataRequests::GetHeightFromFloats, translation.GetX(), translation.GetY(), TerrainDataRequests::Sampler::BILINEAR, nullptr);

            // Update the new Entity translation so that it is placed on the terrain so that any vegetation resulting
            // from it will be planted on the terrain
            translation.SetZ(height);
            AZ::TransformBus::Event(rootEntityId, &AZ::TransformBus::Events::SetWorldTranslation, translation);

            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, AzToolsFramework::EntityIdList{ rootEntityId }, AZ::ComponentTypeList{ LandscapeCanvas::EditorLandscapeCanvasComponentTypeId });

            HandleGraphOpened(rootEntityId, dockWidget->GetDockWidgetId());

            // Update the tab name for the new graph after creating the root Entity to hold its Landscape Canvas component
            AZ::Entity* rootEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(rootEntity, &AZ::ComponentApplicationRequests::FindEntity, rootEntityId);
            AZ_Assert(rootEntity, "No Entity found for EntityId = %s", rootEntityId.ToString().c_str());
            OnEntityNameChanged(rootEntityId, rootEntity->GetName());
        }

        // Initialize the EntityIdNodeMaps that will be used for parsing/creating connections later
        GraphCanvas::GraphId graphId = dockWidget->GetGraphId();
        EntityIdNodeMaps newNodeMaps;
        for (int i = 0; i < EntityIdNodeMapEnum::Count; ++i)
        {
            newNodeMaps.push_back(EntityIdNodeMap());
        }
        m_entityIdNodeMapsByGraph[graphId] = newNodeMaps;
        m_deletedNodePositions[graphId] = DeletedNodePositionsMap();
    }

    void MainWindow::OnEditorClosing(GraphCanvas::EditorDockWidget* dockWidget)
    {
        // Stop listening for changes to this Vegetation Entity when we close the graph for it.
        GraphCanvas::DockWidgetId dockWidgetId = dockWidget->GetDockWidgetId();
        auto it = AZStd::find_if(m_dockWidgetsByEntity.begin(), m_dockWidgetsByEntity.end(),
            [dockWidgetId](decltype(m_dockWidgetsByEntity)::const_reference pair)
            {
                return dockWidgetId == pair.second;
            }
        );

        if (it != m_dockWidgetsByEntity.end())
        {
            const AZ::EntityId& rootEntityId = it->first;
            m_dockWidgetsByEntity.erase(it);

            // Save our graph whenever it is closed
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, rootEntityId);
            if (entity)
            {
                GraphCanvas::GraphId graphId = dockWidget->GetGraphId();
                GraphCanvas::GraphModelRequestBus::Event(graphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, graphId);

                // Serialize the graph into the Landscape Canvas component on the root Entity that
                // corresponds to this graph
                GraphModel::GraphPtr graph = GetGraphById(graphId);
                auto landscapeCanvasComponent = azrtti_cast<LandscapeCanvas::EditorLandscapeCanvasComponent*>(entity->FindComponent(LandscapeCanvas::EditorLandscapeCanvasComponentTypeId));
                if (landscapeCanvasComponent)
                {
                    landscapeCanvasComponent->m_graph = *m_serializeContext->CloneObject(graph.get());

                    // Mark the Landscape Canvas entity as dirty so the changes to the graph will be picked up on the next save
                    AzToolsFramework::ScopedUndoBatch undo("Update Landscape Canvas Graph");
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, rootEntityId);
                }
            }
        }

        // Clear out the cached EntityIdNode mapping for the graph when it is closed
        GraphCanvas::GraphId graphId = dockWidget->GetGraphId();
        auto nodeMapIt = m_entityIdNodeMapsByGraph.find(graphId);
        if (nodeMapIt != m_entityIdNodeMapsByGraph.end())
        {
            m_entityIdNodeMapsByGraph.erase(nodeMapIt);
        }
        auto nodePositionsMapIt = m_deletedNodePositions.find(graphId);
        if (nodePositionsMapIt != m_deletedNodePositions.end())
        {
            m_deletedNodePositions.erase(nodePositionsMapIt);
        }

        // Do this last so that the graph isn't closed before we get a chance to save it
        GraphModelIntegration::EditorMainWindow::OnEditorClosing(dockWidget);
    }

    QAction* MainWindow::AddFileNewAction(QMenu* menu)
    {
        m_fileNewAction = GraphModelIntegration::EditorMainWindow::AddFileNewAction(menu);

        // Disable our file menu action for creating a new graph if a level isn't loaded
        m_fileNewAction->setEnabled(GetLegacyEditor()->IsLevelLoaded());

        return m_fileNewAction;
    }

    QAction* MainWindow::AddFileOpenAction(QMenu* menu)
    {
        AZ_UNUSED(menu);
        return nullptr;
    }

    QAction* MainWindow::AddFileSaveAction(QMenu* menu)
    {
        AZ_UNUSED(menu);
        return nullptr;
    }

    QAction* MainWindow::AddFileSaveAsAction(QMenu* menu)
    {
        AZ_UNUSED(menu);
        return nullptr;
    }

    QMenu* MainWindow::AddEditMenu()
    {
        QMenu* menu = GraphModelIntegration::EditorMainWindow::AddEditMenu();

        // Temporarily add our own Undo/Redo menu actions that will just trigger the main Editor's
        // Undo/Redo actions, since our graphs are listening/responding to Editor Entity/Component
        // changes (e.g. entities/components being added/removed).
        // Once our generic GraphModel windowing framework supports Undo/Redo then we will extend
        // the GraphModel::EditorMainWindow to provide the Undo/Redo menu actions by default.
        if (menu && !menu->actions().empty())
        {
            auto separatorAction = menu->insertSeparator(menu->actions().first());

            auto redoAction = new QAction(QObject::tr("&Redo"), this);
            redoAction->setShortcut(AzQtComponents::RedoKeySequence);
            QObject::connect(redoAction, &QAction::triggered, [] {
                GetLegacyEditor()->Redo();
            });
            menu->insertAction(separatorAction, redoAction);

            auto undoAction = new QAction(QObject::tr("&Undo"), this);
            undoAction->setShortcut(QKeySequence::Undo);
            QObject::connect(undoAction, &QAction::triggered, [] {
                GetLegacyEditor()->Undo();
            });
            menu->insertAction(redoAction, undoAction);
        }

        return menu;
    }

    QAction* MainWindow::AddEditCutAction([[maybe_unused]] QMenu* menu)
    {
        // Disabled until we can leverage prefab API to cut/copy/paste
        return nullptr;
    }

    QAction* MainWindow::AddEditCopyAction([[maybe_unused]] QMenu* menu)
    {
        // Disabled until we can leverage prefab API to cut/copy/paste
        return nullptr;
    }

    QAction* MainWindow::AddEditPasteAction([[maybe_unused]] QMenu* menu)
    {
        // Disabled until we can leverage prefab API to cut/paste
        return nullptr;
    }

    void MainWindow::HandleWrapperNodeActionWidgetClicked(GraphModel::NodePtr wrapperNode, [[maybe_unused]] const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        auto baseNode = static_cast<LandscapeCanvas::BaseNode*>(wrapperNode.get());
        AZ::EntityId entityId = baseNode->GetVegetationEntityId();

        GraphCanvas::NodePaletteConfig config;
        config.m_editorId = GetEditorId();
        config.m_mimeType = LandscapeCanvas::MIME_EVENT_TYPE;
        config.m_isInContextMenu = true;
        config.m_saveIdentifier = LandscapeCanvas::CONTEXT_MENU_SAVE_IDENTIFIER;

        switch (baseNode->GetBaseNodeType())
        {
        case LandscapeCanvas::BaseNode::TerrainArea:
            config.m_rootTreeItem = GetTerrainExtendersNodePaletteRoot(GetEditorId(), entityId);
            break;
        case LandscapeCanvas::BaseNode::VegetationArea:
            config.m_rootTreeItem = GetAreaExtendersNodePaletteRoot(GetEditorId(), entityId);
            break;
        default:
            AZ_Assert(false, "Unsupported node type: %d", baseNode->GetBaseNodeType());
            return;
        }

        // Create the Context Menu with embedded Node Palette for adding extenders to the wrapped node
        // The ownership of this Node Palette is passed to the context menu
        LayerExtenderContextMenu menu(config, this);
        menu.exec(screenPoint);

        // Check if a node was selected in the Node Palette of our context menu.
        // If the menu was dismissed, then the mime event will be null.
        GraphCanvas::GraphCanvasMimeEvent* mimeEvent = menu.GetNodePalette()->GetContextMenuEvent();
        if (mimeEvent)
        {
            GraphCanvas::GraphId graphId = GetActiveGraphCanvasGraphId();
            AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));

            m_ignoreGraphUpdates = true;

            // Create the node that was selected from the node palette.
            if (mimeEvent->ExecuteEvent(dropPos, dropPos, graphId))
            {
                GraphCanvas::NodeId nodeId = mimeEvent->GetCreatedNodeId();

                GraphModel::NodePtr node;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(node, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, nodeId);

                // Stop ignoring the graph updates once the node has been created so that the OnGraphModelNodeWrapped
                // will get called once we wrap the node in the next step
                m_ignoreGraphUpdates = false;

                // Wrap the extender node on its parent wrapped node.
                AZ::u32 layoutOrder = GetWrappedNodeLayoutOrder(node);
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::WrapNodeOrdered, wrapperNode, node, layoutOrder);
            }
            else
            {
                m_ignoreGraphUpdates = false;
            }
        }
    }

    GraphCanvas::Endpoint MainWindow::CreateNodeForProposal(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        GraphCanvas::Endpoint createdEndpoint = GraphCanvas::AssetEditorMainWindow::CreateNodeForProposal(connectionId, endpoint, scenePoint, screenPoint);

        if (createdEndpoint.IsValid())
        {
            GraphModel::NodePtr sourceNode;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(sourceNode, GetActiveGraphCanvasGraphId(), &GraphModelIntegration::GraphControllerRequests::GetNodeById, endpoint.GetNodeId());

            GraphModel::NodePtr createdNode;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(createdNode, GetActiveGraphCanvasGraphId(), &GraphModelIntegration::GraphControllerRequests::GetNodeById, createdEndpoint.GetNodeId());

            AZ_Assert(sourceNode && createdNode, "Unable to find GraphModel::Node for associated Endpoint.");

            // If the source node and the created node both have preview bounds slots,
            // then automatically connect the preview bounds on the created node to the
            // same slot as the one on the source node (if it is connected to something)
            GraphModel::SlotPtr sourcePreviewBoundsSlot = sourceNode->GetSlot(LandscapeCanvas::PREVIEW_BOUNDS_SLOT_ID);
            GraphModel::SlotPtr createdPreviewBoundsSlot = createdNode->GetSlot(LandscapeCanvas::PREVIEW_BOUNDS_SLOT_ID);
            if (sourcePreviewBoundsSlot && createdPreviewBoundsSlot)
            {
                // The preview bounds is an input slot, so it will only have 1 connection (if any)
                GraphModel::Slot::ConnectionList connections = sourcePreviewBoundsSlot->GetConnections();
                if (connections.size() == 1)
                {
                    GraphModel::ConnectionPtr connection = *connections.begin();
                    GraphModel::SlotPtr previewBoundsSourceSlot = connection->GetSourceSlot();

                    GraphModelIntegration::GraphControllerRequestBus::Event(GetActiveGraphCanvasGraphId(), &GraphModelIntegration::GraphControllerRequests::AddConnection, previewBoundsSourceSlot, createdPreviewBoundsSlot);
                }
            }
        }

        return createdEndpoint;
    }

    void MainWindow::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        // We already handle when components are explicitly enabled/disabled, but if they are enabled/disabled
        // as a result of their dependencies being enabled/disabled, then we don't get an explicit
        // notification for that action.  The Entity Inspector also uses this EntitySystemBus::OnEntityActivated
        // to determine when to re-check component state, since the Entity gets deactivated/re-activated when
        // making component changes, so this is when we should update the enabled/disabled state of any nodes
        // associated with this Entity.
        GraphModel::NodePtrList matchingNodes = GetAllNodesMatchingEntity(entityId);
        for (auto node : matchingNodes)
        {
            GraphCanvas::GraphId graphId = GetGraphId(node->GetGraph());

            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            if (baseNodePtr->GetComponent())
            {
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::EnableNode, node);
            }
            else
            {
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::DisableNode, node);
            }
        }
    }

    void MainWindow::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
    {
        // Update the entity name slot on any nodes for this entity across all graphs.
        for (GraphCanvas::GraphId graphId : GetOpenGraphIds())
        {
            GraphModel::NodePtrList nodes = GetAllNodesMatchingEntityInGraph(graphId, entityId);

            for (auto& node : nodes)
            {
                if (auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get()); baseNodePtr)
                {
                    // Refresh the entity name on this node.
                    baseNodePtr->RefreshEntityName();

                    // Refresh the display for the entity name on this node.
                    if (GraphModel::SlotPtr slot = node->GetSlot(LandscapeCanvas::ENTITY_NAME_SLOT_ID); slot)
                    {
                        GraphCanvas::SlotId slotId;
                        GraphModelIntegration::GraphControllerRequestBus::EventResult(
                            slotId, graphId, &GraphModelIntegration::GraphControllerRequests::GetSlotIdBySlot, slot);

                        AZ::EBusAggregateResults<GraphCanvas::NodePropertyDisplay*> nodePropertyDisplays;
                        GraphCanvas::NodePropertyRequestBus::EventResult(
                            nodePropertyDisplays, slotId, &GraphCanvas::NodePropertyRequests::GetNodePropertyDisplay);

                        for (auto& nodePropertyDisplay : nodePropertyDisplays.values)
                        {
                            if (nodePropertyDisplay)
                            {
                                nodePropertyDisplay->UpdateDisplay();
                            }
                        }
                    }
                }
            }
        }

        // If this entity is also the root entity for a graph, update the graph's tab name.
        auto it = m_dockWidgetsByEntity.find(entityId);
        if (it != m_dockWidgetsByEntity.end())
        {
            GraphCanvas::DockWidgetId dockWidgetId = it->second;
            GraphCanvas::EditorDockWidgetRequestBus::Event(dockWidgetId, &GraphCanvas::EditorDockWidgetRequests::SetTitle, name);
        }
    }

    bool MainWindow::HandleGraphOpened(const AZ::EntityId& rootEntityId, const GraphCanvas::DockWidgetId& dockWidgetId)
    {
        // Keep track of the dock widget created for this root Vegetation Entity, and
        // listen for any changes to the entity
        m_dockWidgetsByEntity[rootEntityId] = dockWidgetId;

        GraphCanvas::GraphId graphId;
        GraphCanvas::EditorDockWidgetRequestBus::EventResult(graphId, dockWidgetId, &GraphCanvas::EditorDockWidgetRequests::GetGraphId);

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, rootEntityId);
        AZ_Assert(entity, "No Entity found for EntityId = %s", rootEntityId.ToString().c_str());

        auto landscapeCanvasComponent = azrtti_cast<LandscapeCanvas::EditorLandscapeCanvasComponent*>(entity->FindComponent(LandscapeCanvas::EditorLandscapeCanvasComponentTypeId));
        AZ_Assert(landscapeCanvasComponent, "Missing Landscape Canvas component on EntityId = %s", rootEntityId.ToString().c_str());

        bool isNewGraph = false;
        GraphModel::GraphPtr graph = AZStd::make_shared<GraphModel::Graph>(GetGraphContext());
        GraphModel::Graph& savedGraph = landscapeCanvasComponent->m_graph;
        if (savedGraph.GetNodes().empty())
        {
            // If this graph has never been saved before, then there won't be any nodes in
            // the serialized graph from our component, so we don't need to load anything
            isNewGraph = true;
        }
        else
        {
            // Load the serialized graph and invoke the PostLoadSetup so that all the metadata
            // for the graph/nodes/slots gets setup properly before we call CreateGraphController
            // that will actually recreate the full graph in the scene
            graph.reset(m_serializeContext->CloneObject(&savedGraph));
            graph->PostLoadSetup(GetGraphContext());
        }

        // Keep track of our new graph.
        m_graphs[graphId] = graph;

        // Listen for GraphController notifications on the new graph.
        GraphModelIntegration::GraphControllerNotificationBus::MultiHandler::BusConnect(graphId);

        // Create the controller for the new graph.
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(&GraphModelIntegration::GraphManagerRequests::CreateGraphController, graphId, graph);

        // If we loaded a saved graph, we need to make sure all the loaded nodes Entity/Components still exist,
        // and also look for any new components that have been added that need new nodes created for them
        if (!isNewGraph)
        {
            RefreshEntityComponentNodes(rootEntityId, graphId);
        }

        return isNewGraph;
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        NodeContextMenu contextMenu(GetActiveGraphCanvasGraphId());

        return AssetEditorMainWindow::HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    void MainWindow::GetChildrenTree(const AZ::EntityId& rootEntityId, AzToolsFramework::EntityIdList& childrenList)
    {
        AzToolsFramework::EntityIdList children;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, rootEntityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            childrenList.push_back(childId);

            GetChildrenTree(childId, childrenList);
        }
    }

    QString MainWindow::GetPropertyPathForSlot(GraphModel::SlotPtr slot, GraphModel::DataType::Enum dataType, int elementIndex)
    {
        static const char* ConfigurationPropertyPrefix = "Configuration|";
        static const char* PreviewEntityIdPropertyPath = "Previewer|Preview Settings|Pin Preview to Shape";
        static const char* GradientEntityIdPropertyPath = "Gradient|Gradient Entity Id";
        static const char* ShapeEntityIdPropertyPath = "Shape Entity Id";
        static const char* InputBoundsEntityIdPropertyPath = "Input Bounds";
        static const char* PinToShapeEntityIdPropertyPath = "Pin To Shape Entity Id";
        static const char* VegetationAreasPropertyPath = "Vegetation Areas";
        static const char* TerrainSurfaceEntityIdPropertyPath = "Gradient Entity";

        const GraphModel::SlotName& slotName = slot->GetName();
        QString propertyPath;
        bool useConfigurationPrefix = true;
        switch (dataType)
        {
        case LandscapeCanvas::LandscapeCanvasDataTypeEnum::Bounds:
        {
            if (slotName == LandscapeCanvas::PREVIEW_BOUNDS_SLOT_ID)
            {
                propertyPath = PreviewEntityIdPropertyPath;
                useConfigurationPrefix = false;
            }
            else if (slotName == LandscapeCanvas::INBOUND_SHAPE_SLOT_ID
                || slotName == LandscapeCanvas::PLACEMENT_BOUNDS_SLOT_ID)
            {
                propertyPath = ShapeEntityIdPropertyPath;
            }
            else if (slotName == LandscapeCanvas::PIN_TO_SHAPE_SLOT_ID)
            {
                propertyPath = PinToShapeEntityIdPropertyPath;
            }
            else if (slotName == LandscapeCanvas::INPUT_BOUNDS_SLOT_ID)
            {
                propertyPath = InputBoundsEntityIdPropertyPath;
            }
        } break;
        case LandscapeCanvas::LandscapeCanvasDataTypeEnum::Gradient:
        {
            GraphModel::NodePtr targetNode = slot->GetParentNode();
            auto targetBaseNode = static_cast<LandscapeCanvas::BaseNode*>(targetNode.get());
            auto targetBaseNodeType = targetBaseNode->GetBaseNodeType();

            if (targetBaseNodeType == LandscapeCanvas::BaseNode::TerrainSurfaceExtender)
            {
                propertyPath = TerrainSurfaceEntityIdPropertyPath;
            }
            else if (targetBaseNodeType != LandscapeCanvas::BaseNode::TerrainExtender)
            {
                propertyPath = GradientEntityIdPropertyPath;
            }

            // Special case handling of some gradient properties for extendable gradient mixers
            // and the position modifier which are nested under group elements
            if (slot->SupportsExtendability())
            {
                QString gradientListName;
                if (targetBaseNodeType == LandscapeCanvas::BaseNode::TerrainExtender)
                {
                    gradientListName = "Gradient Entities|[%1]";
                }
                else if (targetBaseNodeType == LandscapeCanvas::BaseNode::TerrainSurfaceExtender)
                {
                    gradientListName = "Gradient to Surface Mappings|[%1]|";
                }
                else
                {
                    gradientListName = "Layers|[%1]|";
                }

                propertyPath.prepend(gradientListName.arg(elementIndex));
            }
            else if (slotName == LandscapeCanvas::BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_ID
                || slotName == LandscapeCanvas::BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_ID
                || slotName == LandscapeCanvas::BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_ID)
            {
                // The X/Y/Z supported nodes are Position/Rotation modifiers, so we need
                // to figure out which one this is to get the right property path
                if (targetNode)
                {
                    // The node titles are "Position Modifier" or "Rotation Modifier", and
                    // the property path is expecting Position/Rotation|Gradient|Gradient Entity Id
                    // so we need to parse the "Position"/"Rotation" out of the title to use
                    // in the property path
                    QStringList parts = QString(targetNode->GetTitle()).split(' ');
                    AZ_Assert(!parts.empty(), "Unrecognized node title");
                    propertyPath.prepend(QString("%1 %2|").arg(parts[0]).arg(slotName.back()));
                }
            }
        } break;
        case LandscapeCanvas::LandscapeCanvasDataTypeEnum::Area:
        {
            propertyPath = QString("%1|[%2]").arg(VegetationAreasPropertyPath).arg(elementIndex);
        } break;
        }

        // Most of our supported properties are nested under a top-level configuration path
        if (!propertyPath.isEmpty() && useConfigurationPrefix)
        {
            propertyPath.prepend(ConfigurationPropertyPrefix);
        }

        return propertyPath;
    }

    void MainWindow::UpdateConnectionData(GraphModel::ConnectionPtr connection, bool added)
    {
        if (!connection)
        {
            return;
        }

        GraphCanvas::GraphId graphId = (*GraphModelIntegration::GraphControllerNotificationBus::GetCurrentBusId());

        // Similarly as below, this protects against the edge case where this logic gets hit if the node and/or
        // slot belonging to this connection got deleted before this was executed.
        if (!connection->GetSourceNode() || !connection->GetTargetNode() || !connection->GetSourceSlot() || !connection->GetTargetSlot())
        {
            return;
        }

        // Figure out the element index we need to update based on the index of the
        // target slot on the target node that have the same data type
        GraphModel::NodePtr targetNode = connection->GetTargetNode();
        GraphModel::SlotPtr targetSlot = connection->GetTargetSlot();
        GraphModel::DataTypePtr dataType = connection->GetSourceSlot()->GetDataType();
        int elementIndexToModify = GetInboundDataSlotIndex(targetNode, dataType, targetSlot);
        if (elementIndexToModify == InvalidSlotIndex)
        {
            // Typically this shouldn't be reached, but there are cases where the slot index might
            // be invalid, such as the target node being deleted before the connection is triggered
            // to be removed, which could happen if the node was deleted while it was in a collapsed group.
            return;
        }

        // If the connection was removed, the target will be set to an invalid EntityId
        // If the connection was added, the target will be updated with the appropriate EntityId from the source
        AZ::EntityId newEntityId;
        if (added)
        {
            auto sourceNode = static_cast<LandscapeCanvas::BaseNode*>(connection->GetSourceNode().get());
            newEntityId = sourceNode->GetVegetationEntityId();
        }

        // Figure out the property path we are looking for based on the data type of the slot
        GraphModel::DataType::Enum dataTypeEnum = dataType->GetTypeEnum();
        QString propertyPath = GetPropertyPathForSlot(targetSlot, dataTypeEnum, elementIndexToModify);
        if (propertyPath.isEmpty())
        {
            // Special-case to handle setting an image asset path.
            // This needs separate logic because all our other data types (Bounds/Gradient/Area)
            // are just AZ::EntityId under the hood and can be set directly on the property,
            // whereas the output asset comes as an AZ::IO::Path and the input is an actual
            // AZ::RPI::StreamingImageAsset, so we need to use the helper buses to get/set
            if (added && dataTypeEnum == LandscapeCanvas::LandscapeCanvasDataTypeEnum::Path)
            {
                auto targetBaseNode = static_cast<LandscapeCanvas::BaseNode*>(targetNode.get());
                HandleSetImageAssetPath(newEntityId, targetBaseNode->GetVegetationEntityId());
            }

            return;
        }

        // Calling UpdateConnectionData will result in a component property being modified,
        // which in turn will result in prefab propagation. Because that is delayed until the next
        // tick, there is a point in time where the OnEntityComponentPropertyChanged event will
        // be triggered but the property won't be set yet, so when UpdateConnections gets called,
        // it will think the connection corresponding to that property needs to be removed. So
        // we need to handle this case by ignoring the next component property change for this entity
        // since it will already be up-to-date by UpdateConnectionData being invoked
        if (targetNode)
        {
            auto targetBaseNode = static_cast<LandscapeCanvas::BaseNode*>(targetNode.get());
            m_ignoreEntityComponentPropertyChanges.push_back(targetBaseNode->GetVegetationEntityId());
        }

        // If our target is an extendable slot (e.g. gradient mixer, area blender, etc...) then the element that needs
        // to be set is actually in a container, and might need to be added
        bool elementInContainer = targetSlot->SupportsExtendability();

        // Queue this event since it occurs when attaching/detaching connections
        // in the UI, otherwise the attach/detach will appear to stall momentarily
        QTimer::singleShot(0, [this, graphId, targetNode, targetSlot, newEntityId, propertyPath, elementIndexToModify, elementInContainer]() {
            if (!targetNode)
            {
                return;
            }

            // Special case for the Vegetation Area Placement Bounds, the slot actually represents a separate
            // Reference Shape or actual Shape component on the same Entity
            AZ::Component* component = nullptr;
            auto targetBaseNode = static_cast<LandscapeCanvas::BaseNode*>(targetNode.get());
            if (targetBaseNode->GetBaseNodeType() == LandscapeCanvas::BaseNode::BaseNodeType::VegetationArea && targetSlot->GetName() == LandscapeCanvas::PLACEMENT_BOUNDS_SLOT_ID)
            {
                // Make sure the target entity still exists before we do all this special-case logic, because it
                // might have been deleted and UpdateConnectionData was only executed because GraphModel was removing
                // the connections associated with a node being deleted
                const AZ::EntityId& targetEntityId = targetBaseNode->GetVegetationEntityId();
                AZ::Entity* targetEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(targetEntity, &AZ::ComponentApplicationRequests::FindEntity, targetEntityId);
                if (!targetEntity)
                {
                    return;
                }

                // Special case handling when connecting the Placement Bounds to a Shape that exists
                // on the same Entity by re-enabling that disabled Shape component. This is mainly for
                // handling existing Vegetation data that wasn't authored in a graph originally.
                if (newEntityId == targetEntityId)
                {
                    auto nodeMapsIt = m_entityIdNodeMapsByGraph.find(graphId);
                    if (nodeMapsIt == m_entityIdNodeMapsByGraph.end())
                    {
                        return;
                    }

                    const EntityIdNodeMaps& nodeMaps = nodeMapsIt->second;
                    const auto& shapeNodeMap = nodeMaps[EntityIdNodeMapEnum::Shapes];
                    auto shapeIt = shapeNodeMap.find(targetEntityId);
                    if (shapeIt != shapeNodeMap.end())
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Enable Embedded Shape");

                        auto shapeNode = static_cast<LandscapeCanvas::BaseNode*>(shapeIt->second.get());
                        AZ::Entity::ComponentArrayType disabledComponents;
                        AzToolsFramework::EditorDisabledCompositionRequestBus::Event(targetEntityId, &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
                        for (auto disabledComponent : disabledComponents)
                        {
                            // Look through the disabled components on our Entity for our disabled Shape component
                            if (disabledComponent->GetId() == shapeNode->GetComponentId())
                            {
                                // Re-enable our Shape component
                                AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::EnableComponents, AZ::Entity::ComponentArrayType{ disabledComponent });

                                // Disable any incompatible components (e.g. an existing Reference Shape component on the Entity)
                                AzToolsFramework::EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
                                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequests::GetPendingComponentInfo, disabledComponent);
                                if (!pendingComponentInfo.m_validComponentsThatAreIncompatible.empty())
                                {
                                    AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::DisableComponents, pendingComponentInfo.m_validComponentsThatAreIncompatible);
                                }

                                break;
                            }
                        }

                        undoBatch.MarkEntityDirty(targetEntityId);

                        return;
                    }
                }

                // For the common case, we just need to use the Reference Shape component on this Entity if it is enabled
                auto baseAreaNodePtr = static_cast<LandscapeCanvas::BaseAreaNode*>(targetNode.get());
                component = baseAreaNodePtr->GetReferenceShapeComponent();

                // If GetReferenceShapeComponent() fails, then that means either there is no Reference Shape component
                // on our Entity, or there is but it is disabled
                if (!component)
                {
                    // Look for a disabled Reference Shape component on this Entity and re-enable it if we find it
                    AZ::Entity::ComponentArrayType disabledComponents;
                    AzToolsFramework::EditorDisabledCompositionRequestBus::Event(targetEntityId, &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
                    for (auto disabledComponent : disabledComponents)
                    {
                        if (disabledComponent->RTTI_GetType() == LmbrCentral::EditorReferenceShapeComponentTypeId)
                        {
                            component = disabledComponent;

                            // Re-enable our Reference Shape component
                            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::EnableComponents, AZ::Entity::ComponentArrayType{ component });

                            // Disable any incompatible components (e.g. a previous Shape Component)
                            AzToolsFramework::EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
                            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequests::GetPendingComponentInfo, component);
                            if (!pendingComponentInfo.m_validComponentsThatAreIncompatible.empty())
                            {
                                AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::DisableComponents, pendingComponentInfo.m_validComponentsThatAreIncompatible);
                            }

                            break;
                        }
                    }

                    // If 'component' is still null then that means there is no Reference Shape component on our Entity, so we need to add one
                    if (!component)
                    {
                        AZ::ComponentId componentId = AddComponentTypeIdToEntity(targetEntityId, LmbrCentral::EditorReferenceShapeComponentTypeId);

                        component = targetEntity->FindComponent(componentId);
                    }
                }
            }
            // Otherwise, just retrieve the main component that this node represents
            else
            {
                component = targetBaseNode->GetComponent();
            }

            // Check this here because the target node might have been deleted before
            // this gets invoked (e.g. a connection being removed because a node was deleted)
            if (!component)
            {
                return;
            }

            // Iterate through the component class element edit context to expand the elements container
            // size (if necessary)
            m_serializeContext->EnumerateObject(component,
                // beginElemCB (this is called at the beginning of processing a new element)
                [this, propertyPath, elementIndexToModify, elementInContainer](void *instance, const AZ::SerializeContext::ClassData *classData, [[maybe_unused]] const AZ::SerializeContext::ClassElement *classElement) -> bool
                {
                    // If the element we are trying to set is in a container, we might need to add some more elements
                    // to the container to hold it
                    if (elementInContainer && classData && classData->m_container)
                    {
                        AZ::SerializeContext::IDataContainer* container = classData->m_container;
                        const AZ::SerializeContext::ClassElement* containerClassElement = container->GetElement(container->GetDefaultElementNameCrc());

                        // If the container already has enough elements, then we don't need to do anything with the container
                        size_t containerSize = container->Size(instance);
                        size_t requiredSize = elementIndexToModify + 1;
                        if (containerSize >= requiredSize)
                        {
                            return true;
                        }

                        if (container->IsFixedCapacity() && !container->IsSmartPointer() && requiredSize >= container->Capacity(instance))
                        {
                            GraphModel::GraphPtr graph = GetGraphById(GetActiveGraphCanvasGraphId());
                            AZ_Warning(graph->GetSystemName(), false, "Cannot add additional entries to the container as it is at its capacity of %zu", container->Capacity(instance));

                            return true;
                        }

                        // Add more elements to the container to reach the necessary size
                        while (containerSize < requiredSize)
                        {
                            // Reserve entry in the container
                            void* dataAddress = container->ReserveElement(instance, containerClassElement);

                            // Store the new element in the container
                            container->StoreElement(instance, dataAddress);

                            ++containerSize;
                        }
                    }

                    return true;
                }, {},
                AZ::SerializeContext::ENUM_ACCESS_FOR_WRITE, nullptr/* errorHandler */);

            {
                // Update the property with the new EntityId
                AzToolsFramework::ScopedUndoBatch undoBatch("Update Component Property");

                AzToolsFramework::PropertyTreeEditor pte = AzToolsFramework::PropertyTreeEditor(reinterpret_cast<void*>(component), component->RTTI_GetType());
                pte.SetProperty(propertyPath.toUtf8().constData(), AZStd::any(newEntityId));

                undoBatch.MarkEntityDirty(targetBaseNode->GetVegetationEntityId());
            }

            // Trigger property editors to update attributes/values or else they might be showing stale data
            // since we are updating the property value directly.
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_AttributesAndValues);
        });
    }

    void MainWindow::HandleSetImageAssetPath(const AZ::EntityId& sourceEntityId, const AZ::EntityId& targetEntityId)
    {
        // This only gets called when a valid connection is made between a Gradient Baker output image slot
        // (sourceEntityId) and an Image Gradient input image asset slot (targetEntityId)
        // So we need to use the corresponding request bus APIs to update the image asset path on
        // the Image Gradient
        AZ::IO::Path outputImagePath;
        GradientSignal::GradientImageCreatorRequestBus::EventResult(
            outputImagePath, sourceEntityId, &GradientSignal::GradientImageCreatorRequests::GetOutputImagePath);

        if (!outputImagePath.empty())
        {
            AzToolsFramework::ScopedUndoBatch undo("Update Image Gradient Asset");

            // The ImageGradientRequests::SetImageAssetPath only takes a product path, but we are given
            // a source asset path, so need to append the product extension
            QString imageAssetPath = QString::fromUtf8(
                outputImagePath.c_str(), static_cast<int>(outputImagePath.Native().size()));
            imageAssetPath += ".streamingimage";

            GradientSignal::ImageGradientRequestBus::Event(
                targetEntityId, &GradientSignal::ImageGradientRequests::SetImageAssetPath, imageAssetPath.toUtf8().constData());

            undo.MarkEntityDirty(targetEntityId);
        }
    }

    GraphCanvas::GraphId MainWindow::OnGraphEntity(const AZ::EntityId& entityId)
    {
        GraphCanvas::GraphId graphId;

        // If we already have a graph open for this Entity, then just focus it
        // instead of creating a new graph
        auto it = m_dockWidgetsByEntity.find(entityId);
        if (it != m_dockWidgetsByEntity.end())
        {
            GraphCanvas::DockWidgetId dockWidgetId = it->second;
            if (FocusDockWidget(dockWidgetId))
            {
                GraphCanvas::EditorDockWidgetRequestBus::EventResult(graphId, dockWidgetId, &GraphCanvas::EditorDockWidgetRequests::GetGraphId);
                return graphId;
            }
        }

        m_ignoreGraphUpdates = true;

        // Retrieve the entity being graphed so we can use the name for the graph title
        AZ::Entity* rootEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(rootEntity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        AZ_Assert(rootEntity, "No Entity found for EntityId = %s", entityId.ToString().c_str());

        // Create a new scene
        GraphCanvas::DockWidgetId dockWidgetId = CreateEditorDockWidget(rootEntity->GetName().c_str());
        GraphCanvas::EditorDockWidgetRequestBus::EventResult(graphId, dockWidgetId, &GraphCanvas::EditorDockWidgetRequests::GetGraphId);

        // If HandleGraphOpened returns true, then it means there was no previously saved graph loaded,
        // so we need to do the first time parsing/creating of nodes/connections + default node layout
        if (HandleGraphOpened(entityId, dockWidgetId))
        {
            InitialEntityGraph(entityId, graphId);
        }
        // Otherwise, we were able to load a previously saved graph so we just need to update the
        // connections 
        else
        {
            GraphModel::NodePtrList nodes;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(nodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);
            for (auto node : nodes)
            {
                UpdateConnections(node);
            }
        }

        m_ignoreGraphUpdates = false;

        // Clear the selection once we have added all the nodes, because by default nodes get
        // selected when they are added to the graph
        GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::ClearSelection);

        return graphId;
    }

    bool MainWindow::ConfigureDefaultLayout()
    {
        if (!GraphCanvas::AssetEditorMainWindow::ConfigureDefaultLayout())
        {
            return false;
        }

        // First try to close our node inspector
        if (!m_customNodeInspector->close())
        {
            return false;
        }

        // Add our custom Node Inspector to the default layout
        addDockWidget(Qt::RightDockWidgetArea, m_customNodeInspector);
        m_customNodeInspector->setFloating(false);
        m_customNodeInspector->show();

        return true;
    }

    void MainWindow::OnEditorEntityCreated(const AZ::EntityId& entityId)
    {
        // If the user has deleted an Entity and then invokes Undo, its parent
        // Entity may be deleted and then re-created as part of the restore
        // operation, so we need to queue our deletes and detect this case
        // in order to safely ignore the Entity deletion
        auto queuedIt = AZStd::find(m_queuedEntityDeletes.begin(), m_queuedEntityDeletes.end(), entityId);
        if (queuedIt != m_queuedEntityDeletes.end())
        {
            // Deleting this from the queue signifies the delete being ignored
            // when it gets invoked after the singleShot
            m_queuedEntityDeletes.erase(queuedIt);

            // If this is any other Entity besides one of our root Entities, then
            // we should still do the refresh (RefreshEntityComponentNodes) to make
            // sure any components that may have been added/removed are parsed
            auto it = m_dockWidgetsByEntity.find(entityId);
            if (it != m_dockWidgetsByEntity.end())
            {
                return;
            }
        }

        HandleEditorEntityCreated(entityId);
    }

    void MainWindow::HandleEditorEntityCreated(const AZ::EntityId& entityId, GraphCanvas::GraphId graphId)
    {
        if (m_ignoreGraphUpdates || m_prefabPropagationInProgress)
        {
            return;
        }

        // Try to find an open graph whose root Entity contains the Entity which this component was added to
        if (!graphId.IsValid())
        {
            graphId = FindGraphContainingEntity(entityId);
        }

        // If we still couldn't find a graph for this Entity, then bail out
        if (!graphId.IsValid())
        {
            return;
        }

        m_ignoreGraphUpdates = true;

        // Refresh the Entity/Component tree for this entity to create any nodes that may
        // have been added by this change.  We only need to update all connections if node(s)
        // were actually created.
        GraphModel::NodePtrList createdNodes = RefreshEntityComponentNodes(entityId, graphId);
        GraphModel::NodePtrList nodes;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);
        if (!createdNodes.empty())
        {
            for (auto node : nodes)
            {
                UpdateConnections(node);
            }
        }
        // Otherwise, we only need to update connections for nodes corresponding to this Entity
        else
        {
            for (auto node : nodes)
            {
                auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
                if (baseNodePtr->GetVegetationEntityId() == entityId)
                {
                    UpdateConnections(node);
                }
            }
        }

        m_ignoreGraphUpdates = false;
    }

    void MainWindow::OnEditorEntityDeleted(const AZ::EntityId& entityId)
    {
        if (m_prefabPropagationInProgress)
        {
            // If we get the entity deleted event while prefab propagation is in progress,
            // it means there was some kind of change that caused that entity to be rebuilt
            // that we can't track by other notification APIs (e.g. entity was added/removed
            // by undo/redo), so we will queue this entity to be refreshed after the
            // propagation is complete.
            m_queuedEntityRefresh.push_back(entityId);
            return;
        }

        m_queuedEntityDeletes.push_back(entityId);

        QTimer::singleShot(0, [this, entityId]() {
            QueuedEditorEntityDeleted(entityId);
        });
    }

    void MainWindow::QueuedEditorEntityDeleted(const AZ::EntityId& entityId)
    {
        // Check if this was a legitimate Entity deletion, or if it was just a result
        // of an undo/redo restoration
        auto queuedIt = AZStd::find(m_queuedEntityDeletes.begin(), m_queuedEntityDeletes.end(), entityId);
        if (queuedIt != m_queuedEntityDeletes.end())
        {
            m_queuedEntityDeletes.erase(queuedIt);
        }
        else
        {
            return;
        }

        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(entityId);

        HandleEditorEntityDeleted(entityId);
    }

    void MainWindow::HandleEditorEntityDeleted(const AZ::EntityId& entityId)
    {
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        m_ignoreGraphUpdates = true;

        // If the Entity deleted corresponds to one of our graphs, then close it
        auto it = m_dockWidgetsByEntity.find(entityId);
        if (it != m_dockWidgetsByEntity.end())
        {
            CloseEditor(it->second);
        }
        // Otherwise check if there are any nodes matching that Entity that need
        // to be removed
        else
        {
            for (GraphCanvas::GraphId graphId : GetOpenGraphIds())
            {
                GraphModel::NodePtrList nodes;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(nodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);

                for (auto node : nodes)
                {
                    // Ignore area extenders since those nodes will end up being removed when their wrapper node (parent) is deleted
                    auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
                    if (baseNodePtr->GetVegetationEntityId() == entityId && !baseNodePtr->IsAreaExtender())
                    {
                        GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::RemoveNode, node);
                    }
                }
            }
        }

        m_ignoreGraphUpdates = false;
    }

    void MainWindow::OnEntityPickModeStarted()
    {
        m_inObjectPickMode = true;
    }

    void MainWindow::OnEntityPickModeStopped()
    {
        m_inObjectPickMode = false;
    }

    GraphModel::NodePtrList MainWindow::GetAllNodesMatchingEntityInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& entityId)
    {
        GraphModel::NodePtrList nodes;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(
            nodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);

        nodes.erase(AZStd::remove_if(
            nodes.begin(),
            nodes.end(),
            [entityId](const GraphModel::NodePtr& nodePtr)
            {
                auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(nodePtr.get());
                return (!baseNodePtr) || (entityId != baseNodePtr->GetVegetationEntityId());

            }), nodes.end());

        return nodes;
    }

    GraphModel::NodePtrList MainWindow::GetAllNodesMatchingEntityComponentInGraph(
        const GraphCanvas::GraphId& graphId, const AZ::EntityComponentIdPair& entityComponentId)
    {
        GraphModel::NodePtrList nodes;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);

        const AZ::EntityId& entityId = entityComponentId.GetEntityId();
        const AZ::ComponentId& componentId = entityComponentId.GetComponentId();

        nodes.erase(AZStd::remove_if(
            nodes.begin(),
            nodes.end(),
            [entityId, componentId](const GraphModel::NodePtr& nodePtr)
            {
                auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(nodePtr.get());
                return (!baseNodePtr)
                    || (entityId != baseNodePtr->GetVegetationEntityId())
                    || (componentId != baseNodePtr->GetComponentId());
            }), nodes.end());

        return nodes;
    }

    GraphModel::NodePtr MainWindow::GetNodeMatchingEntityInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& entityId)
    {
        GraphModel::NodePtrList nodes = GetAllNodesMatchingEntityInGraph(graphId, entityId);
        return nodes.empty() ? nullptr : nodes.front();
    }

    GraphModel::NodePtr MainWindow::GetNodeMatchingEntityComponentInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityComponentIdPair& entityComponentId)
    {
        GraphModel::NodePtrList nodes = GetAllNodesMatchingEntityComponentInGraph(graphId, entityComponentId);
        return nodes.empty() ? nullptr : nodes.front();
    }

    GraphModel::NodePtrList MainWindow::GetAllNodesMatchingEntity(const AZ::EntityId& entityId)
    {
        GraphModel::NodePtrList matchingNodes;

        for (GraphCanvas::GraphId graphId : GetOpenGraphIds())
        {
            GraphModel::NodePtrList nodes = GetAllNodesMatchingEntityInGraph(graphId, entityId);
            matchingNodes.insert(matchingNodes.end(), nodes.begin(), nodes.end());
        }

        return matchingNodes;
    }

    GraphModel::NodePtrList MainWindow::GetAllNodesMatchingEntityComponent(const AZ::EntityComponentIdPair& entityComponentId)
    {
        GraphModel::NodePtrList matchingNodes;

        for (GraphCanvas::GraphId graphId : GetOpenGraphIds())
        {
            GraphModel::NodePtrList nodes = GetAllNodesMatchingEntityComponentInGraph(graphId, entityComponentId);
            matchingNodes.insert(matchingNodes.end(), nodes.begin(), nodes.end());
        }

        return matchingNodes;
    }

    void MainWindow::UpdateConnections(GraphModel::NodePtr node)
    {
        // Retrieve all the input data connections for this node that would be expected
        // based on the component property fields.  If this differs from what is actually
        // connected for the slots on this node, then we will need to update (add/remove)
        // the connections so that they match.
        ConnectionsList expectedConnections;
        GraphCanvas::GraphId graphId = GetGraphId(node->GetGraph());
        ParseNodeConnections(graphId, node, expectedConnections);

        // Iterate through the input data slots on this node to check for
        // existing connections that satisfy our expected connections, and to
        // remove any current connections that aren't in our expected list.
        for (auto slotPair : node->GetSlots())
        {
            GraphModel::SlotPtr slot = slotPair.second;

            // We only care about input data slots because those are the only slots
            // that could be modified when a Component on an Entity is changed,
            // which is what triggers OnEntityComponentPropertyChanged
            if (!slot->Is(GraphModel::SlotDirection::Input, GraphModel::SlotType::Data))
            {
                continue;
            }

            // If there aren't any connections to this slot, we can skip it
            auto slotConnections = slot->GetConnections();
            if (slotConnections.empty())
            {
                continue;
            }

            // Input data slots will only have one connection
            GraphModel::ConnectionPtr connection = *slotConnections.begin();

            // Check if this connection matches one in our list of expected connections
            bool matchesExisting = false;
            for (auto it = expectedConnections.begin(); it != expectedConnections.end(); ++it)
            {
                auto sourceNode = it->first.first;
                auto sourceSlot = it->first.second;
                auto targetNode = it->second.first;
                auto targetSlot = it->second.second;

                // If we found a matching connection, then remove it from our list of expected
                // so we don't have to process it after we are done checking all the slots
                // on the node
                if (sourceNode == connection->GetSourceNode() && sourceSlot == connection->GetSourceSlot() &&
                    targetNode == connection->GetTargetNode() && targetSlot == connection->GetTargetSlot())
                {
                    matchesExisting = true;
                    expectedConnections.erase(it);
                    break;
                }
            }

            // If this connection doesn't match an expected connection, then it needs to be removed
            if (!matchesExisting)
            {
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::RemoveConnection, connection);
            }
        }

        // For the remaining expected connections, this means they didn't exist already,
        // so we need to create them
        for (auto it : expectedConnections)
        {
            GraphModel::SlotPtr sourceSlot = it.first.second;
            GraphModel::SlotPtr targetSlot = it.second.second;

            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::AddConnection, sourceSlot, targetSlot);
        }
    }

    GraphCanvas::GraphId MainWindow::FindGraphContainingEntity(const AZ::EntityId& entityId)
    {
        GraphCanvas::GraphId graphId;

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!entity)
        {
            return graphId;
        }

        AZ::EntityId parentEntityId = entityId;

        AZ::EntityId levelEntityId;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &AzToolsFramework::ToolsApplicationRequests::GetCurrentLevelEntityId);

        // Crawl up the Entity hierarchy looking for a matching open graph.
        // Stop the loop if we encounter the Level Entity, which can be hit here when
        // components are added/removed via the Level Inspector.
        while (parentEntityId.IsValid() && parentEntityId != levelEntityId)
        {
            auto it = m_dockWidgetsByEntity.find(parentEntityId);
            if (it != m_dockWidgetsByEntity.end())
            {
                const GraphCanvas::DockWidgetId& dockWidgetId = it->second;
                GraphCanvas::EditorDockWidgetRequestBus::EventResult(graphId, dockWidgetId, &GraphCanvas::EditorDockWidgetRequests::GetGraphId);
                break;
            }
            else
            {
                AZ::EntityId previousParentEntityId = parentEntityId;

                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentEntityId, parentEntityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

                // Prevent infinite loop if the GetParent ends up returning itself, which could happen in a case where a slice is in
                // the process of being restored and this logic gets invoked.
                if (previousParentEntityId == parentEntityId)
                {
                    AZ_Assert(false, "Corrupt parent hierarchy - entity parent ID is set to itself, breaking here to prevent infinite loop.");
                    break;
                }
            }
        }

        return graphId;
    }

    void MainWindow::EnumerateEntityComponentTree(const AZ::EntityId& rootEntityId, EntityComponentCallback callback)
    {
        // Retrieve the entity hierarchy for our root entity
        AzToolsFramework::EntityIdList children;
        children.push_back(rootEntityId);
        GetChildrenTree(rootEntityId, children);

        // Iterate through our entity hierarchy and invoke our callback on all
        // components that are found (both enabled and disabled)
        for (auto entityId : children)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            if (!entity)
            {
                continue;
            }

            // Retrieve the enabled components on our Entity
            AZ::Entity::ComponentArrayType components = entity->GetComponents();
            for (AZ::Component* component : components)
            {
                callback(entityId, component, false);
            }

            // If there are any disabled components on our Entity, we need to
            // retrieve them separately because they won't show up with Entity::GetComponents()
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* disabledComponent : disabledComponents)
            {
                callback(entityId, disabledComponent, true);
            }
        }
    }

    void MainWindow::InitialEntityGraph(const AZ::EntityId& entityId, GraphCanvas::GraphId graphId)
    {
        // Keep track of our node points for creating a better default node layout
        NodePoint* rootPoint = new NodePoint();
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<NodePoint*>> nodePointMap;

        // Keep track of any node wrappings we will need to setup after the nodes
        // have been added to the graph
        AZStd::unordered_map<AZ::EntityId, GraphModel::NodePtrList> nodeWrappings;

        // We don't need to cache a mapping of the area extenders since they don't have
        // output slots that connect to other nodes
        AZStd::vector<LandscapeCanvas::BaseNode::BaseNodePtr> areaExtenders;

        // Iterate through our entity hierarchy to look for components that
        // correspond with nodes we know how to graph
        GraphModel::NodePtrList disabledNodes;
        GraphModel::GraphPtr graph = GetGraphById(graphId);
        EnumerateEntityComponentTree(entityId, [this, graph, graphId, rootPoint, &nodePointMap, &nodeWrappings, &areaExtenders, &disabledNodes](const AZ::EntityId& entityId, AZ::Component* component, bool isDisabled) {
            const AZ::TypeId& componentTypeId = component->RTTI_GetType();

            // Create the node for the given component type.
            // If we don't support a node for this component type, it will just return nullptr.
            LandscapeCanvas::BaseNode::BaseNodePtr node;
            LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(node, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::CreateNodeForType, graph, componentTypeId);

            // Set the EntityId for the vegetation entity corresponding to this node (if we found one)
            if (node)
            {
                node->SetVegetationEntityId(entityId);
                node->SetComponentId(component->GetId());

                // Update the node mappings we need to cache for this node
                UpdateEntityIdNodeMap(graphId, node);

                // Keep track of which nodes came from disabled components so that we can disable
                // those nodes once they are added to the graph
                if (isDisabled)
                {
                    disabledNodes.push_back(node);
                }

                // Keep track locally of our area extenders so we can parse them later
                LandscapeCanvas::BaseNode::BaseNodeType baseNodeType = node->GetBaseNodeType();
                switch (baseNodeType)
                {
                case LandscapeCanvas::BaseNode::TerrainExtender:
                case LandscapeCanvas::BaseNode::VegetationAreaFilter:
                case LandscapeCanvas::BaseNode::VegetationAreaModifier:
                case LandscapeCanvas::BaseNode::VegetationAreaSelector:
                    areaExtenders.push_back(node);
                    break;
                }

                // If this node is meant to be wrapped on a WrapperNode, then
                // add it to the node wrappings so we can wrap it later after
                // the nodes have been added to the graph
                if (node->IsAreaExtender())
                {
                    auto nodeWrapIt = nodeWrappings.find(entityId);
                    if (nodeWrapIt == nodeWrappings.end())
                    {
                        nodeWrappings[entityId] = { node };
                    }
                    else
                    {
                        nodeWrappings[entityId].push_back(node);
                    }
                }
                // Otherwise, create a new node point for this general node and just place it as a child on our root
                else
                {
                    NodePoint* point = new NodePoint();
                    point->node = node;
                    point->vegetationEntityId = entityId;
                    point->parent = rootPoint;
                    rootPoint->children.push_back(point);
                    nodePointMap[entityId].push_back(point);
                }
            }
        });

        // Find connections between nodes. Save the corresponding node for the slot in a pair, because
        // we can't retrieve the parent node from the Slot until the node has been added to the graph, but we need
        // to match based on that data to place nodes near eachother that have slots connected.
        ConnectionsList connections;
        const EntityIdNodeMaps& nodeMaps = m_entityIdNodeMapsByGraph[graphId];
        for (auto nodeType : { EntityIdNodeMapEnum::Gradients, EntityIdNodeMapEnum::WrapperNodes })
        {
            const EntityIdNodeMap& nodeMap = nodeMaps[nodeType];
            for (const auto& it : nodeMap)
            {
                GraphModel::NodePtr node = it.second;
                ParseNodeConnections(graphId, node, connections);
            }
        }
        for (const auto& it : areaExtenders)
        {
            ParseNodeConnections(graphId, it, connections);
        }

        // Use the connections between nodes to setup the node point tree so
        // that nodes that are connected together are:
        //   1. Placed near eachother
        //   2. Target nodes are placed to the right of the source node
        // When the node points are created, they are all placed as children on
        // a dummy root node point, so any nodes that don't have connections will
        // be placed at the bottom in a vertical column. The tree is connection type
        // agnostic, so it doesn't matter whether a Shape is connected to a Gradient,
        // or a Gradient is connecte to a Gradient Modifier, any nodes that are connected
        // will be placed in a left to right flow, and also handles if one node has multiple
        // output slots connected to multiple nodes. As we continue to add support for more
        // connections, they will automatically be handled by this logic.
        for (auto it : connections)
        {
            GraphModel::NodePtr sourceNode = it.first.first;
            GraphModel::NodePtr targetNode = it.second.first;

            auto sourceBaseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(sourceNode.get());
            auto targetBaseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(targetNode.get());
            AZ::EntityId sourceEntityId = sourceBaseNodePtr->GetVegetationEntityId();
            AZ::EntityId targetEntityId = targetBaseNodePtr->GetVegetationEntityId();

            // Find the source and target NodePoints from the map. There may be multiple
            // NodePoints for a single Vegetation EntityId in the case where multiple
            // components are on the same Entity, so if there's more than one entry
            // we need to search and match based on the NodePtr.
            auto sourcePoints = nodePointMap[sourceEntityId];
            auto targetPoints = nodePointMap[targetEntityId];
            NodePoint* sourcePoint = sourcePoints.size() == 1 ? sourcePoints[0] : FindNodePoint(sourcePoints, nodeWrappings, sourceNode);
            NodePoint* targetPoint = targetPoints.size() == 1 ? targetPoints[0] : FindNodePoint(targetPoints, nodeWrappings, targetNode);
            if (!sourcePoint || !targetPoint)
            {
                AZ_Error(graph->GetSystemName(), false, "Invalid source or target point connection");
                continue;
            }

            // Add this target node as one of the children from the source node
            sourcePoint->children.push_back(targetPoint);

            // If the target already had a parent, remove it as a child
            if (targetPoint->parent)
            {
                NodePoint* parentPoint = targetPoint->parent;
                auto iter = AZStd::find(parentPoint->children.begin(), parentPoint->children.end(), targetPoint);
                if (iter != parentPoint->children.end())
                {
                    parentPoint->children.erase(iter);
                }
            }

            // Then set the new parent for our target
            targetPoint->parent = sourcePoint;
        }

        // Place the nodes in a tree layout grouped by their connections
        AZ::Vector2 gridMajorPitch;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(gridMajorPitch, graphId, &GraphModelIntegration::GraphControllerRequests::GetMajorPitch);
        PlaceNodes(graphId, rootPoint, gridMajorPitch);

        // Setup the node wrappings now that the nodes have been placed in the graph
        for (auto it : nodeWrappings)
        {
            const AZ::EntityId& wrapperNodeEntityId = it.first;
            auto nodePointIt = nodePointMap.find(wrapperNodeEntityId);
            if (nodePointIt == nodePointMap.end())
            {
                continue;
            }

            // Find the wrapper node for this EntityId. There could be multiple nodes with the same
            // EntityId (e.g. box shapes), but there can't be multiple wrapper nodes on the same Entity.
            GraphModel::NodePtr wrapperNode = nullptr;
            for (auto nodePoint : nodePointIt->second)
            {
                if (nodePoint->node->GetNodeType() == GraphModel::NodeType::WrapperNode)
                {
                    wrapperNode = nodePoint->node;
                    break;
                }
            }

            GraphModel::NodePtrList wrappedNodes = it.second;
            for (GraphModel::NodePtr node : wrappedNodes)
            {
                // Wrap the node using its preferred layout order (if it has one)
                AZ::u32 layoutOrder = GetWrappedNodeLayoutOrder(node);
                if (layoutOrder != GraphModel::DefaultWrappedNodeLayoutOrder)
                {
                    GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::WrapNodeOrdered, wrapperNode, node, layoutOrder);
                }
                else
                {
                    GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::WrapNode, wrapperNode, node);
                }
            }
        }

        // Delete the node points now that we've completed placing the nodes
        delete rootPoint;
        for (auto it : nodePointMap)
        {
            for (auto pointIt : it.second)
            {
                delete pointIt;
            }
        }

        // Disable any nodes that came from disabled components now that they've all been added to the graph
        for (auto node : disabledNodes)
        {
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::DisableNode, node);
        }

        // Create the connections now, after placing the nodes, since the
        // connection data is used for appropriate node placement
        for (auto it : connections)
        {
            GraphModel::SlotPtr sourceSlot = it.first.second;
            GraphModel::SlotPtr targetSlot = it.second.second;
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::AddConnection, sourceSlot, targetSlot);
        }
    }

    GraphModel::NodePtrList MainWindow::RefreshEntityComponentNodes(const AZ::EntityId& targetEntityId, GraphCanvas::GraphId graphId)
    {
        GraphModel::GraphPtr graph = GetGraphById(graphId);
        GraphModel::NodePtrList loadedNodes, disabledNodes, createdNodes;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(loadedNodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);

        EnumerateEntityComponentTree(
            targetEntityId,
            [this, graph, graphId, &loadedNodes, &disabledNodes, &createdNodes](
                const AZ::EntityId& entityId, AZ::Component* component, bool isDisabled)
            {
            bool foundMatch = false;
            GraphModel::NodePtr validNode = nullptr;

            // Check if this component matches a node that was already loaded in the graph
            for (auto it = loadedNodes.begin(); it != loadedNodes.end(); ++it)
            {
                GraphModel::NodePtr node = *it;
                auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
                AZ::ComponentId componentId = component->GetId();
                if (entityId == baseNodePtr->GetVegetationEntityId() && componentId == baseNodePtr->GetComponentId())
                {
                    foundMatch = true;
                    validNode = node;

                    // Erase this from our list of loaded nodes so that we know we found its match
                    // After we iterate through the Entity/Component tree, anything left in loadedNodes
                    // will represent saved nodes that no longer have a corresponding Entity/Component in the level
                    loadedNodes.erase(it);
                    break;
                }
            }

            // If we didn't find a match for this component, check if this is a newly added component we need to
            // create a node for
            if (!foundMatch)
            {
                const AZ::TypeId& componentTypeId = component->RTTI_GetType();

                // Try to create the node for the given component type.
                // If we don't support a node for this component type, it will just return nullptr.
                LandscapeCanvas::BaseNode::BaseNodePtr node;
                LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(node, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::CreateNodeForType, graph, componentTypeId);

                if (node)
                {
                    validNode = node;
                    createdNodes.push_back(node);
                    node->SetVegetationEntityId(entityId);
                    node->SetComponentId(component->GetId());

                    PlaceNewNode(graphId, node);
                }
            }

            if (validNode)
            {
                if (isDisabled)
                {
                    disabledNodes.push_back(validNode);
                }

                // Update the node mappings we need to cache for this node
                UpdateEntityIdNodeMap(graphId, validNode);
            }
        });

        // Disable any nodes that came from disabled components now that they've all been added to the graph
        for (auto node : disabledNodes)
        {
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::DisableNode, node);
        }

        // Anything left in 'loadedNodes' at this point after the enumerate is done can be
        // deleted if we were refreshing the the root Entity for this graph, since that means
        // there's no longer an existing component matching it
        if (targetEntityId == GetRootEntityIdForGraphId(graphId))
        {
            for (auto node : loadedNodes)
            {
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::RemoveNode, node);
            }
        }

        return createdNodes;
    }

    void MainWindow::OnEntityComponentAdded(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        // Try to find an open graph whose root Entity contains the Entity which this component was added to
        GraphCanvas::GraphId graphId = FindGraphContainingEntity(entityId);
        if (!graphId.IsValid())
        {
            return;
        }

        // When OnEntityComponentAdded is called, the component won't be accessible by Entity::FindComponent yet, it will still
        // be pending even whether it is disabled or not
        AZ::Component* component = nullptr;
        AZ::Entity::ComponentArrayType pendingComponents;
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
        for (AZ::Component* pendingComponent : pendingComponents)
        {
            if (pendingComponent->GetId() == componentId)
            {
                component = pendingComponent;
                break;
            }
        }

        if (!component)
        {
            return;
        }

        // Create the node for the given component type.
        // If we don't support a node for this component type, it will just return nullptr.
        LandscapeCanvas::BaseNode::BaseNodePtr node;
        GraphModel::GraphPtr graph = GetGraphById(graphId);
        const AZ::TypeId& componentTypeId = component->RTTI_GetType();
        LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(node, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::CreateNodeForType, graph, componentTypeId);

        if (!node)
        {
            return;
        }

        // Set the EntityId for the vegetation entity corresponding to this node (if we found one)
        node->SetVegetationEntityId(entityId);
        node->SetComponentId(componentId);

        // Update the node mappings we need to cache for this node and parse any connections that it may have setup already
        UpdateEntityIdNodeMap(graphId, node);
        ConnectionsList connections;
        ParseNodeConnections(graphId, node, connections);

        m_ignoreGraphUpdates = true;

        // Add the node to the graph, either wrapped on its parent or just in the scene if it's standalone
        PlaceNewNode(graphId, node);

        // Disable this node for now since it's pending when OnEntityComponentAdded is called, it will be enabled
        // after if it becomes enabled
        GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::DisableNode, node);

        // Create connections if any exist (e.g. if a component was copied/pasted with existing configuration)
        for (auto it : connections)
        {
            GraphModel::SlotPtr sourceSlot = it.first.second;
            GraphModel::SlotPtr targetSlot = it.second.second;
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::AddConnection, sourceSlot, targetSlot);
        }

        m_ignoreGraphUpdates = false;

        // As mentioned earlier, the component added when OnEntityComponentAdded is called is still pending currently,
        // so we need to delay checking until after this event is invoked to see if the component was enabled
        QTimer::singleShot(0, [this, entityId, componentId, graphId, node]() {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            if (!entity)
            {
                return;
            }

            AZ::Component* component = entity->FindComponent(componentId);
            if (component)
            {
                // If FindComponent succeeds, then the component has been enabled
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::EnableNode, node);

                // Also check if any other previously deactivated (pending) components on this same Entity were activated
                // when this new component was added (e.g. a random noise gradient component being activated once the
                // gradient transform modifier and shape are added)
                auto nodeMapsIt = m_entityIdNodeMapsByGraph.find(graphId);
                if (nodeMapsIt != m_entityIdNodeMapsByGraph.end())
                {
                    const EntityIdNodeMaps& nodeMaps = nodeMapsIt->second;
                    for (int i = 0; i < EntityIdNodeMapEnum::Count; ++i)
                    {
                        const auto& nodeMap = nodeMaps[i];
                        auto it = nodeMap.find(entityId);
                        if (it != nodeMap.end())
                        {
                            GraphModel::NodePtr cachedNode = it->second;
                            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(cachedNode.get());

                            // Ignore node matching the same componentId as the component that was directly added
                            // If the GetComponent() method returns a valid pointer, it means the component is enabled now
                            if ((baseNodePtr->GetComponentId() != componentId) && baseNodePtr->GetComponent())
                            {
                                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::EnableNode, cachedNode);
                            }
                        }
                    }
                }
            }
        });
    }

    void MainWindow::PlaceNewNode(GraphCanvas::GraphId graphId, LandscapeCanvas::BaseNode::BaseNodePtr node)
    {
        // If this is an extender node, then we need to wrap it to its parent node
        if (node->IsAreaExtender())
        {
            auto nodeMapsIt = m_entityIdNodeMapsByGraph.find(graphId);
            if (nodeMapsIt == m_entityIdNodeMapsByGraph.end())
            {
                return;
            }

            const EntityIdNodeMaps& nodeMaps = nodeMapsIt->second;
            const auto& wrapperNodeMap = nodeMaps[EntityIdNodeMapEnum::WrapperNodes];
            auto it = wrapperNodeMap.find(node->GetVegetationEntityId());
            if (it != wrapperNodeMap.end())
            {
                GraphModel::NodePtr wrapperNode = it->second;
                AZ::u32 layoutOrder = GetWrappedNodeLayoutOrder(node);
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::WrapNodeOrdered, wrapperNode, node, layoutOrder);

                // Some nodes could be wrapped or free floating, so if this was a wrapped node, we can stop now
                // Otherwise, we need to fall-through and just place it in the graph
                return;
            }
        }

        // If we aren't placing a wrapped node, then just add it to the graph
        AZ::Vector2 nodePosition = AZ::Vector2::CreateZero();
        auto it = m_deletedNodePositions.find(graphId);
        if (it != m_deletedNodePositions.end())
        {
            // Check if there was a saved position from a previous node with matching Entity/Component pair
            // that had been previously deleted, so that we can handle Undo/Redo placing the re-created
            // node back in the same position
            const DeletedNodePositionsMap& deletedNodePositionMap = it->second;
            AZ::EntityComponentIdPair pair(node->GetVegetationEntityId(), node->GetComponentId());
            auto deletedPositionIt = deletedNodePositionMap.find(pair);
            if (deletedPositionIt != deletedNodePositionMap.end())
            {
                nodePosition = deletedPositionIt->second;
            }
            // Otherwise, this really is a new node, so place it outside the top-left edge of the bounds of all nodes in the scene
            else
            {
                QRectF sceneArea;
                GraphCanvas::SceneRequestBus::EventResult(sceneArea, graphId, &GraphCanvas::SceneRequests::GetSceneBoundingArea);
                nodePosition = AZ::Vector2(aznumeric_cast<float>(sceneArea.right()) + NODE_OFFSET_X_PIXELS, aznumeric_cast<float>(sceneArea.top()));
            }
        }

        GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::AddNode, node, nodePosition);
    }

    void MainWindow::OnEntityComponentRemoved(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        if (m_ignoreGraphUpdates)
        {
            return;
        }

        m_ignoreGraphUpdates = true;

        for (GraphCanvas::GraphId graphId : GetOpenGraphIds())
        {
            GraphModel::NodePtrList nodes;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(nodes, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodes);

            for (auto node : nodes)
            {
                auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
                if (baseNodePtr->GetVegetationEntityId() == entityId && baseNodePtr->GetComponentId() == componentId)
                {
                    GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::RemoveNode, node);
                    break;
                }
            }
        }

        m_ignoreGraphUpdates = false;
    }

    void MainWindow::OnEntityComponentEnabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        AZ::EntityComponentIdPair entityComponentId(entityId, componentId);
        GraphModel::NodePtrList matchingNodes = GetAllNodesMatchingEntityComponent(entityComponentId);
        for (auto node : matchingNodes)
        {
            GraphCanvas::GraphId graphId = GetGraphId(node->GetGraph());
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::EnableNode, node);
        }
    }

    void MainWindow::OnEntityComponentDisabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        AZ::EntityComponentIdPair entityComponentId(entityId, componentId);
        GraphModel::NodePtrList matchingNodes = GetAllNodesMatchingEntityComponent(entityComponentId);
        for (auto node : matchingNodes)
        {
            GraphCanvas::GraphId graphId = GetGraphId(node->GetGraph());
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::DisableNode, node);
        }
    }

    void MainWindow::OnEntityComponentPropertyChanged(AZ::ComponentId changedComponentId)
    {
        AZ_UNUSED(changedComponentId);

        const AZ::EntityId changedEntityId = *AzToolsFramework::PropertyEditorEntityChangeNotificationBus::GetCurrentBusId();

        auto ignoreIt = AZStd::find(m_ignoreEntityComponentPropertyChanges.begin(), m_ignoreEntityComponentPropertyChanges.end(), changedEntityId);
        if (ignoreIt != m_ignoreEntityComponentPropertyChanges.end())
        {
            return;
        }

        GraphModel::NodePtrList matchingNodes = GetAllNodesMatchingEntity(changedEntityId);
        for (auto node : matchingNodes)
        {
            // Re-parse any input connections for this node to add/remove any connections
            // that might've been modified when the component/property was changed
            UpdateConnections(node);
        }
    }

    void MainWindow::EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
    {
        if (m_prefabPropagationInProgress)
        {
            return;
        }

        GraphCanvas::GraphId oldGraphId = FindGraphContainingEntity(oldParentId);
        GraphCanvas::GraphId newGraphId = FindGraphContainingEntity(newParentId);

        // If the Entity is being re-parented but still inside the same graph, then we don't need to do anything
        // This will also trigger if the Entity isn't in a currently open graph, in which case we can also ignore
        if (newGraphId == oldGraphId)
        {
            return;
        }

        // If there is an open graph for the previous parent, then treat this like the Entity being deleted
        if (oldGraphId.IsValid())
        {
            HandleEditorEntityDeleted(entityId);
        }

        // If there is an open graph for the new parent, then treat this like an Entity being created
        if (newGraphId.IsValid())
        {
            // We need to pass in the new graphId for the new parentEntity because when EntityParentChanged
            // is invoked, the EditorEntityInfoRequestBus::Events::GetParent (that is used by FindGraphContainingEntity)
            // will still return the old parentId
            HandleEditorEntityCreated(entityId, newGraphId);
        }
    }

    void MainWindow::OnPrefabFocusChanged(
        [[maybe_unused]] AZ::EntityId previousContainerEntityId, [[maybe_unused]] AZ::EntityId newContainerEntityId)
    {
        // Make sure to close any open graphs that aren't currently in prefab focus
        // to prevent the user from making modifications outside of the allowed focus scope
        AZStd::vector<GraphCanvas::DockWidgetId> dockWidgetsToClose;
        for (auto [entityId, dockWidgetId] : m_dockWidgetsByEntity)
        {
            if (!m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
            {
                dockWidgetsToClose.push_back(dockWidgetId);
            }
        }

        for (auto dockWidgetId : dockWidgetsToClose)
        {
            CloseEditor(dockWidgetId);
        }
    }

    void MainWindow::OnPrefabInstancePropagationBegin()
    {
        // Ignore graph updates during prefab propagation because the entities will be
        // deleted and re-created, which would inadvertantly trigger our logic to close
        // the graph when the corresponding entity is deleted.
        m_prefabPropagationInProgress = true;
    }

    void MainWindow::OnPrefabInstancePropagationEnd()
    {
        // See comment above in OnPrefabInstancePropagationBegin
        m_prefabPropagationInProgress = false;

        // Clear our list of EntityIds to ignore component property change notifications
        // from since the prefab propagation has completed
        m_ignoreEntityComponentPropertyChanges.clear();

        // After prefab propagation is complete, the entity tied to one of our open
        // graphs might have been deleted (e.g. if a prefab was created from that entity).
        // Any open graphs tied to an entity that no longer exists will need to be closed.
        // We need to close them in a separate iterator because the CloseEditor API will
        // end up modifying m_dockWidgetsByEntity.
        AZStd::vector<GraphCanvas::DockWidgetId> dockWidgetsToDelete;
        for (auto [entityId, dockWidgetId] : m_dockWidgetsByEntity)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            if (!entity)
            {
                dockWidgetsToDelete.push_back(dockWidgetId);
            }
        }
        for (auto dockWidgetId : dockWidgetsToDelete)
        {
            CloseEditor(dockWidgetId);
        }

        // Handle any nodes that might've been created by duplicated/pasted entities
        // once the prefab propagation has finished
        HandleDeserializedNodes();

        // Handle any queued entities that we need to refresh by calling
        // HandleEditorEntityCreated, which will handle if there is anything
        // out of sync in the graph based on the corresponding entity.
        for (const auto& entityId : m_queuedEntityRefresh)
        {
            HandleEditorEntityCreated(entityId);
        }
        m_queuedEntityRefresh.clear();
    }

    void MainWindow::OnCryEditorEndCreate()
    {
        UpdateGraphEnabled();
    }

    void MainWindow::OnCryEditorEndLoad()
    {
        UpdateGraphEnabled();

        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void MainWindow::OnCryEditorCloseScene()
    {
        UpdateGraphEnabled();

        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    void MainWindow::OnCryEditorSceneClosed()
    {
        UpdateGraphEnabled();

        // Close all the open editor graphs when the level is closed, and stop listening
        // for Editor Entity property changes since our graphs are tied to the level data
        CloseAllEditors();
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect();
    }

    void MainWindow::UpdateGraphEnabled()
    {
        bool isLevelLoaded = GetLegacyEditor()->IsLevelLoaded();

        // Disable being able to drag from the node palette to the empty dock window
        // to create a new graph when a level isn't loaded
        GetCentralDockWindow()->GetEmptyDockWidget()->setAcceptDrops(isLevelLoaded);

        // Disable the new graph menu action when no level is loaded
        if (m_fileNewAction)
        {
            m_fileNewAction->setEnabled(isLevelLoaded);
        }

        // Extra safety check to prevent our tool from creating Entities if a node is added to a graph
        // This in theory shouldn't be hit since we are preventing new graphs from being created
        // in the first place, but is just an extra precaution
        m_ignoreGraphUpdates = !isLevelLoaded;
    }

    void MainWindow::PostOnActiveGraphChanged()
    {
        // Update our selection in our custom Node Inspector when the active graph changes
        OnSelectionChanged();
    }

    AZ::u32 MainWindow::GetWrappedNodeLayoutOrder(GraphModel::NodePtr node)
    {
        AZ::u32 layoutOrder = GraphModel::DefaultWrappedNodeLayoutOrder;
        if (!node)
        {
            return layoutOrder;
        }

        auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        if (!baseNodePtr)
        {
            return layoutOrder;
        }

        // Find the layout order for the wrapped node
        int index = -1;
        LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(index, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::GetNodeRegisteredIndex, baseNodePtr->RTTI_GetType());
        if (index != -1)
        {
            return index;
        }

        return layoutOrder;
    }

    AZ::EntityId MainWindow::GetRootEntityIdForGraphId(const GraphCanvas::GraphId& graphId)
    {
        for (const auto& pair : m_dockWidgetsByEntity)
        {
            const GraphCanvas::DockWidgetId& dockWidgetId = pair.second;

            GraphCanvas::GraphId dockGraphId;
            GraphCanvas::EditorDockWidgetRequestBus::EventResult(dockGraphId, dockWidgetId, &GraphCanvas::EditorDockWidgetRequests::GetGraphId);
            if (dockGraphId == graphId)
            {
                return pair.first;
            }
        }

        return AZ::EntityId();
    }

    AZ::ComponentId MainWindow::AddComponentTypeIdToEntity(
        const AZ::EntityId& entityId, AZ::TypeId componentToAddTypeId, AZStd::span<const AZ::ComponentServiceType> optionalServices)
    {
        using namespace AzToolsFramework;

        // Cache the original m_ignoreGraphUpdates so we can restore it later
        bool originalIgnoreGraphUpdates = m_ignoreGraphUpdates;

        // Add the corresponding Component for this node to its representative Entity,
        // and any required Components it may need by keeping track of any missing required
        // services that are reported when the Component(s) are added
        // Initialize our list of missing required services with any optional services this component needs
        AZ::ComponentDescriptor::DependencyArrayType missingRequiredServices(optionalServices.begin(), optionalServices.end());
        AZ::ComponentId requestedComponentId = AZ::InvalidComponentId;
        do
        {
            AZ::ComponentDescriptor* componentDescriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(componentDescriptor, componentToAddTypeId, &AZ::ComponentDescriptor::GetDescriptor);
            AZ_Assert(componentDescriptor, "Unable to find ComponentDescriptor for %s.", componentToAddTypeId.ToString<AZStd::string>().c_str());

            // Find what (if any) services are provided by the Component we are about to add,
            // and remove them from the list of missing required services are are tracking
            AZ::ComponentDescriptor::DependencyArrayType providedServices;
            componentDescriptor->GetProvidedServices(providedServices, nullptr);
            for (const auto& service : providedServices)
            {
                auto it = AZStd::find(missingRequiredServices.begin(), missingRequiredServices.end(), service);
                if (it != missingRequiredServices.end())
                {
                    missingRequiredServices.erase(it);
                }
            }

            // Add the Component to the Vegetation Entity
            EntityCompositionRequests::AddComponentsOutcome outcome = AZ::Failure(AZStd::string());
            EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::AddComponentsToEntities, EntityIdList{ entityId }, AZ::ComponentTypeList{ componentToAddTypeId });
            AZ_Assert(outcome.IsSuccess(), "Failed to add component %s", componentToAddTypeId.ToString<AZStd::string>().c_str());

            // Capture the ComponentId for the original component type that was requested to be added
            if (requestedComponentId == AZ::InvalidComponentId)
            {
                const AZ::Entity::ComponentArrayType& componentsAdded = outcome.GetValue()[entityId].m_componentsAdded;
                AZ_Assert(!componentsAdded.empty(), "Failed to add component %s", componentToAddTypeId.ToString<AZStd::string>().c_str());

                requestedComponentId = componentsAdded.front()->GetId();
            }

            // After the Component has been added, check if it is missing any required services
            // by checking the m_addedPendingComponents property in the outcome, which means
            // the Component was added to the Entity, but is missing one or more required services.
            // If m_addedPendingComponents is empty, then that means the Component was added
            // with no issues, so we can continue.
            const AZ::Entity::ComponentArrayType& pendingComponents = outcome.GetValue()[entityId].m_addedPendingComponents;
            if (!pendingComponents.empty())
            {
                AZ::Component* component = pendingComponents.front();

                // Find the missing required services for the pending Component,
                // and them to our list (if it wasn't in the list already).
                EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
                EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &EntityCompositionRequests::GetPendingComponentInfo, component);
                for (const auto& service : pendingComponentInfo.m_missingRequiredServices)
                {
                    if (AZStd::find(missingRequiredServices.begin(), missingRequiredServices.end(), service) == missingRequiredServices.end())
                    {
                        missingRequiredServices.push_back(service);
                    }
                }

                // Disable any components that are incompatible with the component we have added
                if (!pendingComponentInfo.m_validComponentsThatAreIncompatible.empty())
                {
                    AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::DisableComponents, pendingComponentInfo.m_validComponentsThatAreIncompatible);
                }
            }

            // If we are missing any required services, use the ComponentPaletteUtil::ComponentDataTable to find
            // what components will satisfy them, then choose one to be added and repeat the loop so we can find
            // any additional required services that Component may need
            if (!missingRequiredServices.empty())
            {
                ComponentPaletteUtil::ComponentDataTable componentDataTable;
                ComponentPaletteUtil::ComponentIconTable componentIconTable;
                ComponentPaletteUtil::BuildComponentTables(m_serializeContext, AppearsInGameComponentMenu, missingRequiredServices, componentDataTable, componentIconTable);
                AZ_Assert(componentDataTable.size(), "No components found that satisfy the missing required service(s).");

                componentToAddTypeId = PickComponentTypeIdToAdd(componentDataTable);
            }

            // After adding the first component, re-enable listening to graph updates
            // This handles the case where we add dependent components that have
            // corresponding nodes we want to see in the graph
            m_ignoreGraphUpdates = false;

        } while (!missingRequiredServices.empty());

        // Restore m_ignoreGraphUpdates to its original value now that we've added the intended component
        // and all its dependencies
        m_ignoreGraphUpdates = originalIgnoreGraphUpdates;

        return requestedComponentId;
    }

    void MainWindow::HandleNodeCreated(GraphModel::NodePtr node)
    {
        using namespace LandscapeCanvas;

        if (m_ignoreGraphUpdates)
        {
            return;
        }

        // Ignore for wrapped nodes that were added since we don't want to
        // create a new Entity for them. Adding their component will be handled
        // later when the OnGraphModelNodeWrapped event gets called.
        auto wrappedNodeIt = AZStd::find(m_addedWrappedNodes.begin(), m_addedWrappedNodes.end(), node);
        if (wrappedNodeIt != m_addedWrappedNodes.end())
        {
            return;
        }

        auto* baseNodePtr = static_cast<BaseNode*>(node.get());
        if (!baseNodePtr)
        {
            return;
        }

        GraphCanvas::GraphId graphId = (*GraphModelIntegration::GraphControllerNotificationBus::GetCurrentBusId());
        AZ::EntityId rootEntityId = GetRootEntityIdForGraphId(graphId);
        if (!rootEntityId.IsValid())
        {
            AZ_Assert(false, "No root Entity associated with this graph.");
            return;
        }

        m_ignoreGraphUpdates = true;

        // If the new node already has a valid EntityId, then it means the node was copy/pasted, so we need
        // to find the corresponding deserialized Entity and fix-up the references.
        // However, the new deserialized entities/components won't be available until the propagation is
        // complete, so we'll need to keep track of the deserialized nodes and then handle the fix-up after.
        AZ::EntityId existingEntityId = baseNodePtr->GetVegetationEntityId();
        if (existingEntityId.IsValid())
        {
            m_deserializedNodes.push_back(node);
        }
        // Otherwise, this new node was created by the user from the node palette or right-click menu,
        // so create a fresh Entity/Component for the node
        else
        {
            // Creating a node is actually two operations:  creating an Entity + adding a component(s) to that Entity
            // so we need to batch the operations so that undo/redo will treat it all as one operation
            AzToolsFramework::ScopedUndoBatch undoBatch("Create Node");

            // Create a new Entity to hold the Component for this new node
            AZ::EntityId vegetationEntityId;
            AzToolsFramework::EditorRequestBus::BroadcastResult(vegetationEntityId, &AzToolsFramework::EditorRequests::CreateNewEntity, rootEntityId);

            // Add the Component for this node, as well as any required components
            AddComponentForNode(node, vegetationEntityId);
        }

        m_ignoreGraphUpdates = false;
    }

    void MainWindow::AddComponentForNode(GraphModel::NodePtr node, const AZ::EntityId& entityId)
    {
        auto* baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        if (!baseNodePtr)
        {
            return;
        }

        AZ::TypeId componentToAddTypeId;
        LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(componentToAddTypeId, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::GetComponentTypeId, baseNodePtr->RTTI_GetType());
        if (componentToAddTypeId.IsNull())
        {
            AZ_Assert(false, "Node missing a registered component TypeId.");
            return;
        }

        AZ::ComponentId newComponentId = AddComponentTypeIdToEntity(entityId, componentToAddTypeId, baseNodePtr->GetOptionalRequiredServices());

        // Tie this new node to its representative Entity and Component
        baseNodePtr->SetVegetationEntityId(entityId);
        baseNodePtr->SetComponentId(newComponentId);
    }

    void MainWindow::HandleNodeAdded(GraphModel::NodePtr node)
    {
        auto* baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        if (!baseNodePtr)
        {
            return;
        }

        // Update our EntityId/Node mappings when a new node is added
        GraphCanvas::GraphId graphId = (*GraphModelIntegration::GraphControllerNotificationBus::GetCurrentBusId());
        if (!m_ignoreGraphUpdates)
        {
            UpdateEntityIdNodeMap(graphId, node);
        }

        // For any node with an Entity Name slot, we need to replace the string property display with a read-only version
        // instead until we have support for listening for GraphModel slot value changes.  We need to delay this because
        // when the node is added, the slots haven't been added to the element map yet.
        QTimer::singleShot(0, [node, graphId]() {
            GraphModel::SlotPtr slot = node->GetSlot(LandscapeCanvas::ENTITY_NAME_SLOT_ID);
            if (slot)
            {
                GraphCanvas::NodeId nodeId;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, graphId, &GraphModelIntegration::GraphControllerRequests::GetNodeIdByNode, node);

                GraphCanvas::SlotId slotId;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(slotId, graphId, &GraphModelIntegration::GraphControllerRequests::GetSlotIdBySlot, slot);

                // If this is a wrapped node, then remove the Entity Name property slot since the wrapper node will already have one
                bool isNodeWrapped = false;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(isNodeWrapped, graphId, &GraphModelIntegration::GraphControllerRequests::IsNodeWrapped, node);
                if (isNodeWrapped)
                {
                    GraphCanvas::NodeRequestBus::Event(nodeId, &GraphCanvas::NodeRequests::RemoveSlot, slotId);
                    return;
                }

                // The ownership of the new data interface and property display get passed to the node property display widget
                // when we call SetNodePropertyDisplay
                auto dataInterface = aznew GraphModelIntegration::ReadOnlyDataInterface(slot);
                GraphCanvas::NodePropertyDisplay* readOnlyPropertyDisplay = nullptr;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(readOnlyPropertyDisplay, &GraphCanvas::GraphCanvasRequests::CreateReadOnlyNodePropertyDisplay, static_cast<GraphCanvas::ReadOnlyDataInterface*>(dataInterface));

                readOnlyPropertyDisplay->SetNodeId(nodeId);
                readOnlyPropertyDisplay->SetSlotId(slotId);
                GraphCanvas::NodePropertyRequestBus::Event(slotId, &GraphCanvas::NodePropertyRequests::SetNodePropertyDisplay, readOnlyPropertyDisplay);
            }
        });

        // Listen for component property changes on the Entity corresponding to this node
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(baseNodePtr->GetVegetationEntityId());

        LandscapeCanvas::BaseNode::BaseNodeType nodeType = baseNodePtr->GetBaseNodeType();
        if (nodeType == LandscapeCanvas::BaseNode::Shape)
        {
            // Add thumbnail image of the shape type to the node
            AZ::TypeId componentTypeId;
            LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(componentTypeId, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::GetComponentTypeId, baseNodePtr->RTTI_GetType());
            AZStd::string entityIconPath;
            AzToolsFramework::EditorRequestBus::BroadcastResult(entityIconPath, &AzToolsFramework::EditorRequestBus::Events::GetComponentIconPath, componentTypeId, AZ::Edit::Attributes::ViewportIcon, nullptr);
            if (!entityIconPath.empty())
            {
                QPixmap iconPixmap(entityIconPath.c_str());
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::SetThumbnailImageOnNode, node, iconPixmap);
            }
        }
        else if (nodeType == LandscapeCanvas::BaseNode::Gradient || nodeType == LandscapeCanvas::BaseNode::GradientGenerator || nodeType == LandscapeCanvas::BaseNode::GradientModifier)
        {
            // Add custom gradient preview thumbnail to all gradient type nodes
            // The node layout takes ownership of the thumbnail, so it will be deleted whenever the node is deleted
            const AZ::EntityId& gradientEntityId = baseNodePtr->GetVegetationEntityId();
            GradientPreviewThumbnailItem* previewThumbnail = new GradientPreviewThumbnailItem(gradientEntityId);
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::SetThumbnailOnNode, node, previewThumbnail);
        }
        else if ((nodeType == LandscapeCanvas::BaseNode::VegetationArea || nodeType == LandscapeCanvas::BaseNode::TerrainArea) && (node->GetNodeType() == GraphModel::NodeType::WrapperNode))
        {
            GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::SetWrapperNodeActionString, node, QObject::tr("Add Extenders").toUtf8().constData());
        }
    }

    void MainWindow::UpdateEntityIdNodeMap(GraphCanvas::GraphId graphId, GraphModel::NodePtr node)
    {
        auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        const AZ::EntityId& entityId = baseNodePtr->GetVegetationEntityId();
        auto nodeMap = GetEntityIdNodeMap(graphId, node);
        if (nodeMap)
        {
            nodeMap->insert({ entityId, node });
        }
    }

    MainWindow::EntityIdNodeMap* MainWindow::GetEntityIdNodeMap(GraphCanvas::GraphId graphId, GraphModel::NodePtr node)
    {
        auto nodeMapsIt = m_entityIdNodeMapsByGraph.find(graphId);
        if (nodeMapsIt == m_entityIdNodeMapsByGraph.end())
        {
            return nullptr;
        }

        // Return the corresponding EntityIdNodeMap for this node type
        EntityIdNodeMaps& nodeMaps = nodeMapsIt->second;
        auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        LandscapeCanvas::BaseNode::BaseNodeType baseNodeType = baseNodePtr->GetBaseNodeType();
        auto nodeMapType = EntityIdNodeMapEnum::Invalid;
        switch (baseNodeType)
        {
        case LandscapeCanvas::BaseNode::Shape:
            nodeMapType = EntityIdNodeMapEnum::Shapes;
            break;
        case LandscapeCanvas::BaseNode::TerrainArea:
        case LandscapeCanvas::BaseNode::VegetationArea:
            nodeMapType = EntityIdNodeMapEnum::WrapperNodes;
            break;
        case LandscapeCanvas::BaseNode::Gradient:
        case LandscapeCanvas::BaseNode::GradientGenerator:
        case LandscapeCanvas::BaseNode::GradientModifier:
            nodeMapType = EntityIdNodeMapEnum::Gradients;
            break;
        }

        if (nodeMapType != EntityIdNodeMapEnum::Invalid)
        {
            return &nodeMaps[nodeMapType];
        }

        return nullptr;
    }

    void MainWindow::ParseNodeConnections(GraphCanvas::GraphId graphId, GraphModel::NodePtr node, ConnectionsList& connections)
    {
        auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
        AZ::Component* component = baseNodePtr->GetComponent();
        if (!component)
        {
            return;
        }

        // Find the node mappings for this graph
        auto nodeMapsIt = m_entityIdNodeMapsByGraph.find(graphId);
        if (nodeMapsIt == m_entityIdNodeMapsByGraph.end())
        {
            return;
        }
        const EntityIdNodeMaps& nodeMaps = nodeMapsIt->second;

        // Iterate through the component class elements to find any matching fields corresponding
        // to input slots
        AZ::EntityId previewEntityId, inboundShapeEntityId;
        AzToolsFramework::EntityIdList gradientSamplerIds;
        AzToolsFramework::EntityIdList vegetationAreaIds;
        m_serializeContext->EnumerateObject(component,
            // beginElemCB
            [&previewEntityId, &inboundShapeEntityId, &gradientSamplerIds, &vegetationAreaIds, baseNodePtr](void *instance, [[maybe_unused]] const AZ::SerializeContext::ClassData *classData, const AZ::SerializeContext::ClassElement *classElement) -> bool
        {
            if (classElement && (classElement->m_typeId == azrtti_typeid<AZ::EntityId>()))
            {
                if (strcmp(classElement->m_name, PreviewEntityElementName) == 0)
                {
                    previewEntityId = *reinterpret_cast<AZ::EntityId*>(instance);
                    return false;
                }
                else if ((strcmp(classElement->m_name, GradientIdElementName) == 0)
                    || (strcmp(classElement->m_name, GradientEntityIdElementName) == 0))
                {
                    gradientSamplerIds.push_back(*reinterpret_cast<AZ::EntityId*>(instance));
                    return false;
                }
                else if (strcmp(classElement->m_name, EntityIdListElementName) == 0)
                {
                    if (baseNodePtr->GetBaseNodeType() == LandscapeCanvas::BaseNode::BaseNodeType::VegetationArea)
                    {
                        vegetationAreaIds.push_back(*reinterpret_cast<AZ::EntityId*>(instance));
                    }
                    else
                    {
                        gradientSamplerIds.push_back(*reinterpret_cast<AZ::EntityId*>(instance));
                    }

                    return false;
                }
                else if (strcmp(classElement->m_name, ShapeEntityIdElementName) == 0)
                {
                    inboundShapeEntityId = *reinterpret_cast<AZ::EntityId*>(instance);
                    return false;
                }
                else if (strcmp(classElement->m_name, InputBoundsEntityIdElementName) == 0)
                {
                    inboundShapeEntityId = *reinterpret_cast<AZ::EntityId*>(instance);
                    return false;
                }
            }

            return true;
        },
            // endElemCB
            []() -> bool { return true; },
            AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr/* errorHandler */);

        // Connect any preview entities to the corresponding shape bounds
        AZStd::vector<AZStd::pair<GraphModel::SlotId, AZ::EntityId>> shapeSlotEntityPairs;
        if (previewEntityId.IsValid())
        {
            shapeSlotEntityPairs.push_back(AZStd::make_pair(LandscapeCanvas::PREVIEW_BOUNDS_SLOT_ID, previewEntityId));
        }

        // Connect any inbound shape slots to the corresponding shape bounds
        if (inboundShapeEntityId.IsValid())
        {
            // We have multiple inbound shape slots that share the same underlying property,
            // so we need to figure out which kind of inbound shape slot this node has
            GraphModel::SlotId shapeSlotId(LandscapeCanvas::INBOUND_SHAPE_SLOT_ID);
            if (!node->GetSlot(shapeSlotId))
            {
                shapeSlotId = GraphModel::SlotId(LandscapeCanvas::PIN_TO_SHAPE_SLOT_ID);
                if (!node->GetSlot(shapeSlotId))
                {
                    shapeSlotId = GraphModel::SlotId(LandscapeCanvas::INPUT_BOUNDS_SLOT_ID);
                }
            }

            shapeSlotEntityPairs.push_back(AZStd::make_pair(shapeSlotId, inboundShapeEntityId));
        }

        // Look for a placement bounds on Vegetation Areas, which is a special case since it could be
        // driven by a Reference Shape or actual Shape component that also exists on the same Entity
        // as the Vegetation Area Component that we represent with the node, but in this case the
        // component will actually be shown as a Placement Bounds slot
        AZ::EntityId placementBoundsEntityId;
        if (baseNodePtr->GetBaseNodeType() == LandscapeCanvas::BaseNode::BaseNodeType::VegetationArea)
        {
            GraphModel::SlotPtr placementBoundsSlot = node->GetSlot(LandscapeCanvas::PLACEMENT_BOUNDS_SLOT_ID);
            if (placementBoundsSlot)
            {
                // Retrieve the Placement Bounds slot value from the Reference Shape component if it exists
                auto baseAreaNodePtr = static_cast<LandscapeCanvas::BaseAreaNode*>(node.get());
                AZ::Component* referenceShapeComponent = baseAreaNodePtr->GetReferenceShapeComponent();
                if (referenceShapeComponent)
                {
                    QString propertyPath = GetPropertyPathForSlot(placementBoundsSlot, LandscapeCanvas::LandscapeCanvasDataTypeEnum::Bounds);
                    AzToolsFramework::PropertyTreeEditor pte = AzToolsFramework::PropertyTreeEditor(reinterpret_cast<void*>(referenceShapeComponent), referenceShapeComponent->RTTI_GetType());
                    auto placementBounds = pte.GetProperty(propertyPath.toUtf8().constData());
                    if (placementBounds.IsSuccess())
                    {
                        placementBoundsEntityId = AZStd::any_cast<AZ::EntityId>(placementBounds.GetValue());
                        if (placementBoundsEntityId.IsValid())
                        {
                            shapeSlotEntityPairs.push_back(AZStd::make_pair(LandscapeCanvas::PLACEMENT_BOUNDS_SLOT_ID, placementBoundsEntityId));
                        }
                    }
                }
                // Otherwise, also check if this Entity has its own Shape component as well that will serve
                // as the placement bounds
                else
                {
                    const auto& shapeNodeMap = nodeMaps[EntityIdNodeMapEnum::Shapes];
                    const AZ::EntityId& entityId = baseAreaNodePtr->GetVegetationEntityId();
                    auto shapeIt = shapeNodeMap.find(entityId);
                    if (shapeIt != shapeNodeMap.end())
                    {
                        GraphModel::NodePtr shapeNode = (*shapeIt).second;
                        auto baseShapeNodePtr = static_cast<LandscapeCanvas::BaseNode*>(shapeNode.get());
                        if (baseShapeNodePtr->GetComponent())
                        {
                            shapeSlotEntityPairs.push_back(AZStd::make_pair(LandscapeCanvas::PLACEMENT_BOUNDS_SLOT_ID, entityId));
                        }
                    }
                }
            }
        }

        // Connect any input bounds slots to their corresponding shape bounds
        const auto& shapeNodeMap = nodeMaps[EntityIdNodeMapEnum::Shapes];
        for (auto slotEntityPair : shapeSlotEntityPairs)
        {
            const AZ::EntityId& entityId = slotEntityPair.second;
            auto shapeIt = shapeNodeMap.find(entityId);
            if (shapeIt == shapeNodeMap.end())
            {
                continue;
            }

            GraphModel::NodePtr shapeNode = (*shapeIt).second;
            GraphModel::SlotPtr shapeBoundsSlot = shapeNode->GetSlot(LandscapeCanvas::BaseShapeNode::BOUNDS_SLOT_ID);
            GraphModel::SlotPtr shapeTargetSlot = node->GetSlot(slotEntityPair.first);

            auto source = AZStd::make_pair(shapeNode, shapeBoundsSlot);
            auto target = AZStd::make_pair(node, shapeTargetSlot);
            connections.push_back(AZStd::make_pair(source, target));
        }

        // Handle if this node has an image asset slot to parse
        HandleImageAssetSlot(node, nodeMaps[EntityIdNodeMapEnum::Gradients], connections);

        auto handleIndexedSlots = [this, graphId, node, &connections](
                                      const AzToolsFramework::EntityIdList& entityIds,
                                      const EntityIdNodeMap& sourceNodeMap,
                                      const GraphModel::SlotName& outboundSlotId,
                                      LandscapeCanvas::LandscapeCanvasDataTypeEnum slotDataType)
        {
            if (entityIds.empty())
            {
                return;
            }

            size_t numEntityIds = entityIds.size();
            for (int i = 0; i < numEntityIds; ++i)
            {
                const AZ::EntityId& entityId = entityIds[i];
                if (!entityId.IsValid())
                {
                    continue;
                }

                // Find the source node
                GraphModel::NodePtr sourceNode;
                auto nodeIt = sourceNodeMap.find(entityId);
                if (nodeIt != sourceNodeMap.end())
                {
                    sourceNode = (*nodeIt).second;
                }

                if (sourceNode)
                {
                    // Don't allow a node's output to be connected to itself
                    if (sourceNode == node)
                    {
                        continue;
                    }

                    GraphModel::SlotPtr outboundSlot = sourceNode->GetSlot(outboundSlotId);

                    // Find the corresponding input slot based on the index
                    GraphModel::DataTypePtr dataType = GetGraphContext()->GetDataType(slotDataType);
                    GraphModel::SlotPtr inboundSlot = EnsureInboundDataSlotWithIndex(graphId, node, dataType, i);
                    if (!inboundSlot)
                    {
                        AZ_Assert(false, "Unhandled inbound slot mapping.");
                        continue;
                    }

                    auto source = AZStd::make_pair(sourceNode, outboundSlot);
                    auto target = AZStd::make_pair(node, inboundSlot);
                    connections.push_back(AZStd::make_pair(source, target));
                }
            }
        };

        // Connect any inbound gradient slots to the corresponding Gradient, Gradient Generator, or Gradient Modifier
        handleIndexedSlots(gradientSamplerIds, nodeMaps[EntityIdNodeMapEnum::Gradients], LandscapeCanvas::OUTBOUND_GRADIENT_SLOT_ID, LandscapeCanvas::LandscapeCanvasDataTypeEnum::Gradient);

        // Connect any inbound vegetation area slots to the corresponding vegetation area
        handleIndexedSlots(vegetationAreaIds, nodeMaps[EntityIdNodeMapEnum::WrapperNodes], LandscapeCanvas::OUTBOUND_AREA_SLOT_ID, LandscapeCanvas::LandscapeCanvasDataTypeEnum::Area);
    }

    void MainWindow::HandleImageAssetSlot(GraphModel::NodePtr targetNode, const EntityIdNodeMap& gradientNodeMap, ConnectionsList& connections)
    {
        auto baseNode = static_cast<LandscapeCanvas::BaseNode*>(targetNode.get());
        const AZ::EntityId& entityId = baseNode->GetVegetationEntityId();

        AZStd::string imageSourceAsset;
        GradientSignal::ImageGradientRequestBus::EventResult(
            imageSourceAsset, entityId, &GradientSignal::ImageGradientRequests::GetImageAssetSourcePath);
        AZ::IO::Path imageSourceAssetPath(imageSourceAsset);

        // The imageSourceAssetPath will only be valid if the targetNode is an Image Gradient that has
        // a valid image asset path set.
        if (!imageSourceAssetPath.empty())
        {
            // Look through all the gradient nodes in this graph to find a Gradient Baker that
            // has the same output path as the input image asset to the Image Gradient. There
            // might not be one if the user is generating the image gradients themselves and
            // not from a gradient baker.
            for (auto it = gradientNodeMap.begin(); it != gradientNodeMap.end(); ++it)
            {
                const AZ::EntityId& nodeEntityId = it->first;
                GraphModel::NodePtr sourceNode = it->second;

                // If this node doesn't have an output image slot, it's not a Gradient Baker
                // so keep looking
                GraphModel::SlotPtr outputImageSlot = sourceNode->GetSlot(LandscapeCanvas::OUTPUT_IMAGE_SLOT_ID);
                if (!outputImageSlot)
                {
                    continue;
                }

                AZ::IO::Path outputImagePath;
                GradientSignal::GradientImageCreatorRequestBus::EventResult(
                    outputImagePath, nodeEntityId, &GradientSignal::GradientImageCreatorRequests::GetOutputImagePath);

                if (imageSourceAssetPath == outputImagePath)
                {
                    GraphModel::SlotPtr imageAssetSlot = targetNode->GetSlot(LandscapeCanvas::IMAGE_ASSET_SLOT_ID);
                    auto source = AZStd::make_pair(sourceNode, outputImageSlot);
                    auto target = AZStd::make_pair(targetNode, imageAssetSlot);
                    connections.push_back(AZStd::make_pair(source, target));
                }
            }
        }
    }

    void MainWindow::HandleDeserializedNodes()
    {
        if (m_deserializedNodes.empty())
        {
            return;
        }

        m_ignoreGraphUpdates = true;

        LandscapeCanvas::LandscapeCanvasSerialization serialization;
        LandscapeCanvas::LandscapeCanvasSerializationRequestBus::BroadcastResult(serialization, &LandscapeCanvas::LandscapeCanvasSerializationRequests::GetSerializedMappings);

        GraphCanvas::GraphId graphId = GetActiveGraphCanvasGraphId();

        // The deserialized nodes already have a valid EntityId, so we need
        // to find the corresponding deserialized Entity and fix-up the references
        for (auto node : m_deserializedNodes)
        {
            auto* baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            if (!baseNodePtr)
            {
                continue;
            }

            AZ::EntityId existingEntityId = baseNodePtr->GetVegetationEntityId();
            if (!existingEntityId.IsValid())
            {
                continue;
            }

            auto it = serialization.m_deserializedEntities.find(existingEntityId);
            if (it == serialization.m_deserializedEntities.end())
            {
                continue;
            }

            AZ::EntityId newEntityId = it->second;

            AZ::TypeId componentTypeId;
            LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::BroadcastResult(componentTypeId, &LandscapeCanvas::LandscapeCanvasNodeFactoryRequests::GetComponentTypeId, baseNodePtr->RTTI_GetType());
            if (componentTypeId.IsNull())
            {
                continue;
            }

            AZ::Entity* newEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationRequests::FindEntity, newEntityId);
            AZ_Assert(newEntity, "Unable to find deserialized Entity");

            // Find the component on the Entity that corresponds to this node
            AZ::Component* newComponent = newEntity->FindComponent(componentTypeId);
            if (!newComponent)
            {
                // The FindComponent won't find a component if its disabled, so if it failed
                // then look through the disabled components on this Entity
                AZ::Entity::ComponentArrayType disabledComponents;
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(newEntityId, &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
                for (auto disabledComponent : disabledComponents)
                {
                    if (disabledComponent->RTTI_GetType() == componentTypeId)
                    {
                        newComponent = disabledComponent;
                        break;
                    }
                }

                // Look through the pending components next if we didn't find it in the disabled components,
                // since it may be put in the pending bucket if a dependent component is actually deleted
                // instead of just being disabled
                if (!newComponent)
                {
                    AZ::Entity::ComponentArrayType pendingComponents;
                    AzToolsFramework::EditorPendingCompositionRequestBus::Event(newEntityId, &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
                    for (AZ::Component* pendingComponent : pendingComponents)
                    {
                        if (pendingComponent->RTTI_GetType() == componentTypeId)
                        {
                            newComponent = pendingComponent;
                            break;
                        }
                    }
                }

                // If the component for this node is disabled, then the node needs to be disabled as well
                GraphModelIntegration::GraphControllerRequestBus::Event(graphId, &GraphModelIntegration::GraphControllerRequests::DisableNode, node);
            }

            AZ_Assert(newComponent, "Deserialized Entity missing component matching node");

            // Fix-up the references on the new node to the deserialized Entity/Component
            baseNodePtr->SetVegetationEntityId(newEntityId);
            baseNodePtr->SetComponentId(newComponent->GetId());
        }

        m_deserializedNodes.clear();

        m_ignoreGraphUpdates = false;
    }

    int MainWindow::GetInboundDataSlotIndex(GraphModel::NodePtr node, GraphModel::DataTypePtr dataType, GraphModel::SlotPtr targetSlot)
    {
        if (!node)
        {
            return InvalidSlotIndex;
        }

        // Return the index of the specified targetSlot based on the input data slots that match the specified data type on the given node
        int index = 0;
        for (auto& it : node->GetSlots())
        {
            GraphModel::SlotPtr slot = it.second;

            if (slot->Is(GraphModel::SlotDirection::Input, GraphModel::SlotType::Data))
            {
                // Our Bounds and Gradient data types are both AZ::EntityId under the hood, so there is
                // some magic that takes place where they each support an Invalid data type as well as
                // their specific data type, so instead of comparing the current slot->GetDataType() directly
                // we need to check the possible data types instead for a  match.
                const auto& dataTypes = slot->GetSupportedDataTypes();
                auto iter = AZStd::find(dataTypes.begin(), dataTypes.end(), dataType);
                if (iter != dataTypes.end())
                {
                    if (slot == targetSlot)
                    {
                        return index;
                    }
                    else
                    {
                        ++index;
                    }
                }
            }
        }

        return InvalidSlotIndex;
    }

    GraphModel::SlotPtr MainWindow::EnsureInboundDataSlotWithIndex(GraphCanvas::GraphId graphId, GraphModel::NodePtr node, GraphModel::DataTypePtr dataType, int index)
    {
        // Iterate through all the slots on the node to find an input data slot that matches the specified data type for the specified index
        int currentIndex = 0;
        for (GraphModel::SlotDefinitionPtr slotDefinition : node->GetSlotDefinitions())
        {
            if (slotDefinition->Is(GraphModel::SlotDirection::Input, GraphModel::SlotType::Data))
            {
                const GraphModel::SlotName& slotName = slotDefinition->GetName();
                const auto& dataTypes = slotDefinition->GetSupportedDataTypes();
                auto iter = AZStd::find(dataTypes.begin(), dataTypes.end(), dataType);
                if (iter != dataTypes.end())
                {
                    if (slotDefinition->SupportsExtendability())
                    {
                        // The subId for the extendable slots aren't necessarily an index starting at 0, depending
                        // on if the user removes/re-adds slots, so we first need to check if we need to offset
                        // the index we are expecting based on the starting subId
                        int subIdOffset = 0;
                        auto extendableSlots = node->GetExtendableSlots(slotName);
                        if (!extendableSlots.empty())
                        {
                            GraphModel::SlotPtr firstSlot = *extendableSlots.begin();
                            subIdOffset = firstSlot->GetSlotSubId();
                            index += subIdOffset;
                        }

                        GraphModel::SlotId slotId(slotName, index);

                        // If it's an extendable slot, we need to add enough to be able to accommodate the specified index.
                        for (int i = node->GetExtendableSlotCount(slotName) + subIdOffset; i < index + 1; ++i)
                        {
                            // If we fail to add an extended slot at any point (e.g. reached maximum, node has custom logic overriding, etc..)
                            // then we need to bail out.  We need to add the extended slot using a different API when we are doing an initial
                            // graph vs. if the graph is already loaded because in the former case the node hasn't been fully created yet so
                            // we are just updating the data model, whereas in the latter case the node already exists in the graph and so
                            // we need to use the GraphController API so that the UI gets updated properly.
                            bool success = false;
                            if (node->GetId() == GraphModel::Node::INVALID_NODE_ID)
                            {
                                success = node->AddExtendedSlot(slotName);
                            }
                            else
                            {
                                GraphModelIntegration::GraphControllerRequestBus::EventResult(slotId, graphId, &GraphModelIntegration::GraphControllerRequests::ExtendSlot, node, slotName);
                                success = slotId.IsValid();
                            }
                            
                            if (!success)
                            {
                                return nullptr;
                            }
                        }

                        return node->GetSlot(slotId);
                    }
                    else if (currentIndex == index)
                    {
                        return node->GetSlot(slotName);
                    }

                    ++currentIndex;
                }
            }
        }

        return nullptr;
    }
}

#include <Source/Editor/moc_MainWindow.cpp>
