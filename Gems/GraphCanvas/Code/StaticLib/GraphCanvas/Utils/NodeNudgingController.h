/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QPoint>
#include <QRect>

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    // Rearranges nodes in order to create spaces for the root elements within the specified graph.
    class NodeNudgingController
        : public GeometryNotificationBus::MultiHandler
        , public AZ::SystemTickBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR(NodeNudgingController, AZ::SystemAllocator);
    
        NodeNudgingController();
        NodeNudgingController(const GraphId& graphId, const AZStd::unordered_set< NodeId >& rootElements);
        ~NodeNudgingController() = default;

        void SetGraphId(const GraphId& graphId);
        
        void StartNudging(const AZStd::unordered_set< NodeId >& fixedElements);
        void FinalizeNudging();
        void CancelNudging(bool animate = true);
    
        // SystemTickBus
        void OnSystemTick() override;
        ////
        
        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& targetEntity, const AZ::Vector2& position) override;
        void OnBoundsChanged() override;
        ////
    
    private:
    
        void UpdatePositioning();
        void SanitizeDirection(AZ::Vector2& movementVector, const QPointF& directionSanitization) const;
        
        void DirtyPositioning();

        void UpdateBoundingBox(QRectF& moveableBoundingBox, const QRectF& staticBoundingBox, QPointF& transitionDirection, const QPointF& incitingMovement, float halfWidth, float halfHeight, const AZ::Vector2& gridStep);
        
        GraphId m_graphId;
        AZStd::unordered_set< NodeId > m_rootElements;
        
        // Keeping track of a store of mainpulated bounding boxes
        AZStd::unordered_map< NodeId, QRectF > m_originalBoundingBoxes;

        AZStd::unordered_map< NodeId, QRectF > m_cachedNodeElements;
    };
}
