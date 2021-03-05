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
#include <AzCore/std/chrono/chrono.h>

#include <GraphCanvas/Utils/NodeNudgingController.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    //////////////////////////
    // NodeNudgingController
    //////////////////////////
    
    AZStd::chrono::milliseconds k_transitionTime(350);
    
    NodeNudgingController::NodeNudgingController()
    {
    }
    
    NodeNudgingController::NodeNudgingController(const GraphId& graphId, const AZStd::unordered_set< NodeId >& rootElements)
    {
        SetGraphId(graphId);
        StartNudging(rootElements);
    }

    void NodeNudgingController::SetGraphId(const GraphId& graphId)
    {
        m_graphId = graphId;
    }
    
    void NodeNudgingController::StartNudging(const AZStd::unordered_set< NodeId >& rootElements)
    {
        if (!m_rootElements.empty())
        {
            const bool animate = false;
            CancelNudging(animate);
        }

        m_rootElements.clear();

        for (const NodeId& nodeId : rootElements)
        {
            NodeId outermostNodeId = GraphUtils::FindOutermostNode(nodeId);
            m_rootElements.emplace(outermostNodeId);
        }

        UpdatePositioning();
        
        for (const NodeId& nodeId : m_rootElements)
        {
            GeometryNotificationBus::MultiHandler::BusConnect(nodeId);
        }
    }

    void NodeNudgingController::FinalizeNudging()
    {
        m_originalBoundingBoxes.clear();
        m_rootElements.clear();
    }
    
    void NodeNudgingController::CancelNudging(bool animate)
    {
        for (const auto& mapPair : m_originalBoundingBoxes)
        {
            if (animate)
            {
                RootGraphicsItemRequestBus::Event(mapPair.first, &RootGraphicsItemRequests::AnimatePositionTo, mapPair.second.topLeft(), k_transitionTime);
            }
            else
            {
                RootGraphicsItemRequestBus::Event(mapPair.first, &RootGraphicsItemRequests::CancelAnimation);

                AZ::Vector2 position(aznumeric_cast<float>(mapPair.second.topLeft().x()), aznumeric_cast<float>(mapPair.second.topLeft().y()));
                GeometryRequestBus::Event(mapPair.first, &GeometryRequests::SetPosition, position);
            }
        }
        
        m_rootElements.clear();
        m_originalBoundingBoxes.clear();
    }
    
    void NodeNudgingController::OnSystemTick()
    {
        UpdatePositioning();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }
    
    void NodeNudgingController::OnPositionChanged(const AZ::EntityId& /*targetEntity*/, const AZ::Vector2& /*position*/)
    {
        DirtyPositioning();
    }

    void NodeNudgingController::OnBoundsChanged()
    {
        const AZ::EntityId* entityId = GeometryNotificationBus::GetCurrentBusId();

        if (entityId)
        {
            auto cachedElementIter = m_cachedNodeElements.find((*entityId));

            if (cachedElementIter != m_cachedNodeElements.end())
            {
                QGraphicsItem* rootGraphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(rootGraphicsItem, (*entityId), &SceneMemberUIRequests::GetRootGraphicsItem);

                cachedElementIter->second = rootGraphicsItem->sceneBoundingRect();
            }

            DirtyPositioning();
        }
    }
    
    void NodeNudgingController::UpdatePositioning()
    {
        ScopedGraphUndoBlocker undoBlocker(m_graphId);
            
        AZStd::unordered_map< NodeId, QRectF > finalBoundingBoxes;
            
        AZStd::unordered_set< NodeId > searchableEntities = m_rootElements;
            
        // Map of all the elements that caused them to move
        AZStd::unordered_multimap< NodeId, NodeId > incitingElements;
            
        AZStd::unordered_map< NodeId, QPointF > movementDirections;
            
        AZ::Vector2 gridStep;

        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, m_graphId, &SceneRequests::GetGrid);

        GridRequestBus::EventResult(gridStep, gridId, &GridRequests::GetMinorPitch);
        
        while (!searchableEntities.empty())
        {
            NodeId currentNodeId = (*searchableEntities.begin());
            searchableEntities.erase(searchableEntities.begin());

            QRectF currentBoundingBox;
            
            auto mapIter = m_originalBoundingBoxes.find(currentNodeId);
                
            // Cache the original bounding boxes so we can math out where we expect to be from our current
            // visual location to make it look cool(slide out of place and slide back into place in real time)
            if (mapIter != m_originalBoundingBoxes.end())
            {
                currentBoundingBox = mapIter->second;
            }
            else
            {
                auto cachedIter = m_cachedNodeElements.find(currentNodeId);

                if (cachedIter != m_cachedNodeElements.end())
                {
                    m_originalBoundingBoxes[currentNodeId] = cachedIter->second;
                    currentBoundingBox = cachedIter->second;
                }
                else
                {
                    QGraphicsItem* rootGraphicsItem = nullptr;

                    SceneMemberUIRequestBus::EventResult(rootGraphicsItem, currentNodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

                    if (rootGraphicsItem)
                    {
                        currentBoundingBox = rootGraphicsItem->sceneBoundingRect();

                        // If we aren't one of the root elements, store the original position, so we can do fanciness with it.
                        if (m_rootElements.count(currentNodeId) == 0)
                        {
                            m_originalBoundingBoxes[currentNodeId] = currentBoundingBox;
                        }
                    }
                }
            }                
                
            // Helper Elements
            QPointF originalBoundingCenter = currentBoundingBox.center();
            AZ::Vector2 originalCenterPoint(aznumeric_cast<float>(originalBoundingCenter.x()), aznumeric_cast<float>(originalBoundingCenter.y()));

            float halfWidth(aznumeric_cast<float>(currentBoundingBox.width() * 0.5));
            float halfHeight(aznumeric_cast<float>(currentBoundingBox.height() * 0.5));

            auto incitingRange = incitingElements.equal_range(currentNodeId);
                
            // Handle the actual updating of the current element to a new position
            // Using the inciting elements as the direction we want to move them
            //
            // Future Improvments: Keep track of all the previous inciting elements so things move uniformly away from elements
            //       and not in isolated sections where overlapping could happen.
            //
            // Future Improvments: Make this play nicely with grouped nodes(uncollapsed groups don't really work as I'd want which is
            //       moving the group as a whole instead of as sub elements)
            //
            // Future Improvments: Attempt to do something with connection lines to make them go in the right direction(i.e. splicing a group of nodes onto
            //       a small connection between two nodes will not create sufficient space currently.
            if (incitingRange.first != incitingElements.end())
            {                    
                // Potential for conflicts here. Might just let it happen, since trying to fix it is super scary.
                // And for most reasonably sane cases this should work just fine.
                QPointF transitionDirection;
                    
                // Iterate over the list of inciting elements, and move ourselves based on that.
                // Soft update the current bounding box so we know roughly where we expect to be.
                //
                // Want to make some soft restrictions here where we don't want to move backwards.
                // So once we have a transition direction, we want to apply all our fixes in that direction.
                //
                // We also want to keep track of the inciting elements directions and move in the direciton they tell us                    
                // to avoid the same situations of moving into a space created by a node moving out of a space.
                for (auto iter = incitingRange.first; iter != incitingRange.second; ++iter)
                {
                    QRectF incitingRect = finalBoundingBoxes[iter->second];                        
                    QPointF incitingMovement = movementDirections[iter->second];
                        
                    UpdateBoundingBox(currentBoundingBox, incitingRect, transitionDirection, incitingMovement, halfHeight, halfWidth, gridStep);
                }

                for (auto boundingBoxPair : finalBoundingBoxes)
                {
                    QRectF movedSource = boundingBoxPair.second;

                    if (movedSource.intersects(currentBoundingBox))
                    {
                        QPointF incitingMovement = movementDirections[boundingBoxPair.first];

                        UpdateBoundingBox(currentBoundingBox, movedSource, transitionDirection, incitingMovement, halfHeight, halfWidth, gridStep);
                    }
                }
                    
                movementDirections[currentNodeId] = transitionDirection;                
                 
                // Note: Won't play nicely with the offset anchoring just yet.
                //       Would need to calculate the actual desired anchor point, and pass that in, and let the internal element handle offsetting it.
                RootGraphicsItemRequestBus::Event(currentNodeId, &RootGraphicsItemRequests::AnimatePositionTo, currentBoundingBox.topLeft(), k_transitionTime);
            }
            else
            {
                // Root level elements don't care which direction you go.
                movementDirections[currentNodeId] = QPointF(0,0);
            }
                
            incitingElements.erase(currentNodeId);
                
            // Store our final position so others can reference it in determinig 
            // total movement offsets on elements that have moved.
            finalBoundingBoxes[currentNodeId] = currentBoundingBox;
                
            // Find all of the entities we may intersect with
            // in out new position so we can update them
            AZStd::vector< AZ::EntityId > sceneEntities;
            SceneRequestBus::EventResult(sceneEntities, m_graphId, &SceneRequests::GetEntitiesInRect, currentBoundingBox, Qt::ItemSelectionMode::IntersectsItemBoundingRect);
                
            for (const AZ::EntityId& sceneMemberId : sceneEntities)
            {
                if (GraphUtils::IsNode(sceneMemberId)
                    && m_rootElements.count(sceneMemberId) == 0
                    && finalBoundingBoxes.count(sceneMemberId) == 0)
                {                    
                    bool isWrapped = false;
                    NodeRequestBus::EventResult(isWrapped, sceneMemberId, &NodeRequests::IsWrapped);

                    if (isWrapped)
                    {
                        continue;
                    }
                        
                    incitingElements.insert(AZStd::make_pair(sceneMemberId, currentNodeId));
                    searchableEntities.insert(sceneMemberId);
                }
            }
                
            // Root elements cannot be in this list.
            // So we can skip over the sanitization step for them
            for (auto mapPair : m_originalBoundingBoxes)
            {
                if (finalBoundingBoxes.find(mapPair.first) == finalBoundingBoxes.end())
                {
                    if (mapPair.second.intersects(currentBoundingBox))
                    {
                        incitingElements.insert(AZStd::make_pair(mapPair.first, currentNodeId));
                        searchableEntities.insert(mapPair.first);
                    }
                }
            }
        }
        
        // Clean up the unused original positions if we are no longer moving them around.
        auto mapIter = m_originalBoundingBoxes.begin();
        
        while (mapIter != m_originalBoundingBoxes.end())
        {
            if (finalBoundingBoxes.count(mapIter->first) == 0)
            {
                RootGraphicsItemRequestBus::Event(mapIter->first, &RootGraphicsItemRequests::AnimatePositionTo, mapIter->second.topLeft(), k_transitionTime);
                m_cachedNodeElements[mapIter->first] = mapIter->second;

                mapIter = m_originalBoundingBoxes.erase(mapIter);
            }
            else
            {
                ++mapIter;
            }
        }
    }
    
    void NodeNudgingController::SanitizeDirection(AZ::Vector2& movementVector, const QPointF& sanitizer) const
    {
        if ((movementVector.GetX() < 0 && sanitizer.x() > 0)
            || (movementVector.GetX() > 0 && sanitizer.x() < 0))
        {
            movementVector.SetX(-movementVector.GetX());
        }
        else if ((movementVector.GetY() < 0 && sanitizer.y() > 0)
            || (movementVector.GetY() > 0 && sanitizer.y() < 0))
        {
            movementVector.SetY(-movementVector.GetY());
        }
    }
    
    void NodeNudgingController::DirtyPositioning()
    {
        if (!AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void NodeNudgingController::UpdateBoundingBox(QRectF& moveableBoundingBox, const QRectF& staticBoundingBox, QPointF& transitionDirection, const QPointF& incitingMovement, float halfHeight, float halfWidth, const AZ::Vector2& gridStep)
    {
        AZ::Vector2 incitingCenter = AZ::Vector2(aznumeric_cast<float>(staticBoundingBox.center().x()), aznumeric_cast<float>(staticBoundingBox.center().y()));

        // Calculate our current center, since we are moving around.
        AZ::Vector2 centerPoint(aznumeric_cast<float>(moveableBoundingBox.center().x()), aznumeric_cast<float>(moveableBoundingBox.center().y()));

        // Determine the direction of movement
        AZ::Vector2 movementDirection = centerPoint - incitingCenter;

        // Invert this so we can align centers to make the final movement vector easier to compute
        AZ::Vector2 movementVector = -movementDirection;

        if (!movementDirection.IsZero())
        {
            movementDirection.Normalize();
        }
        else
        {
            movementDirection = AZ::Vector2(aznumeric_cast<float>(incitingMovement.x()), aznumeric_cast<float>(incitingMovement.y()));
        }

        SanitizeDirection(movementDirection, incitingMovement);
        SanitizeDirection(movementDirection, transitionDirection);

        AZ::Vector2 movementAmount;

        // Restrict our movement to a single direction, then move so that the two bounding boxes
        // won't overlap in that direction anymore, with a single grid step of spacing, because we're fancy
        // like that.
        if (abs(movementDirection.GetX()) > abs(movementDirection.GetY()))
        {
            float horMove(aznumeric_cast<float>(halfWidth + staticBoundingBox.width() * 0.5f + gridStep.GetX()));

            transitionDirection.setX(1);

            if (movementDirection.GetX() < 0)
            {
                transitionDirection.setX(-1);
                horMove *= -1;
            }

            movementAmount.SetX(movementVector.GetX() + horMove);
            movementAmount.SetY(0);
        }
        else
        {
            float verMove(aznumeric_cast<float>(halfHeight + staticBoundingBox.height() * 0.5f + gridStep.GetY()));
            transitionDirection.setY(1);

            if (movementDirection.GetY() < 0)
            {
                transitionDirection.setY(-1);
                verMove *= -1;
            }

            movementAmount.SetX(0);
            movementAmount.SetY(movementVector.GetY() + verMove);
        }

        QPointF newTopLeft = moveableBoundingBox.topLeft();

        newTopLeft.setX(newTopLeft.x() + movementAmount.GetX());
        newTopLeft.setY(newTopLeft.y() + movementAmount.GetY());

        moveableBoundingBox.moveTopLeft(newTopLeft);
    }
}
