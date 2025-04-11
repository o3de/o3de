/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Graph/GraphViewConstructPresets.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#endif

namespace AtomToolsFramework
{
    // Settings for initializing graph canvas and node palettes
    struct GraphViewSettings : private GraphCanvas::AssetEditorSettingsRequestBus::Handler
    {
        AZ_RTTI(GraphViewSettings, "{00E392C7-C372-4E09-9C07-5803B8864B85}");
        AZ_CLASS_ALLOCATOR(GraphViewSettings, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(GraphViewSettings);

        static void Reflect(AZ::ReflectContext* context);

        GraphViewSettings() = default;
        ~GraphViewSettings();

        void Initialize(const AZ::Crc32& toolId, const AZStd::map<AZStd::string, AZ::Color>& defaultGroupPresets);

        // GraphCanvas::AssetEditorSettingsRequestBus::Handler overrides...
        double GetSnapDistance() const override;

        bool IsGroupDoubleClickCollapseEnabled() const override;
        bool IsBookmarkViewportControlEnabled() const override;

        bool IsDragNodeCouplingEnabled() const override;
        AZStd::chrono::milliseconds GetDragCouplingTime() const override;
        bool IsDragConnectionSpliceEnabled() const override;
        AZStd::chrono::milliseconds GetDragConnectionSpliceTime() const override;
        bool IsDropConnectionSpliceEnabled() const override;
        AZStd::chrono::milliseconds GetDropConnectionSpliceTime() const override;

        bool IsSplicedNodeNudgingEnabled() const;
        bool IsNodeNudgingEnabled() const override;

        bool IsShakeToDespliceEnabled() const override;
        int GetShakesToDesplice() const override;
        float GetMinimumShakePercent() const override;
        float GetShakeDeadZonePercent() const override;
        float GetShakeStraightnessPercent() const override;

        AZStd::chrono::milliseconds GetMaximumShakeDuration() const override;
        AZStd::chrono::milliseconds GetAlignmentTime() const override;
        float GetMaxZoom() const override;

        float GetEdgePanningPercentage() const override;
        float GetEdgePanningScrollSpeed() const override;

        GraphCanvas::EditorConstructPresets* GetConstructPresets() const override;
        const GraphCanvas::ConstructTypePresetBucket* GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const override;

        GraphCanvas::Styling::ConnectionCurveType GetConnectionCurveType() const override;
        GraphCanvas::Styling::ConnectionCurveType GetDataConnectionCurveType() const override;

        bool AllowNodeDisabling() const override;
        bool AllowDataReferenceSlots() const override;

        AZ::Crc32 m_toolId = {};
        // Full path to the graph canvas style manager settings
        AZStd::string m_styleManagerPath;
        // Full path to translation settings
        AZStd::string m_translationPath;
        // Mime type identifying compatibility between nodes dragged from the node palette to the current graph view
        AZStd::string m_nodeMimeType;
        // String identifier used to save settings for graph canvas context menus
        AZStd::string m_nodeSaveIdentifier;
        // Callback function used to create node palette items
        AZStd::function<GraphCanvas::GraphCanvasTreeItem*(const AZ::Crc32&)> m_createNodeTreeItemsFn;

        // Settings related to Basic movement and selection
        double m_snapDistance = 20.0;
        int m_alignmentTime = 200;
        float m_maxZoom = 2.0f;
        float m_edgePanningPercentage = 0.1f;
        float m_edgePanningScrollSpeed = 100.0f;

        // Settings related to coupling and decoupling connections between nodes
        bool m_dragNodeCouplingEnabled = true;
        int m_dragCouplingTime = 500;

        // Settings related to splicing nodes along existing connections
        bool m_dragConnectionSpliceEnabled = true;
        int m_dragConnectionSpliceTime = 500;
        bool m_dropConnectionSpliceEnabled = true;
        int m_dropConnectionSpliceTime = 500;
        bool m_shakeToDespliceEnabled = true;
        int m_shakesToDesplice = 3;
        float m_minimumShakePercent = 40.0f;
        float m_shakeDeadZonePercent = 20.0f;
        float m_shakeStraightnessPercent = 0.75f;
        int m_maximumShakeDuration = 1000;

        // Settings related to nudging or Moving nodes around in relation to each other
        bool m_splicedNodeNudgingEnabled = true;
        bool m_nodeNudgingEnabled = true;

        // Settings related to how lines are rendered between connections
        GraphCanvas::Styling::ConnectionCurveType m_connectionCurveType = GraphCanvas::Styling::ConnectionCurveType::Straight;
        GraphCanvas::Styling::ConnectionCurveType m_dataConnectionCurveType = GraphCanvas::Styling::ConnectionCurveType::Straight;

        // Other miscellaneous settings
        bool m_groupDoubleClickCollapseEnabled = true;
        bool m_bookmarkViewportControlEnabled = false;
        bool m_allowNodeDisabling = false;
        bool m_allowDataReferenceSlots = false;

        mutable GraphViewConstructPresets m_constructPresets;
    };

    using GraphViewSettingsPtr = AZStd::shared_ptr<GraphViewSettings>;

} // namespace AtomToolsFramework
