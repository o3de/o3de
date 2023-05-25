/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "QtHelpers.h"

namespace EntityHelpers
{
    AZ::Vector2 RoundXY(const AZ::Vector2& v)
    {
        return AZ::Vector2(roundf(v.GetX()), roundf(v.GetY()));
    }

    AZ::Vector3 RoundXY(const AZ::Vector3& v)
    {
        return AZ::Vector3(roundf(v.GetX()), roundf(v.GetY()), v.GetZ());
    }

    AZ::Vector3 MakeVec3(const AZ::Vector2& v)
    {
        return AZ::Vector3(v.GetX(), v.GetY(), 0.0f);
    }

    float Snap(float value, float snapDistance)
    {
        // std::remainder gives the difference between the value and the closest multiple of the snap distance
        float rem = std::remainder(value, snapDistance);

        // return the closest value that is on the snap grid
        return value - rem;
    }

    AZ::Vector2 Snap(const AZ::Vector2& v, float snapDistance)
    {
        return AZ::Vector2(v.GetX() - std::remainder(v.GetX(), snapDistance),
            v.GetY() - std::remainder(v.GetY(), snapDistance));
    }

    UiTransform2dInterface::Offsets Snap(const UiTransform2dInterface::Offsets& offs, const ViewportHelpers::ElementEdges& grabbedEdges, float snapDistance)
    {
        return UiTransform2dInterface::Offsets(offs.m_left - std::remainder((grabbedEdges.m_left ? offs.m_left : 0.0f), snapDistance),
            offs.m_top - std::remainder((grabbedEdges.m_top ? offs.m_top : 0.0f), snapDistance),
            offs.m_right - std::remainder((grabbedEdges.m_right ? offs.m_right : 0.0f), snapDistance),
            offs.m_bottom - std::remainder((grabbedEdges.m_bottom ? offs.m_bottom : 0.0f), snapDistance));
    }

