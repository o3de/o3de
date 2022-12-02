/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphViewSettings.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AtomToolsFramework
{
    void GraphViewSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GraphViewSettings>()
                ->Version(0)
                ->Field("snapDistance", &GraphViewSettings::m_snapDistance)
                ->Field("alignmentTime", &GraphViewSettings::m_alignmentTime)
                ->Field("maxZoom", &GraphViewSettings::m_maxZoom)
                ->Field("edgePanningPercentage", &GraphViewSettings::m_edgePanningPercentage)
                ->Field("edgePanningScrollSpeed", &GraphViewSettings::m_edgePanningScrollSpeed)
                ->Field("dragNodeCouplingEnabled", &GraphViewSettings::m_dragNodeCouplingEnabled)
                ->Field("dragCouplingTime", &GraphViewSettings::m_dragCouplingTime)
                ->Field("dragConnectionSpliceEnabled", &GraphViewSettings::m_dragConnectionSpliceEnabled)
                ->Field("dragConnectionSpliceTime", &GraphViewSettings::m_dragConnectionSpliceTime)
                ->Field("dropConnectionSpliceEnabled", &GraphViewSettings::m_dropConnectionSpliceEnabled)
                ->Field("dropConnectionSpliceTime", &GraphViewSettings::m_dropConnectionSpliceTime)
                ->Field("shakeToDespliceEnabled", &GraphViewSettings::m_shakeToDespliceEnabled)
                ->Field("shakesToDesplice", &GraphViewSettings::m_shakesToDesplice)
                ->Field("minimumShakePercent", &GraphViewSettings::m_minimumShakePercent)
                ->Field("shakeDeadZonePercent", &GraphViewSettings::m_shakeDeadZonePercent)
                ->Field("shakeStraightnessPercent", &GraphViewSettings::m_shakeStraightnessPercent)
                ->Field("maximumShakeDuration", &GraphViewSettings::m_maximumShakeDuration)
                ->Field("splicedNodeNudgingEnabled", &GraphViewSettings::m_splicedNodeNudgingEnabled)
                ->Field("nodeNudgingEnabled", &GraphViewSettings::m_nodeNudgingEnabled)
                ->Field("connectionCurveType", &GraphViewSettings::m_connectionCurveType)
                ->Field("dataConnectionCurveType", &GraphViewSettings::m_dataConnectionCurveType)
                ->Field("groupDoubleClickCollapseEnabled", &GraphViewSettings::m_groupDoubleClickCollapseEnabled)
                ->Field("bookmarkViewportControlEnabled", &GraphViewSettings::m_bookmarkViewportControlEnabled)
                ->Field("allowNodeDisabling", &GraphViewSettings::m_allowNodeDisabling)
                ->Field("allowDataReferenceSlots", &GraphViewSettings::m_allowDataReferenceSlots)
                ->Field("constructPresets", &GraphViewSettings::m_constructPresets)
                ;

            serialize->RegisterGenericType<GraphViewSettingsPtr>();

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<GraphViewSettingsPtr>("GraphViewSettingsPtr", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<GraphViewSettings>("Graph View Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Basic Interactions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_snapDistance, "Snap Distance", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_maxZoom, "Max Zoom", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_edgePanningPercentage, "Edge Panning Percentage", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_edgePanningScrollSpeed, "Edge Panning Scroll Speed", "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Coupling")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dragNodeCouplingEnabled, "Drag Node Coupling Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dragCouplingTime, "Drag Coupling Time", "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Splicing")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dragConnectionSpliceEnabled, "Drag Connection Splice Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dragConnectionSpliceTime, "Drag Connection Splice Time", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dropConnectionSpliceEnabled, "Drop Connection Splice Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dropConnectionSpliceTime, "Drop Connection Splice Time", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_splicedNodeNudgingEnabled, "Spliced Node Nudging Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_shakeToDespliceEnabled, "Shake To Desplice Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_shakesToDesplice, "Shakes To Desplice", "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Nudging")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_nodeNudgingEnabled, "Node Nudging Enabled", "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Shaking")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_minimumShakePercent, "Minimum Shake Percent", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_shakeDeadZonePercent, "Shake Dead Zone Percent", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_shakeStraightnessPercent, "Shake Straightness Percent", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_maximumShakeDuration, "Maximum Shake Duration", "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Aligning")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_alignmentTime, "Alignment Time", "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Connections")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphViewSettings::m_connectionCurveType, "Connection Curve Type", "")
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Straight, "Straight")
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Curved, "Curved")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphViewSettings::m_dataConnectionCurveType, "Data Connection Curve Type", "")
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Straight, "Straight")
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Curved, "Curved")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Misc")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_groupDoubleClickCollapseEnabled, "Group Double Click Collapse Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_bookmarkViewportControlEnabled, "Bookmark Viewport Control Enabled", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_allowNodeDisabling, "Allow Node Disabling", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_allowDataReferenceSlots, "Allow Data Reference Slots", "")
                    ;
            }
        }
    }

    GraphViewSettings::~GraphViewSettings()
    {
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusDisconnect();
    }

    void GraphViewSettings::Initialize(const AZ::Crc32& toolId, const AZStd::map<AZStd::string, AZ::Color>& defaultGroupPresets)
    {
        m_toolId = toolId;
        m_constructPresets.SetDefaultGroupPresets(defaultGroupPresets);
        m_constructPresets.SetEditorId(m_toolId);
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusConnect(m_toolId);
    }

    double GraphViewSettings::GetSnapDistance() const
    {
        return m_snapDistance;
    }

    bool GraphViewSettings::IsGroupDoubleClickCollapseEnabled() const
    {
        return m_groupDoubleClickCollapseEnabled;
    }

    bool GraphViewSettings::IsBookmarkViewportControlEnabled() const
    {
        return m_bookmarkViewportControlEnabled;
    }

    bool GraphViewSettings::IsDragNodeCouplingEnabled() const
    {
        return m_dragNodeCouplingEnabled;
    }

    AZStd::chrono::milliseconds GraphViewSettings::GetDragCouplingTime() const
    {
        return AZStd::chrono::milliseconds(m_dragCouplingTime);
    }

    bool GraphViewSettings::IsDragConnectionSpliceEnabled() const
    {
        return m_dragConnectionSpliceEnabled;
    }

    AZStd::chrono::milliseconds GraphViewSettings::GetDragConnectionSpliceTime() const
    {
        return AZStd::chrono::milliseconds(m_dropConnectionSpliceTime);
    }

    bool GraphViewSettings::IsDropConnectionSpliceEnabled() const
    {
        return m_dropConnectionSpliceEnabled;
    }

    AZStd::chrono::milliseconds GraphViewSettings::GetDropConnectionSpliceTime() const
    {
        return AZStd::chrono::milliseconds(m_dropConnectionSpliceTime);
    }

    bool GraphViewSettings::IsSplicedNodeNudgingEnabled() const
    {
        return m_splicedNodeNudgingEnabled;
    }

    bool GraphViewSettings::IsNodeNudgingEnabled() const
    {
        return m_nodeNudgingEnabled;
    }

    bool GraphViewSettings::IsShakeToDespliceEnabled() const
    {
        return m_shakeToDespliceEnabled;
    }

    int GraphViewSettings::GetShakesToDesplice() const
    {
        return m_shakesToDesplice;
    }

    float GraphViewSettings::GetMinimumShakePercent() const
    {
        return m_minimumShakePercent;
    }

    float GraphViewSettings::GetShakeDeadZonePercent() const
    {
        return m_shakeDeadZonePercent;
    }

    float GraphViewSettings::GetShakeStraightnessPercent() const
    {
        return m_shakeStraightnessPercent;
    }

    AZStd::chrono::milliseconds GraphViewSettings::GetMaximumShakeDuration() const
    {
        return AZStd::chrono::milliseconds(m_maximumShakeDuration);
    }

    AZStd::chrono::milliseconds GraphViewSettings::GetAlignmentTime() const
    {
        return AZStd::chrono::milliseconds(m_alignmentTime);
    }

    float GraphViewSettings::GetMaxZoom() const
    {
        return m_maxZoom;
    }

    float GraphViewSettings::GetEdgePanningPercentage() const
    {
        return m_edgePanningPercentage;
    }

    float GraphViewSettings::GetEdgePanningScrollSpeed() const
    {
        return m_edgePanningScrollSpeed;
    }

    GraphCanvas::EditorConstructPresets* GraphViewSettings::GetConstructPresets() const
    {
        return &m_constructPresets;
    }

    const GraphCanvas::ConstructTypePresetBucket* GraphViewSettings::GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const
    {
        return m_constructPresets.FindPresetBucket(constructType);
    }

    GraphCanvas::Styling::ConnectionCurveType GraphViewSettings::GetConnectionCurveType() const
    {
        return m_connectionCurveType;
    }

    GraphCanvas::Styling::ConnectionCurveType GraphViewSettings::GetDataConnectionCurveType() const
    {
        return m_dataConnectionCurveType;
    }

    bool GraphViewSettings::AllowNodeDisabling() const
    {
        return m_allowNodeDisabling;
    }

    bool GraphViewSettings::AllowDataReferenceSlots() const
    {
        return m_allowDataReferenceSlots;
    }
} // namespace AtomToolsFramework
