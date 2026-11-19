/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxAuthoringBus.h"
#include "EditorWhiteBoxComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    void ReflectAuthoring(AZ::ReflectContext* context)
    {
        // SerializeContext registration is what lets Python lists convert to and from these containers.
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AuthoringResult>();
            serializeContext->Class<AuthoringShapeParams>();
            serializeContext->Class<AuthoringLayerSettings>();
            serializeContext->Class<AuthoringPolygonInfo>();
            serializeContext->Class<AuthoringEdgeInfo>();
            serializeContext->Class<AuthoringDisplaySettings>();
            serializeContext->Class<AuthoringBooleanSettings>();
            serializeContext->Class<AuthoringDrawSettings>();

            serializeContext->RegisterGenericType<AZStd::vector<int>>();
            serializeContext->RegisterGenericType<AZStd::vector<float>>();
            serializeContext->RegisterGenericType<AZStd::vector<AZ::u32>>();
            serializeContext->RegisterGenericType<AZStd::vector<AZ::Vector3>>();
            serializeContext->RegisterGenericType<AZStd::vector<AuthoringPolygonInfo>>();
            serializeContext->RegisterGenericType<AZStd::vector<AuthoringEdgeInfo>>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AuthoringResult>("WhiteBoxResult")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("Success", BehaviorValueProperty(&AuthoringResult::m_success))
                ->Property("Message", BehaviorValueProperty(&AuthoringResult::m_message))
                ->Property("Ids", BehaviorValueProperty(&AuthoringResult::m_ids));

            behaviorContext->Class<AuthoringShapeParams>("WhiteBoxShapeParams")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("Shape", BehaviorValueProperty(&AuthoringShapeParams::m_shape))
                ->Property("Width", BehaviorValueProperty(&AuthoringShapeParams::m_width))
                ->Property("Depth", BehaviorValueProperty(&AuthoringShapeParams::m_depth))
                ->Property("Height", BehaviorValueProperty(&AuthoringShapeParams::m_height))
                ->Property("Sides", BehaviorValueProperty(&AuthoringShapeParams::m_sides))
                ->Property("Steps", BehaviorValueProperty(&AuthoringShapeParams::m_steps))
                ->Property("StepsByHeight", BehaviorValueProperty(&AuthoringShapeParams::m_stepsByHeight))
                ->Property("StepHeight", BehaviorValueProperty(&AuthoringShapeParams::m_stepHeight))
                ->Property("WallThickness", BehaviorValueProperty(&AuthoringShapeParams::m_wallThickness))
                ->Property("CavityGap", BehaviorValueProperty(&AuthoringShapeParams::m_cavityGap))
                ->Property("Floor", BehaviorValueProperty(&AuthoringShapeParams::m_floor))
                ->Property("Ceiling", BehaviorValueProperty(&AuthoringShapeParams::m_ceiling))
                ->Property("DoorFrame", BehaviorValueProperty(&AuthoringShapeParams::m_doorFrame))
                ->Property("ArchHeight", BehaviorValueProperty(&AuthoringShapeParams::m_archHeight))
                ->Property("InnerRadius", BehaviorValueProperty(&AuthoringShapeParams::m_innerRadius))
                ->Property("SweepAngle", BehaviorValueProperty(&AuthoringShapeParams::m_sweepAngle))
                ->Property("HoleRatio", BehaviorValueProperty(&AuthoringShapeParams::m_holeRatio))
                ->Property("TubeSides", BehaviorValueProperty(&AuthoringShapeParams::m_tubeSides));

            behaviorContext->Class<AuthoringLayerSettings>("WhiteBoxLayerSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("Name", BehaviorValueProperty(&AuthoringLayerSettings::m_name))
                ->Property("Visible", BehaviorValueProperty(&AuthoringLayerSettings::m_visible))
                ->Property("Collision", BehaviorValueProperty(&AuthoringLayerSettings::m_collision))
                ->Property("Tint", BehaviorValueProperty(&AuthoringLayerSettings::m_tint))
                ->Property("CombineMode", BehaviorValueProperty(&AuthoringLayerSettings::m_combineMode))
                ->Property("InvertNormals", BehaviorValueProperty(&AuthoringLayerSettings::m_invertNormals))
                ->Property("EdgesOnly", BehaviorValueProperty(&AuthoringLayerSettings::m_edgesOnly))
                ->Property("Position", BehaviorValueProperty(&AuthoringLayerSettings::m_position))
                ->Property("Rotation", BehaviorValueProperty(&AuthoringLayerSettings::m_rotation))
                ->Property("Scale", BehaviorValueProperty(&AuthoringLayerSettings::m_scale))
                ->Property("MirrorX", BehaviorValueProperty(&AuthoringLayerSettings::m_mirrorX))
                ->Property("MirrorY", BehaviorValueProperty(&AuthoringLayerSettings::m_mirrorY))
                ->Property("MirrorZ", BehaviorValueProperty(&AuthoringLayerSettings::m_mirrorZ))
                ->Property("ArrayCount", BehaviorValueProperty(&AuthoringLayerSettings::m_arrayCount))
                ->Property("ArrayOffset", BehaviorValueProperty(&AuthoringLayerSettings::m_arrayOffset));

            behaviorContext->Class<AuthoringPolygonInfo>("WhiteBoxPolygonInfo")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("Id", BehaviorValueProperty(&AuthoringPolygonInfo::m_id))
                ->Property("FaceCount", BehaviorValueProperty(&AuthoringPolygonInfo::m_faceCount))
                ->Property("Normal", BehaviorValueProperty(&AuthoringPolygonInfo::m_normal))
                ->Property("Center", BehaviorValueProperty(&AuthoringPolygonInfo::m_center))
                ->Property("Area", BehaviorValueProperty(&AuthoringPolygonInfo::m_area))
                ->Property("VertexIds", BehaviorValueProperty(&AuthoringPolygonInfo::m_vertexIds))
                ->Property("VertexPositions", BehaviorValueProperty(&AuthoringPolygonInfo::m_vertexPositions));

            behaviorContext->Class<AuthoringEdgeInfo>("WhiteBoxEdgeInfo")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("Id", BehaviorValueProperty(&AuthoringEdgeInfo::m_id))
                ->Property("VertexA", BehaviorValueProperty(&AuthoringEdgeInfo::m_vertexA))
                ->Property("VertexB", BehaviorValueProperty(&AuthoringEdgeInfo::m_vertexB))
                ->Property("Start", BehaviorValueProperty(&AuthoringEdgeInfo::m_start))
                ->Property("End", BehaviorValueProperty(&AuthoringEdgeInfo::m_end))
                ->Property("Length", BehaviorValueProperty(&AuthoringEdgeInfo::m_length))
                ->Property("Boundary", BehaviorValueProperty(&AuthoringEdgeInfo::m_boundary))
                ->Property("Interior", BehaviorValueProperty(&AuthoringEdgeInfo::m_interior));

            behaviorContext->Class<AuthoringDisplaySettings>("WhiteBoxDisplaySettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("EdgesOnly", BehaviorValueProperty(&AuthoringDisplaySettings::m_edgesOnly))
                ->Property("UseGlobalTint", BehaviorValueProperty(&AuthoringDisplaySettings::m_useGlobalTint))
                ->Property("Tint", BehaviorValueProperty(&AuthoringDisplaySettings::m_tint))
                ->Property("UseTexture", BehaviorValueProperty(&AuthoringDisplaySettings::m_useTexture))
                ->Property("MaterialPath", BehaviorValueProperty(&AuthoringDisplaySettings::m_materialPath))
                ->Property("CsgSolver", BehaviorValueProperty(&AuthoringDisplaySettings::m_csgSolver))
                ->Property("FlipYZForExport", BehaviorValueProperty(&AuthoringDisplaySettings::m_flipYZForExport));

            behaviorContext->Class<AuthoringBooleanSettings>("WhiteBoxBooleanSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("SourceEntity", BehaviorValueProperty(&AuthoringBooleanSettings::m_sourceEntity))
                ->Property("Operation", BehaviorValueProperty(&AuthoringBooleanSettings::m_operation))
                ->Property("Live", BehaviorValueProperty(&AuthoringBooleanSettings::m_live))
                ->Property("AffectActiveOnly", BehaviorValueProperty(&AuthoringBooleanSettings::m_affectActiveOnly))
                ->Property("SourceAfterApply", BehaviorValueProperty(&AuthoringBooleanSettings::m_sourceAfterApply))
                ->Property("ExcludeFromBoolean", BehaviorValueProperty(&AuthoringBooleanSettings::m_excludeFromBoolean))
                ->Property("BooleanOthers", BehaviorValueProperty(&AuthoringBooleanSettings::m_booleanOthers))
                ->Property("CutterOperation", BehaviorValueProperty(&AuthoringBooleanSettings::m_cutterOperation));

            behaviorContext->Class<AuthoringDrawSettings>("WhiteBoxDrawSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Property("Shape", BehaviorValueProperty(&AuthoringDrawSettings::m_shape))
                ->Property("Sides", BehaviorValueProperty(&AuthoringDrawSettings::m_sides))
                ->Property("HoleRatio", BehaviorValueProperty(&AuthoringDrawSettings::m_holeRatio))
                ->Property("TubeSides", BehaviorValueProperty(&AuthoringDrawSettings::m_tubeSides))
                ->Property("Carve", BehaviorValueProperty(&AuthoringDrawSettings::m_carve))
                ->Property("MergeUnion", BehaviorValueProperty(&AuthoringDrawSettings::m_mergeUnion))
                ->Property("PolygonExtrude", BehaviorValueProperty(&AuthoringDrawSettings::m_polygonExtrude))
                ->Property("UnitCube", BehaviorValueProperty(&AuthoringDrawSettings::m_unitCube))
                ->Property("UnitCubeSize", BehaviorValueProperty(&AuthoringDrawSettings::m_unitCubeSize))
                ->Property("UnitCubeShowGrid", BehaviorValueProperty(&AuthoringDrawSettings::m_unitCubeShowGrid))
                ->Property("StairSteps", BehaviorValueProperty(&AuthoringDrawSettings::m_stairSteps))
                ->Property("StairByHeight", BehaviorValueProperty(&AuthoringDrawSettings::m_stairByHeight))
                ->Property("StairStepHeight", BehaviorValueProperty(&AuthoringDrawSettings::m_stairStepHeight))
                ->Property("StairRotation", BehaviorValueProperty(&AuthoringDrawSettings::m_stairRotation));

            behaviorContext->EBus<EditorWhiteBoxAuthoringRequestBus>("EditorWhiteBoxAuthoringRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.authoring")
                ->Event("Describe", &EditorWhiteBoxAuthoringRequestBus::Events::Describe)
                ->Event("GetLayerCount", &EditorWhiteBoxAuthoringRequestBus::Events::GetLayerCount)
                ->Event("GetActiveLayer", &EditorWhiteBoxAuthoringRequestBus::Events::GetActiveLayer)
                ->Event("SetActiveLayer", &EditorWhiteBoxAuthoringRequestBus::Events::SetActiveLayer)
                ->Event("AddLayer", &EditorWhiteBoxAuthoringRequestBus::Events::AddLayer)
                ->Event("AddShapeLayer", &EditorWhiteBoxAuthoringRequestBus::Events::AddShapeLayer)
                ->Event("DeleteActiveLayer", &EditorWhiteBoxAuthoringRequestBus::Events::DeleteActiveLayer)
                ->Event("DuplicateActiveLayer", &EditorWhiteBoxAuthoringRequestBus::Events::DuplicateActiveLayer)
                ->Event("MoveLayer", &EditorWhiteBoxAuthoringRequestBus::Events::MoveLayer)
                ->Event("GetShapeParams", &EditorWhiteBoxAuthoringRequestBus::Events::GetShapeParams)
                ->Event("SetShapeParams", &EditorWhiteBoxAuthoringRequestBus::Events::SetShapeParams)
                ->Event("BakeShapeLayer", &EditorWhiteBoxAuthoringRequestBus::Events::BakeShapeLayer)
                ->Event("GetLayerSettings", &EditorWhiteBoxAuthoringRequestBus::Events::GetLayerSettings)
                ->Event("SetLayerSettings", &EditorWhiteBoxAuthoringRequestBus::Events::SetLayerSettings)
                ->Event("ApplyLayerTransform", &EditorWhiteBoxAuthoringRequestBus::Events::ApplyLayerTransform)
                ->Event("ApplyLayerModifiers", &EditorWhiteBoxAuthoringRequestBus::Events::ApplyLayerModifiers)
                ->Event("GetVertexCount", &EditorWhiteBoxAuthoringRequestBus::Events::GetVertexCount)
                ->Event("GetFaceCount", &EditorWhiteBoxAuthoringRequestBus::Events::GetFaceCount)
                ->Event("GetVertexIds", &EditorWhiteBoxAuthoringRequestBus::Events::GetVertexIds)
                ->Event("GetVertexPositions", &EditorWhiteBoxAuthoringRequestBus::Events::GetVertexPositions)
                ->Event("GetFaceIds", &EditorWhiteBoxAuthoringRequestBus::Events::GetFaceIds)
                ->Event("GetFaceVertexIds", &EditorWhiteBoxAuthoringRequestBus::Events::GetFaceVertexIds)
                ->Event("GetPolygons", &EditorWhiteBoxAuthoringRequestBus::Events::GetPolygons)
                ->Event("GetEdges", &EditorWhiteBoxAuthoringRequestBus::Events::GetEdges)
                ->Event("SetMeshFromTriangles", &EditorWhiteBoxAuthoringRequestBus::Events::SetMeshFromTriangles)
                ->Event("AddPolygon", &EditorWhiteBoxAuthoringRequestBus::Events::AddPolygon)
                ->Event("ClearMesh", &EditorWhiteBoxAuthoringRequestBus::Events::ClearMesh)
                ->Event("SetVertexPositions", &EditorWhiteBoxAuthoringRequestBus::Events::SetVertexPositions)
                ->Event("TranslateVertices", &EditorWhiteBoxAuthoringRequestBus::Events::TranslateVertices)
                ->Event("ExtrudePolygons", &EditorWhiteBoxAuthoringRequestBus::Events::ExtrudePolygons)
                ->Event("InsetPolygons", &EditorWhiteBoxAuthoringRequestBus::Events::InsetPolygons)
                ->Event("TranslatePolygons", &EditorWhiteBoxAuthoringRequestBus::Events::TranslatePolygons)
                ->Event("BevelEdges", &EditorWhiteBoxAuthoringRequestBus::Events::BevelEdges)
                ->Event("Bridge", &EditorWhiteBoxAuthoringRequestBus::Events::Bridge)
                ->Event("FillHole", &EditorWhiteBoxAuthoringRequestBus::Events::FillHole)
                ->Event("DeletePolygons", &EditorWhiteBoxAuthoringRequestBus::Events::DeletePolygons)
                ->Event("MergePolygons", &EditorWhiteBoxAuthoringRequestBus::Events::MergePolygons)
                ->Event("WeldVertices", &EditorWhiteBoxAuthoringRequestBus::Events::WeldVertices)
                ->Event("InsertEdgeLoops", &EditorWhiteBoxAuthoringRequestBus::Events::InsertEdgeLoops)
                ->Event("SubdividePolygons", &EditorWhiteBoxAuthoringRequestBus::Events::SubdividePolygons)
                ->Event("ConnectVertices", &EditorWhiteBoxAuthoringRequestBus::Events::ConnectVertices)
                ->Event("InsertVertexOnEdge", &EditorWhiteBoxAuthoringRequestBus::Events::InsertVertexOnEdge)
                ->Event("FlipEdge", &EditorWhiteBoxAuthoringRequestBus::Events::FlipEdge)
                ->Event("DetachPolygonsToLayer", &EditorWhiteBoxAuthoringRequestBus::Events::DetachPolygonsToLayer)
                ->Event("RepairMesh", &EditorWhiteBoxAuthoringRequestBus::Events::RepairMesh)
                ->Event("SetPolygonMaterial", &EditorWhiteBoxAuthoringRequestBus::Events::SetPolygonMaterial)
                ->Event("SetPolygonPaint", &EditorWhiteBoxAuthoringRequestBus::Events::SetPolygonPaint)
                ->Event("SetPolygonSmoothing", &EditorWhiteBoxAuthoringRequestBus::Events::SetPolygonSmoothing)
                ->Event("AutoSmooth", &EditorWhiteBoxAuthoringRequestBus::Events::AutoSmooth)
                ->Event("SetPolygonUvProjection", &EditorWhiteBoxAuthoringRequestBus::Events::SetPolygonUvProjection)
                ->Event("FitPolygonUvs", &EditorWhiteBoxAuthoringRequestBus::Events::FitPolygonUvs)
                ->Event("RecalculateUvs", &EditorWhiteBoxAuthoringRequestBus::Events::RecalculateUvs)
                ->Event("GetDisplaySettings", &EditorWhiteBoxAuthoringRequestBus::Events::GetDisplaySettings)
                ->Event("SetDisplaySettings", &EditorWhiteBoxAuthoringRequestBus::Events::SetDisplaySettings)
                ->Event("GetBooleanSettings", &EditorWhiteBoxAuthoringRequestBus::Events::GetBooleanSettings)
                ->Event("SetBooleanSettings", &EditorWhiteBoxAuthoringRequestBus::Events::SetBooleanSettings)
                ->Event("ApplyBoolean", &EditorWhiteBoxAuthoringRequestBus::Events::ApplyBoolean)
                ->Event("RefreshGlobalBooleans", &EditorWhiteBoxAuthoringRequestBus::Events::RefreshGlobalBooleans)
                ->Event("GetDrawSettings", &EditorWhiteBoxAuthoringRequestBus::Events::GetDrawSettings)
                ->Event("SetDrawSettings", &EditorWhiteBoxAuthoringRequestBus::Events::SetDrawSettings)
                ->Event("SetVoxelCells", &EditorWhiteBoxAuthoringRequestBus::Events::SetVoxelCells)
                ->Event("ClearCubeStamp", &EditorWhiteBoxAuthoringRequestBus::Events::ClearCubeStamp)
                ->Event("AddCollision", &EditorWhiteBoxAuthoringRequestBus::Events::AddCollision)
                ->Event("ExportObj", &EditorWhiteBoxAuthoringRequestBus::Events::ExportObj);
        }
    }
} // namespace WhiteBox