    void MoveElementToGlobalPosition(AZ::Entity* element, const QPoint& globalPos)
    {
        if (!element)
        {
             return;
        }

        // Transform pivot position to canvas space
        AZ::Vector2 pivotPos;
        UiTransformBus::EventResult(pivotPos, element->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

        // Transform destination position to canvas space
        AZ::Matrix4x4 transformFromViewport;
        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
        AZ::Vector2 globalPos2(QtHelpers::QPointFToVector2(globalPos));
        AZ::Vector3 destPos3 = transformFromViewport * AZ::Vector3(globalPos2.GetX(), globalPos2.GetY(), 0.0f);
        AZ::Vector2 destPos(destPos3.GetX(), destPos3.GetY());

        // Adjust offsets
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);
        UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, offsets + (destPos - pivotPos));
    }

    AZ::Entity* GetParentElement(const AZ::Entity* element)
    {
        AZ::Entity* parentElement = nullptr;

        if (!element)
        {
            return nullptr;
        }

        UiElementBus::EventResult(parentElement, element->GetId(), &UiElementBus::Events::GetParent);

        return parentElement;
    }

    AZ::Entity* GetParentElement(const AZ::EntityId& elementId)
    {
        AZ::Entity* parentElement = nullptr;
        UiElementBus::EventResult(parentElement, elementId, &UiElementBus::Events::GetParent);
        return parentElement;
    }

    AZ::Entity* GetEntity(AZ::EntityId id)
    {
        AZ::Entity* element = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(element, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        return element;
    }

    void ComputeCanvasSpaceRectNoScaleRotate(AZ::EntityId elementId, UiTransform2dInterface::Offsets offsets, UiTransformInterface::Rect& rect)
    {
        AZ::Entity* parentElement = nullptr;
        UiElementBus::EventResult(parentElement, elementId, &UiElementBus::Events::GetParent);
        if (parentElement)
        {
            UiTransformInterface::Rect parentRect;
            UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

            AZ::Vector2 parentSize = parentRect.GetSize();

            UiTransform2dInterface::Anchors anchors;
            UiTransform2dBus::EventResult(anchors, elementId, &UiTransform2dBus::Events::GetAnchors);

            float left   = parentRect.left + parentSize.GetX() * anchors.m_left   + offsets.m_left;
            float right  = parentRect.left + parentSize.GetX() * anchors.m_right  + offsets.m_right;
            float top    = parentRect.top + parentSize.GetY() * anchors.m_top    + offsets.m_top;
            float bottom = parentRect.top + parentSize.GetY() * anchors.m_bottom + offsets.m_bottom;

            rect.Set(left, right, top, bottom);
        }
        else
        {
            AZ_Assert(false, "This is the root element.");
        }

        // we never return a "flipped" rect. I.e. left is always less than right, top is always less than bottom
        // if it is flipped in a dimension then we make it zero size in that dimension
        if (rect.left > rect.right)
        {
            rect.left = rect.right = rect.GetCenterX();
        }
        if (rect.top > rect.bottom)
        {
            rect.top = rect.bottom = rect.GetCenterY();
        }
    }

    AZ::Vector2 ComputeCanvasSpacePivotNoScaleRotate(AZ::EntityId elementId, UiTransform2dInterface::Offsets offsets)
    {
        AZ::Vector2 pivot;
        UiTransformBus::EventResult(pivot, elementId, &UiTransformBus::Events::GetPivot);

        UiTransformInterface::Rect rect;
        ComputeCanvasSpaceRectNoScaleRotate(elementId, offsets, rect);

        AZ::Vector2 size = rect.GetSize();

        float x =  rect.left + size.GetX() * pivot.GetX();
        float y =  rect.top + size.GetY() * pivot.GetY();

        return AZ::Vector2(x, y);
    }

    AZStd::string GetHierarchicalElementName(AZ::EntityId entityId)
    {
        AZStd::string result;

        // attempt to get more info about the entity
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        if (entity)
        {
            result = entity->GetName();

            AZ::Entity* parent = nullptr;
            UiElementBus::EventResult(parent, entityId, &UiElementBus::Events::GetParent);
            while (parent)
            {
                AZStd::string entityName = parent->GetName();
                AZ::EntityId parentId = parent->GetId();
                parent = nullptr;   // in case entity is not listening on bus
                UiElementBus::EventResult(parent, parentId, &UiElementBus::Events::GetParent);

                // we do not want to include the root element name
                if (parent)
                {
                    result = entityName + "/" + result;
                }
            }
        }
        else
        {
            result = entityId.ToString();
        }

        return result;
    }

    AZ::Entity* GetCommonAncestor(AZ::Entity* element1, AZ::Entity* element2,
        AZ::Entity*& element1NextAncestor, AZ::Entity*& element2NextAncestor)
    {
        element1NextAncestor = element1;
        element2NextAncestor = element2;

        // if the two elements are the same then their common ancestor is the element itself
        if (element1 == element2)
        {
            return element1;
        }

        // traverse up element1's parent chain storing all the ancestors in a vector
        AZStd::vector<AZ::Entity*> element1Ancestors;
        AZ::Entity* parent = GetParentElement(element1);
        while (parent)
        {
            if (parent == element2)
            {
                return element2;    // element2 is an ancestor of element1, early out
            }

            element1Ancestors.push_back(parent);
            element1NextAncestor = parent;

            parent = GetParentElement(parent);
        }

        // now traverse up element2's parent chain looking for a match in element1's ancestors
        parent = GetParentElement(element2);
        while (parent)
        {
            if (parent == element1)
            {
                return element1; // element1 is a parent of element 2, early out
            }

            // search for this parent in element1's ancestors
            for (int i = 0; i < element1Ancestors.size(); ++i)
            {
                if (element1Ancestors[i] == parent)
                {
                    // this parent is in element1's ancestors so it is the common ancestor

                    if (i > 0)
                    {
                        // the child of the common ancestor is the previous ancestor in the list
                        element1NextAncestor = element1Ancestors[i-1];
                    }
                    else
                    {
                        // element1 is an immediate child of the common parent
                        element1NextAncestor = element1;
                    }

                    // we have found the common parent
                    return parent;
                }
            }

            element2NextAncestor = parent;
            parent = GetParentElement(parent);
        }

        // no common parent was found
        // this should never happen if the given elements are part of the same canvas
        return nullptr;
    }

    bool CompareOrderInElementHierarchy(AZ::Entity* element1, AZ::Entity* element2)
    {
        if (element1 == element2)
        {
            // this should not be used to compare the same element but if it is always return a consistent
            // result
            return true;
        }

        AZ::Entity* element1NextAncestor = nullptr;
        AZ::Entity* element2NextAncestor = nullptr;
        AZ::Entity* commonParent = GetCommonAncestor(element1, element2, element1NextAncestor, element2NextAncestor);

        if (!commonParent)
        {
            // an error orccured and no common parent was found
            // to recover just compare the pointers
            AZ_Assert(false, "No common parent found.");
            return (element1 < element2);
        }

        if (element1 == commonParent)
        {
            return true; // element2 is a child of element1 so element1 is before
        }
        else if (element2 == commonParent)
        {
            return false; // element1 is a child of element2 so element1 is not before
        }
        else
        {
            // neither is the parent of the other. In this case we know that element1NextAncestor and
            // element2NextAncestor are siblings and children of the common parent
            int index1 = -1;
            UiElementBus::EventResult(index1, commonParent->GetId(), &UiElementBus::Events::GetIndexOfChild, element1NextAncestor);
            int index2 = -1;
            UiElementBus::EventResult(index2, commonParent->GetId(), &UiElementBus::Events::GetIndexOfChild, element2NextAncestor);

            AZ_Assert(index1 != -1 && index2 != -1, "Immediate ancestors not found in parent.");

            return (index1 < index2);
        }

    }

    void MoveByLocalDeltaUsingOffsets(AZ::EntityId entityId, AZ::Vector2 deltaInLocalSpace)
    {
        // Get the existing offsets and pass them to the version of this function that takes starting offsets
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, entityId, &UiTransform2dBus::Events::GetOffsets);
        MoveByLocalDeltaUsingOffsets(entityId, offsets, deltaInLocalSpace);
    }

    void MoveByLocalDeltaUsingOffsets(AZ::EntityId entityId, const UiTransform2dInterface::Offsets& startingOffsets, AZ::Vector2 deltaInLocalSpace)
    {
        // simply add the local space delta to the offsets
        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetOffsets, startingOffsets + deltaInLocalSpace);
    }

    AZ::Vector2 MoveByLocalDeltaUsingAnchors(AZ::EntityId entityId, AZ::EntityId parentEntityId, AZ::Vector2 deltaInLocalSpace, bool restrictDirection)
    {
        // Get the existing anchors and pass them to the version of this function that takes starting anchors
        UiTransform2dInterface::Anchors anchors;
        UiTransform2dBus::EventResult(anchors, entityId, &UiTransform2dBus::Events::GetAnchors);
        return MoveByLocalDeltaUsingAnchors(entityId, parentEntityId, anchors, deltaInLocalSpace, restrictDirection);
    }

    AZ::Vector2 MoveByLocalDeltaUsingAnchors(AZ::EntityId entityId, AZ::EntityId parentEntityId,
        const UiTransform2dInterface::Anchors& startingAnchors, AZ::Vector2 deltaInLocalSpace, bool restrictDirection)
    {
        UiTransform2dInterface::Anchors anchors = startingAnchors;

        AZ::Vector2 parentSize;
        UiTransformBus::EventResult(parentSize, parentEntityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

        // compute the anchorDelta in anchor space (0-1) and add to the anchor values
        AZ::Vector2 anchorDelta(0.0f, 0.0f);
        const float epsilon = 0.001f;
        if (parentSize.GetX() > epsilon)
        {
            anchorDelta.SetX(deltaInLocalSpace.GetX() / parentSize.GetX());
            anchors.m_left += anchorDelta.GetX();
            anchors.m_right += anchorDelta.GetX();
        }
        if (parentSize.GetY() > epsilon)
        {
            anchorDelta.SetY(deltaInLocalSpace.GetY() / parentSize.GetY());
            anchors.m_top += anchorDelta.GetY();
            anchors.m_bottom += anchorDelta.GetY();
        }

        // Check if the anchors are now out of the 0-1 range and if so move them back along the delta vector
        // Note that we can't just clamp (both because it doesn't work if the anchors are apart and because we
        // do not want to change the angle of movement).
        if (anchors.m_left < 0.0f || anchors.m_right > 1.0f || anchors.m_top < 0.0f || anchors.m_bottom > 1.0f)
        {
            // compute the adjustment needed to get the anchors in range
            AZ::Vector2 adjustment(0.0f, 0.0f);
            if (anchors.m_left < 0.0f)
            {
                adjustment.SetX(0.0f - anchors.m_left);
            }
            else if (anchors.m_right > 1.0f)
            {
                adjustment.SetX(1.0f - anchors.m_right);
            }

            if (anchors.m_top < 0.0f)
            {
                adjustment.SetY(0.0f - anchors.m_top);
            }
            else if (anchors.m_bottom > 1.0f)
            {
                adjustment.SetY(1.0f - anchors.m_bottom);
            }

            // If we are moving only in one axis then there are no issues with sliding along the edge since we must be moving directly
            // against one anchor limit (edge) only, if we are moving in more than one axis then we have to make sure we adjust back only
            // along the direction of movement (if restrictDirection is true)
            if (anchorDelta.GetX() != 0.0f && anchorDelta.GetY() != 0.0f && restrictDirection)
            {
                // The adjustment vector as it is would put the anchor on the limit but not respect only moving it along the
                // deltaInLocalSpace (and therefore anchorDelta) vector.
                // So we modify the adjustment vector to be co-linear (but in the opposite direction) to anchorDelta.
                // Because of rounding errors where the anchorDelta vector is very close to the x or y axis we check whether
                // the x or y component is greater and switch the order of the calculation.
                float xAdjustAbs = fabsf(adjustment.GetX());
                float yAdjustAbs = fabsf(adjustment.GetY());
                if (xAdjustAbs < yAdjustAbs)
                {
                    float xAdjustmentToFitY = adjustment.GetY() * anchorDelta.GetX() / anchorDelta.GetY();
                    if (fabsf(xAdjustmentToFitY) >= xAdjustAbs)
                    {
                        adjustment.SetX(xAdjustmentToFitY);
                    }
                    else
                    {
                        float yAdjustmentToFitX = adjustment.GetX() * anchorDelta.GetY() / anchorDelta.GetX();
                        adjustment.SetY(yAdjustmentToFitX);
                    }
                }
                else
                {
                    float yAdjustmentToFitX = adjustment.GetX() * anchorDelta.GetY() / anchorDelta.GetX();
                    if (fabsf(yAdjustmentToFitX) >= yAdjustAbs)
                    {
                        adjustment.SetY(yAdjustmentToFitX);
                    }
                    else
                    {
                        float xAdjustmentToFitY = adjustment.GetY() * anchorDelta.GetX() / anchorDelta.GetY();
                        adjustment.SetX(xAdjustmentToFitY);
                    }
                }
            }

            // apply the adjustment to the anchors so that they stay in bounds
            anchors.m_left += adjustment.GetX();
            anchors.m_right += adjustment.GetX();
            anchors.m_top += adjustment.GetY();
            anchors.m_bottom += adjustment.GetY();

            // do an extra clamp just in case of rounding errors to ensure the anchor is never even a tiny amount out of range
            anchors.UnitClamp();

            // we will return the adjusted localTranslation which is the amount we actually moved in local space
            deltaInLocalSpace.SetX(deltaInLocalSpace.GetX() + adjustment.GetX() * parentSize.GetX());
            deltaInLocalSpace.SetY(deltaInLocalSpace.GetY() + adjustment.GetY() * parentSize.GetY());
        }

        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

        return deltaInLocalSpace;
    }

    AZ::Vector2 TransformDeltaFromCanvasToLocalSpace(AZ::EntityId entityId, AZ::Vector2 deltaInCanvasSpace)
    {
        AZ::Vector3 deltaInCanvasSpace3(deltaInCanvasSpace.GetX(), deltaInCanvasSpace.GetY(), 0.0f);
        AZ::Matrix4x4 transform;
        UiTransformBus::Event(entityId, &UiTransformBus::Events::GetTransformFromCanvasSpace, transform);

        AZ::Vector3 deltaInLocalSpace3 = transform.Multiply3x3(deltaInCanvasSpace3);
        AZ::Vector2 deltaInLocalSpace = AZ::Vector2(deltaInLocalSpace3.GetX(), deltaInLocalSpace3.GetY());

        return deltaInLocalSpace;
    }

    AZ::Vector2 TransformDeltaFromLocalToCanvasSpace(AZ::EntityId entityId, AZ::Vector2 deltaInLocalSpace)
    {
        AZ::Vector3 deltaInLocalSpace3(deltaInLocalSpace.GetX(), deltaInLocalSpace.GetY(), 0.0f);
        AZ::Matrix4x4 transform;
        UiTransformBus::Event(entityId, &UiTransformBus::Events::GetTransformToCanvasSpace, transform);

        AZ::Vector3 deltaInCanvasSpace3 = transform.Multiply3x3(deltaInLocalSpace3);
        AZ::Vector2 deltaInCanvasSpace = AZ::Vector2(deltaInCanvasSpace3.GetX(), deltaInCanvasSpace3.GetY());

        return deltaInCanvasSpace;
    }

    AZ::Vector2 TransformDeltaFromViewportToCanvasSpace(AZ::EntityId canvasEntityId, AZ::Vector2 deltaInViewportSpace)
    {
        AZ::Vector3 deltaInViewportSpace3(deltaInViewportSpace.GetX(), deltaInViewportSpace.GetY(), 0.0f);
        AZ::Matrix4x4 transform;
        UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::GetViewportToCanvasMatrix, transform);

        AZ::Vector3 deltaInCanvasSpace3 = transform.Multiply3x3(deltaInViewportSpace3);
        AZ::Vector2 deltaInLocalSpace = AZ::Vector2(deltaInCanvasSpace3.GetX(), deltaInCanvasSpace3.GetY());

        return deltaInLocalSpace;
    }

}   // namespace EntityHelpers
