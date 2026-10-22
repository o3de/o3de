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
#include <AzFramework/Translation/TranslationDef.h>

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
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<GraphViewSettings>(QT_TRANSLATE_NOOP("AtomToolsFramework", "Graph View Config"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Basic Interactions"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_snapDistance, QT_TRANSLATE_NOOP("AtomToolsFramework", "Snap Distance"), QT_TRANSLATE_NOOP("AtomToolsFramework", "The distance at which connections will snap to a nearby slot."))
                        ->Attribute(AZ::Edit::Attributes::Min, 10.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_maxZoom, QT_TRANSLATE_NOOP("AtomToolsFramework", "Max Zoom"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Controls the maximum magnification for zooming in"))
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 5.0)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1)
                        ->Attribute(AZ::Edit::Attributes::Suffix, "X")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_edgePanningPercentage, QT_TRANSLATE_NOOP("AtomToolsFramework", "Edge Panning Percentage"), QT_TRANSLATE_NOOP("AtomToolsFramework", "The percentage of the visible area to start scrolling when the mouse cursor is dragged into"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_edgePanningScrollSpeed, QT_TRANSLATE_NOOP("AtomToolsFramework", "Edge Panning Scroll Speed"), QT_TRANSLATE_NOOP("AtomToolsFramework", "How fast the scene will scroll when scrolling"))
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.0)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Coupling"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dragNodeCouplingEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Drag Node Coupling Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Allow automatic connections to be created between nodes when dragging them on top of each other"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_dragCouplingTime, QT_TRANSLATE_NOOP("AtomToolsFramework", "Drag Coupling Time"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Amount of time nodes must be overlapping before automatic coupling is activated"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Splicing"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dragConnectionSpliceEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Drag Connection Splice Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Allow automatic splicing when dragging nodes on top of existing graph connections"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_dragConnectionSpliceTime, QT_TRANSLATE_NOOP("AtomToolsFramework", "Drag Connection Splice Time"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Amount of time a node must be overlapping an existing connection before automatic splicing occurs"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_dropConnectionSpliceEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Drop Connection Splice Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Allow automatic splicing when dropping nodes onto existing graph connections"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_dropConnectionSpliceTime, QT_TRANSLATE_NOOP("AtomToolsFramework", "Drop Connection Splice Time"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Amount of time a node must be overlapping an existing connection before automatic splicing occurs"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_splicedNodeNudgingEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Spliced Node Nudging Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Allow splicing actions to nudge other nodes out of the way"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_shakeToDespliceEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Shake To Desplice Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Allow shaking a node to break connections from the nodes"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_shakesToDesplice, QT_TRANSLATE_NOOP("AtomToolsFramework", "Shakes To Desplice"), QT_TRANSLATE_NOOP("AtomToolsFramework", "The number of shades required to break connections between nodes"))
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 10)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Nudging"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_nodeNudgingEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Node Nudging Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Controls if nodes will attempt to nudge each other out of the way during different operations"))
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Shaking"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_minimumShakePercent, QT_TRANSLATE_NOOP("AtomToolsFramework", "Minimum Shake Percent"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Controls how long each motion must be in order to be registered as a shake"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_shakeDeadZonePercent, QT_TRANSLATE_NOOP("AtomToolsFramework", "Shake Dead Zone Percent"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Controls how far the cursor must move before a check is initiated"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_shakeStraightnessPercent, QT_TRANSLATE_NOOP("AtomToolsFramework", "Shake Straightness Percent"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Controls how aligned the individual motions must be in order to qualify as a shake"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_maximumShakeDuration, QT_TRANSLATE_NOOP("AtomToolsFramework", "Maximum Shake Duration"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Sets a cap on how long it consider a series of actions as a single shake gesture"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Aligning"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GraphViewSettings::m_alignmentTime, QT_TRANSLATE_NOOP("AtomToolsFramework", "Alignment Time"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Controls the amount of time nodes will take to slide into place when performing alignment commands"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, "ms")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 5)
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Connections"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphViewSettings::m_connectionCurveType, QT_TRANSLATE_NOOP("AtomToolsFramework", "Connection Curve Type"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Style used for drawing lines between non-data connections"))
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Straight, QT_TRANSLATE_NOOP("AtomToolsFramework", "Straight"))
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Curved, QT_TRANSLATE_NOOP("AtomToolsFramework", "Curved"))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphViewSettings::m_dataConnectionCurveType, QT_TRANSLATE_NOOP("AtomToolsFramework", "Data Connection Curve Type"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Style used for drawing lines between data connections"))
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Straight, QT_TRANSLATE_NOOP("AtomToolsFramework", "Straight"))
                        ->EnumAttribute(GraphCanvas::Styling::ConnectionCurveType::Curved, QT_TRANSLATE_NOOP("AtomToolsFramework", "Curved"))
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomToolsFramework", "Misc"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_groupDoubleClickCollapseEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Group Double Click Collapse Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Toggle if groups can be expanded or collapsed by double clicking"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_bookmarkViewportControlEnabled, QT_TRANSLATE_NOOP("AtomToolsFramework", "Bookmark Viewport Control Enabled"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Will cause the bookmarks to force the viewport into the state determined by the bookmark type\nBookmark Anchors - The viewport that exists when the bookmark is created.\nNode Groups - The area the Node Group covers"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphViewSettings::m_allowDataReferenceSlots, QT_TRANSLATE_NOOP("AtomToolsFramework", "Allow Data Reference Slots"), "")
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
