/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiTransform2dComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutBus.h>

#include "UiSerialize.h"
#include "UiElementComponent.h"
#include "UiCanvasComponent.h"

#include <set>
#include <list>

namespace
{
    bool AxisAlignedBoxesIntersect(const AZ::Vector2& minA, const AZ::Vector2& maxA, const AZ::Vector2& minB, const AZ::Vector2& maxB)
    {
        bool boxesIntersect = true;

        if (maxA.GetX() < minB.GetX() || // a is left of b
            minA.GetX() > maxB.GetX() || // a is right of b
            maxA.GetY() < minB.GetY() || // a is above b
            minA.GetY() > maxB.GetY())   // a is below b
        {
            boxesIntersect = false;   // no overlap
        }

        return boxesIntersect;
    }

    void GetInverseTransform(const AZ::Vector2& pivot, const AZ::Vector2& scale, float rotation, AZ::Matrix4x4& mat)
    {
        AZ::Vector3 pivot3(pivot.GetX(), pivot.GetY(), 0);

        float rotRad = DEG2RAD(-rotation);            // inverse rotation

        // Avoid a divide by zero. We could compare with 0.0f here and that would avoid a divide
        // by zero. However comparing with FLT_EPSILON also avoids the rare case of an overflow.
        // FLT_EPSILON is small enough to be considered equivalent to zero in this application.
        float inverseScaleX = (fabsf(scale.GetX()) > FLT_EPSILON) ? 1.0f / scale.GetX() : 1;
        float inverseScaleY = (fabsf(scale.GetY()) > FLT_EPSILON) ? 1.0f / scale.GetY() : 1;

        AZ::Vector3 scale3(inverseScaleX, inverseScaleY, 1); // inverse scale

        AZ::Matrix4x4 moveToPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(-pivot3);
        AZ::Matrix4x4 scaleMat = AZ::Matrix4x4::CreateScale(scale3);
        AZ::Matrix4x4 rotMat = AZ::Matrix4x4::CreateRotationZ(rotRad);
        AZ::Matrix4x4 moveFromPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(pivot3);

        mat = moveFromPivotSpaceMat * scaleMat * rotMat * moveToPivotSpaceMat;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert a bool field to an int for ScaleToDevice
    inline bool ConvertScaleToDeviceFromBoolToEnum(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // Note that the name of the new element has to be the same as the name of the old element
        // because we have no version conversion for data patches. The bool to enum conversion happens
        // to work out for the data patches because the bool value of 1 maps to the correct int value.
        constexpr const char* ScaleToDeviceName = "ScaleToDevice";

        int index = classElement.FindElement(AZ_CRC_CE(ScaleToDeviceName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            bool oldData;

            if (!elementNode.GetData(oldData))
            {
                // Error, old subElement was not a bool or not valid
                AZ_Error("Serialization", false, "Cannot get bool data for element %s.", ScaleToDeviceName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add a new element for the new data.
            int newElementIndex = classElement.AddElement<int>(context, ScaleToDeviceName);
            if (newElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", ScaleToDeviceName);
                return false;
            }

            int newData = (oldData) ?
                static_cast<int>(UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit) :
                static_cast<int>(UiTransformInterface::ScaleToDeviceMode::None);

            classElement.GetSubElement(newElementIndex).SetData(context, newData);
        }

        // if the field did not exist then we do not report an error
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent::UiTransform2dComponent()
    : m_pivot(AZ::Vector2(0.5f, 0.5f))
    , m_rotation(0.0f)
    , m_scale(AZ::Vector2(1.0f, 1.0f))
    , m_isFlooringOffsets(false)
    , m_scaleToDeviceMode(ScaleToDeviceMode::None)
    , m_recomputeTransformToViewport(true)
    , m_recomputeTransformToCanvasSpace(true)
    , m_recomputeCanvasSpaceRect(true)
    , m_rectInitialized(false)
    , m_rectChangedByInitialization(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent::~UiTransform2dComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetZRotation()
{
    return m_rotation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetZRotation(float rotation)
{
    if (m_rotation != rotation)
    {
        m_rotation = rotation;
        SetRecomputeFlags(UiTransformInterface::Recompute::TransformOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetScale()
{
    return m_scale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetScale(AZ::Vector2 scale)
{
    if (m_scale != scale)
    {
        m_scale = scale;
        SetRecomputeFlags(UiTransformInterface::Recompute::TransformOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetScaleX()
{
    return m_scale.GetX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetScaleX(float scale)
{
    return SetScale(AZ::Vector2(scale, m_scale.GetY()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetScaleY()
{
    return m_scale.GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetScaleY(float scale)
{
    return SetScale(AZ::Vector2(m_scale.GetX(), scale));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetPivot()
{
    return m_pivot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetPivot(AZ::Vector2 pivot)
{
    if (m_pivot != pivot)
    {
        m_pivot = pivot;
        // changing the pivot does not change the rect, but if there is scale or rotation it does affect the transform.
        // However, we do want to notify other components if the pivot changes (for example the ImageComponent in
        // fixed mode is affected). So we recompute regardless of whether there is a scale or rotation.
        SetRecomputeFlags(UiTransformInterface::Recompute::TransformOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetPivotX()
{
    return m_pivot.GetX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetPivotX(float pivot)
{
    return SetPivot(AZ::Vector2(pivot, m_pivot.GetY()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetPivotY()
{
    return m_pivot.GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetPivotY(float pivot)
{
    return SetPivot(AZ::Vector2(m_pivot.GetX(), pivot));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::GetIsFlooringOffsets()
{
    return m_isFlooringOffsets;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetIsFlooringOffsets(bool isFlooringOffsets)
{
    if (m_isFlooringOffsets != isFlooringOffsets)
    {
        m_isFlooringOffsets = isFlooringOffsets;
        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent::ScaleToDeviceMode UiTransform2dComponent::GetScaleToDeviceMode()
{
    return m_scaleToDeviceMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetScaleToDeviceMode(ScaleToDeviceMode scaleToDeviceMode)
{
    if (m_scaleToDeviceMode != scaleToDeviceMode)
    {
        m_scaleToDeviceMode = scaleToDeviceMode;
        SetRecomputeFlags(UiTransformInterface::Recompute::TransformOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetViewportSpacePoints(RectPoints& points)
{
    GetCanvasSpacePointsNoScaleRotate(points);
    RotateAndScalePoints(points);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetViewportSpacePivot()
{
    // this function is primarily used for drawing the pivot in the editor. Since we snap the pivot
    // icon to the nearest pixel, if the X position is something like 20.5 it will snap different ways
    // depending on rounding errors. We don't want this to happen while rotating an element. So, make
    // sure the ViewportSpacePivot is calculated in a way that is independent of this element's
    // scale and rotation.
    AZ::Vector2 canvasSpacePivot = GetCanvasSpacePivotNoScaleRotate();
    AZ::Vector3 point3(canvasSpacePivot.GetX(), canvasSpacePivot.GetY(), 0.0f);

    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (parentTransformComponent)
    {
        AZ::Matrix4x4 transform;
        parentTransformComponent->GetTransformToViewport(transform);

        point3 = transform * point3;
    }

    return AZ::Vector2(point3.GetX(), point3.GetY());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetTransformToViewport(AZ::Matrix4x4& mat)
{
    RecomputeTransformToViewportIfNeeded();
    mat = m_transformToViewport;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetTransformFromViewport(AZ::Matrix4x4& mat)
{
    // first get the transform from canvas space
    GetTransformFromCanvasSpace(mat);

    // then get the transform from viewport to canvas space
    AZ::Matrix4x4 viewportToCanvasMatrix;
    if (IsFullyInitialized())
    {
        GetCanvasComponent()->GetViewportToCanvasMatrix(viewportToCanvasMatrix);
    }
    else
    {
        EmitNotInitializedWarning();
        viewportToCanvasMatrix = AZ::Matrix4x4::CreateIdentity();
    }

    // add the transform from viewport space to canvas space to the transform matrix
    mat = mat * viewportToCanvasMatrix;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::RotateAndScalePoints(RectPoints& points)
{
    if (IsFullyInitialized() && GetElementComponent()->GetParent())
    {
        AZ::Matrix4x4 transform;
        GetTransformToViewport(transform);

        points = points.Transform(transform);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetCanvasSpacePoints(RectPoints& points)
{
    GetCanvasSpacePointsNoScaleRotate(points);

    // apply the transform to canvas space
    if (IsFullyInitialized() && GetElementComponent()->GetParent())
    {
        AZ::Matrix4x4 transform;
        GetTransformToCanvasSpace(transform);

        points = points.Transform(transform);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetCanvasSpacePivot()
{
    AZ::Vector2 canvasSpacePivot = GetCanvasSpacePivotNoScaleRotate();
    AZ::Vector3 point3(canvasSpacePivot.GetX(), canvasSpacePivot.GetY(), 0.0f);

    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (parentTransformComponent)
    {
        AZ::Matrix4x4 transform;
        parentTransformComponent->GetTransformToCanvasSpace(transform);

        point3 = transform * point3;
    }

    return AZ::Vector2(point3.GetX(), point3.GetY());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetTransformToCanvasSpace(AZ::Matrix4x4& mat)
{
    RecomputeTransformToCanvasSpaceIfNeeded();
    mat = m_transformToCanvasSpace;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetTransformFromCanvasSpace(AZ::Matrix4x4& mat)
{
    // this takes a matrix and builds the concatenation of this elements rotate and scale about the pivot
    // with the transforms for all parent elements into one 3x4 matrix.
    // The result is an inverse transform that can be used to map from transformed space to non-transformed
    // space
    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (parentTransformComponent)
    {
        parentTransformComponent->GetTransformFromCanvasSpace(mat);

        AZ::Matrix4x4 transformFromParent;
        GetLocalInverseTransform(transformFromParent);

        mat = transformFromParent * mat;
    }
    else
    {
        mat = AZ::Matrix4x4::CreateIdentity();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetCanvasSpaceRectNoScaleRotate(Rect& rect)
{
    CalculateCanvasSpaceRect();
    rect = m_rect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetCanvasSpacePointsNoScaleRotate(RectPoints& points)
{
    Rect rect;
    GetCanvasSpaceRectNoScaleRotate(rect);
    points.SetAxisAligned(rect.left, rect.right, rect.top, rect.bottom);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetCanvasSpaceSizeNoScaleRotate()
{
    Rect rect;
    GetCanvasSpaceRectNoScaleRotate(rect);
    return rect.GetSize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetCanvasSpacePivotNoScaleRotate()
{
    Rect rect;
    GetCanvasSpaceRectNoScaleRotate(rect);

    AZ::Vector2 size = rect.GetSize();

    float x =  rect.left + size.GetX() * m_pivot.GetX();
    float y =  rect.top + size.GetY() * m_pivot.GetY();

    return AZ::Vector2(x, y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetLocalTransform(AZ::Matrix4x4& mat)
{
    if (HasScaleOrRotation())
    {
        // this takes a matrix and builds the concatenation of this element's rotate and scale about the pivot
        AZ::Vector2 pivot = GetCanvasSpacePivotNoScaleRotate();
        AZ::Vector3 pivot3(pivot.GetX(), pivot.GetY(), 0);

        float rotRad = DEG2RAD(m_rotation);     // rotation

        AZ::Vector2 scale = GetScaleAdjustedForDevice();
        AZ::Vector3 scale3(scale.GetX(), scale.GetY(), 1);   // scale

        AZ::Matrix4x4 moveToPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(-pivot3);
        AZ::Matrix4x4 scaleMat = AZ::Matrix4x4::CreateScale(scale3);
        AZ::Matrix4x4 rotMat = AZ::Matrix4x4::CreateRotationZ(rotRad);
        AZ::Matrix4x4 moveFromPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(pivot3);

        mat = moveFromPivotSpaceMat * rotMat * scaleMat * moveToPivotSpaceMat;
    }
    else
    {
        mat = AZ::Matrix4x4::CreateIdentity();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::GetLocalInverseTransform(AZ::Matrix4x4& mat)
{
    if (HasScaleOrRotation())
    {
        // this takes a matrix and builds the concatenation of this element's rotate and scale about the pivot
        // The result is an inverse transform that can be used to map from parent space to non-transformed
        // space
        AZ::Vector2 pivot = GetCanvasSpacePivotNoScaleRotate();
        AZ::Vector2 scale = GetScaleAdjustedForDevice();
        GetInverseTransform(pivot, scale, m_rotation, mat);
    }
    else
    {
        mat = AZ::Matrix4x4::CreateIdentity();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::HasScaleOrRotation()
{
    return (m_scaleToDeviceMode != ScaleToDeviceMode::None || m_scale.GetX() != 1.0f || m_scale.GetY() != 1.0f || m_rotation != 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetViewportPosition()
{
    return GetViewportSpacePivot();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetViewportPosition(const AZ::Vector2& position)
{
    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (!parentTransformComponent)
    {
        return; // this is the root element
    }

    AZ::Vector2 curCanvasSpacePosition = GetCanvasSpacePivotNoScaleRotate();

    AZ::Matrix4x4 transform;
    parentTransformComponent->GetTransformFromViewport(transform);

    AZ::Vector3 point3(position.GetX(), position.GetY(), 0.0f);
    point3 = transform * point3;
    AZ::Vector2 canvasSpacePosition(point3.GetX(), point3.GetY());

    if (canvasSpacePosition != curCanvasSpacePosition)
    {
        m_offsets += canvasSpacePosition - curCanvasSpacePosition;
        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetCanvasPosition()
{
    return GetCanvasSpacePivot();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetCanvasPosition(const AZ::Vector2& position)
{
    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (!parentTransformComponent)
    {
        return; // this is the root element
    }

    AZ::Vector2 curCanvasSpacePosition = GetCanvasSpacePivotNoScaleRotate();

    AZ::Matrix4x4 transform;
    parentTransformComponent->GetTransformFromCanvasSpace(transform);

    AZ::Vector3 point3(position.GetX(), position.GetY(), 0.0f);
    point3 = transform * point3;
    AZ::Vector2 canvasSpacePosition(point3.GetX(), point3.GetY());

    if (canvasSpacePosition != curCanvasSpacePosition)
    {
        m_offsets += canvasSpacePosition - curCanvasSpacePosition;
        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetLocalPosition()
{
    AZ::Vector2 position = GetCanvasSpacePivotNoScaleRotate() - GetCanvasSpaceAnchorsCenterNoScaleRotate();
    return position;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetLocalPosition(const AZ::Vector2& position)
{
    AZ::Vector2 curPosition = GetLocalPosition();

    if (position != curPosition)
    {
        m_offsets += position - curPosition;
        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetLocalPositionX()
{
    AZ::Vector2 position = GetCanvasSpacePivotNoScaleRotate() - GetCanvasSpaceAnchorsCenterNoScaleRotate();
    return position.GetX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetLocalPositionX(float position)
{
    AZ::Vector2 curPosition = GetLocalPosition();
    SetLocalPosition(AZ::Vector2(position, curPosition.GetY()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetLocalPositionY()
{
    AZ::Vector2 position = GetCanvasSpacePivotNoScaleRotate() - GetCanvasSpaceAnchorsCenterNoScaleRotate();
    return position.GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetLocalPositionY(float position)
{
    AZ::Vector2 curPosition = GetLocalPosition();
    SetLocalPosition(AZ::Vector2(curPosition.GetX(), position));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::MoveViewportPositionBy(const AZ::Vector2& offset)
{
    SetViewportPosition(GetViewportPosition() + offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::MoveCanvasPositionBy(const AZ::Vector2& offset)
{
    SetCanvasPosition(GetCanvasPosition() + offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::MoveLocalPositionBy(const AZ::Vector2& offset)
{
    SetLocalPosition(GetLocalPosition() + offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::IsPointInRect(AZ::Vector2 point)
{
    // get point in the no scale/rotate canvas space for this element
    AZ::Matrix4x4 transform;
    GetTransformFromViewport(transform);
    AZ::Vector3 point3(point.GetX(), point.GetY(), 0.0f);
    point3 = transform * point3;

    // get the rect for this element in the same space
    Rect rect;
    GetCanvasSpaceRectNoScaleRotate(rect);

    float left   = rect.left;
    float right  = rect.right;
    float top    = rect.top;
    float bottom = rect.bottom;

    // allow for "flipped" rects
    if (left > right)
    {
        std::swap(left, right);
    }
    if (top > bottom)
    {
        std::swap(top, bottom);
    }

    // point is in rect if it is within rect or exactly on edge
    if (point3.GetX() >= left &&
        point3.GetX() <= right &&
        point3.GetY() >= top &&
        point3.GetY() <= bottom)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::BoundsAreOverlappingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1)
{
    // Get the element points in viewport space
    RectPoints points;
    GetViewportSpacePoints(points);

    // If the element is axis aligned we can just do an AAAB to AABB intersection test.
    // This is by far the most common case in UI canvases
    if (points.TopLeft().GetY() == points.TopRight().GetY() && points.TopLeft().GetX() <= points.TopRight().GetX() &&
        points.TopLeft().GetX() == points.BottomLeft().GetX() && points.TopLeft().GetY() <= points.BottomLeft().GetY())
    {
        // the element has no rotation and is not flipped so use AABB test
        return AxisAlignedBoxesIntersect(bound0, bound1, points.TopLeft(), points.BottomRight());
    }

    // IMPORTANT: This collision detection algorithm is based on the
    // Separating Axis Theorem, but is optimized for this context.
    // This ISN'T a generalized implementation. We DISCOURAGE using
    // this implementation elsewhere.
    //
    // Reference:
    // http://en.wikipedia.org/wiki/Hyperplane_separation_theorem

    // Vertices from shape A (input shape, which is axis-aligned).
    std::list<AZ::Vector2> vertsA;
    {
        // bound0
        //        A----B
        //        |    |
        //        D----C
        //               bound1

        vertsA.push_back(bound0); // A.
        vertsA.push_back(AZ::Vector2(bound1.GetX(), bound0.GetY())); // B.
        vertsA.push_back(bound1); // C.
        vertsA.push_back(AZ::Vector2(bound0.GetX(), bound1.GetY())); // D.
    }

    // Vertices from shape B (our shape, which ISN'T axis-aligned).
    RectPoints vertsB = points;

    // Normals from shape A (input shape, which is axis-aligned).
    // IMPORTANT: This ISN'T thread-safe.
    static std::list<AZ::Vector2> edgeNormalsA;
    if (edgeNormalsA.empty())
    {
        edgeNormalsA.push_back(AZ::Vector2(0.0f, 1.0f));
        edgeNormalsA.push_back(AZ::Vector2(1.0f, 0.0f));
        edgeNormalsA.push_back(AZ::Vector2(0.0f, -1.0f));
        edgeNormalsA.push_back(AZ::Vector2(-1.0f, 0.0f));
    }

    // All edge normals.
    std::list<AZ::Vector2> edgeNormals(edgeNormalsA);

    // Normals from shape B (our rect shape, which ISN'T axis-aligned).
    {
        // A----B
        // |    |
        // D----C
        const AZ::Vector2& A = vertsB.TopLeft();
        const AZ::Vector2& B = vertsB.TopRight();
        const AZ::Vector2& C = vertsB.BottomRight();
        const AZ::Vector2& D = vertsB.BottomLeft();

        AZ::Vector2 normAB((B - A).GetNormalized().GetPerpendicular());
        AZ::Vector2 normBC((C - B).GetNormalized().GetPerpendicular());
        AZ::Vector2 normCD((D - C).GetNormalized().GetPerpendicular());
        AZ::Vector2 normDA((A - D).GetNormalized().GetPerpendicular());

        edgeNormals.push_back(normAB);
        edgeNormals.push_back(normBC);
        edgeNormals.push_back(normCD);
        edgeNormals.push_back(normDA);
    }

    // A collision occurs only when we CAN'T find any gaps.
    // To find a gap, we project all vertices against all normals.
    for (auto && n : edgeNormals)
    {
        std::set<float> vertsAdot;
        std::set<float> vertsBdot;

        for (auto && v : vertsA)
        {
            vertsAdot.insert(n.Dot(v));
        }

        for (auto && v : vertsB.pt)
        {
            vertsBdot.insert(n.Dot(v));
        }

        float minA = *vertsAdot.begin();
        float maxA = *vertsAdot.rbegin();
        float minB = *vertsBdot.begin();
        float maxB = *vertsBdot.rbegin();

        // Two intervals overlap if:
        //
        // ( ( A.min < B.max ) &&
        //   ( A.max > B.min ) )
        //
        // Visual reference:
        // http://silentmatt.com/rectangle-intersection/
        if (!(minA < maxB) && (maxA > minB))
        {
            // Stop as soon as we find a gap.
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetRecomputeFlags(Recompute recompute)
{
    if (!IsFullyInitialized())
    {
        // If not initialized yet then transform will be recomputed after Fixup so no need to emit warning
        return;
    }

    if (m_isFlooringOffsets && (recompute == Recompute::RectOnly || recompute == Recompute::RectAndTransform))
    {
        m_offsets.m_right = floorf(m_offsets.m_right);
        m_offsets.m_left = floorf(m_offsets.m_left);
        m_offsets.m_top = floorf(m_offsets.m_top);
        m_offsets.m_bottom = floorf(m_offsets.m_bottom);
    }

    if (recompute == Recompute::RectOnly && HasScaleOrRotation())
    {
        // if this element has scale or rotation then a rect change will require the transforms to be recomputed
        // This is an optimization because, in most canvases, most elements have no scale or rotation
        recompute = Recompute::RectAndTransform;
    }

    int numChildren = GetElementComponent()->GetNumChildElements();
    for (int i = 0; i < numChildren; i++)
    {
        UiTransform2dComponent* childTransformComponent = GetChildTransformComponent(i);
        if (childTransformComponent)
        {
            childTransformComponent->SetRecomputeFlags(recompute);
        }
    }

    switch (recompute)
    {
    case Recompute::RectOnly:
        m_recomputeCanvasSpaceRect = true;
        break;
    case Recompute::TransformOnly:
        m_recomputeTransformToCanvasSpace = true;
        m_recomputeTransformToViewport = true;
        break;
    case Recompute::ViewportTransformOnly:
        m_recomputeTransformToViewport = true;
        break;
    case Recompute::RectAndTransform:
        m_recomputeTransformToCanvasSpace = true;
        m_recomputeTransformToViewport = true;
        m_recomputeCanvasSpaceRect = true;
        break;
    }

    // Tell the canvas that this element needs a recompute
    GetCanvasComponent()->ScheduleElementForTransformRecompute(GetElementComponent());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::HasCanvasSpaceRectChanged()
{
    CalculateCanvasSpaceRect();

    return (HasCanvasSpaceRectChangedByInitialization() || m_rect != m_prevRect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::HasCanvasSpaceSizeChanged()
{
    if (HasCanvasSpaceRectChanged())
    {
        static const float sizeChangeTolerance = 0.05f;

        // If old rect equals new rect, size changed due to initialization
        return (HasCanvasSpaceRectChangedByInitialization() || !m_prevRect.GetSize().IsClose(m_rect.GetSize(), sizeChangeTolerance));
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::HasCanvasSpaceRectChangedByInitialization()
{
    return m_rectChangedByInitialization;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::NotifyAndResetCanvasSpaceRectChange()
{
    if (HasCanvasSpaceRectChanged())
    {
        // Reset before sending the notification because the notification could trigger a new rect change
        Rect prevRect = m_prevRect;
        m_prevRect = m_rect;
        m_rectChangedByInitialization = false;
        UiTransformChangeNotificationBus::Event(
            GetEntityId(), &UiTransformChangeNotificationBus::Events::OnCanvasSpaceRectChanged, GetEntityId(), prevRect, m_rect);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent::Anchors UiTransform2dComponent::GetAnchors()
{
    return m_anchors;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetAnchors(Anchors anchors, bool adjustOffsets, bool allowPush)
{
    Anchors oldAnchors = m_anchors;
    Offsets oldOffsets = m_offsets;

    // First adjust the input structure to be valid.
    // If either pair of anchors is flipped then set them to be the same.
    // To avoid changing one anchor "pushing" the other we check which one changed and correct that
    // unless allowPush is set in which case we do the opposite
    if (anchors.m_right < anchors.m_left)
    {
        if (anchors.m_right != m_anchors.m_right)
        {
            // right anchor changed
            if (allowPush)
            {
                anchors.m_left = anchors.m_right;   // push left to match right
            }
            else
            {
                anchors.m_right = anchors.m_left;   // clamp to right to equal left
            }
        }
        else
        {
            // left changed or both changed
            if (allowPush)
            {
                anchors.m_right = anchors.m_left;   // push right to match left
            }
            else
            {
                anchors.m_left = anchors.m_right;   // clamp left to equal right
            }
        }
    }

    if (anchors.m_bottom < anchors.m_top)
    {
        if (anchors.m_bottom != m_anchors.m_bottom)
        {
            // bottom anchor changed
            if (allowPush)
            {
                anchors.m_top = anchors.m_bottom;   // push top to match bottom
            }
            else
            {
                anchors.m_bottom = anchors.m_top;   // clamp bottom to equal top
            }
        }
        else
        {
            // top changed or both changed
            if (allowPush)
            {
                anchors.m_bottom = anchors.m_top;   // push bottom to match top
            }
            else
            {
                anchors.m_top = anchors.m_bottom;   // clamp top to equal bottom
            }
        }
    }

    if (adjustOffsets)
    {
        // now we need to adjust the offsets
        UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
        if (parentTransformComponent)
        {
            AZ::Vector2 parentSize = parentTransformComponent->GetCanvasSpaceSizeNoScaleRotate();

            m_offsets.m_left    -= parentSize.GetX() * (anchors.m_left - m_anchors.m_left);
            m_offsets.m_right   -= parentSize.GetX() * (anchors.m_right - m_anchors.m_right);
            m_offsets.m_top     -= parentSize.GetY() * (anchors.m_top - m_anchors.m_top);
            m_offsets.m_bottom  -= parentSize.GetY() * (anchors.m_bottom - m_anchors.m_bottom);
        }
    }

    // now actually change the anchors
    m_anchors = anchors;

    // now, if the anchors are the same in a dimension we check that the offsets are not flipped in that dimension
    // if they are we set them to be zero apart. This is a rule when the anchors are together in order to prevent
    // displaying a negative width or height
    if (m_anchors.m_left == m_anchors.m_right && m_offsets.m_left > m_offsets.m_right)
    {
        // left and right offsets are flipped, set to their midpoint
        m_offsets.m_left = m_offsets.m_right = (m_offsets.m_left + m_offsets.m_right) * 0.5f;
    }
    if (m_anchors.m_top == m_anchors.m_bottom && m_offsets.m_top > m_offsets.m_bottom)
    {
        // top and bottom offsets are flipped, set to their midpoint
        m_offsets.m_top = m_offsets.m_bottom = (m_offsets.m_top + m_offsets.m_bottom) * 0.5f;
    }

    if (oldAnchors != m_anchors || oldOffsets != m_offsets)
    {
        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent::Offsets UiTransform2dComponent::GetOffsets()
{
    return m_offsets;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetOffsets(Offsets offsets)
{
    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (!parentTransformComponent)
    {
        return; // cannot set offsets on the root element
    }

    // first adjust the input structure to be valid
    // if either pair of offsets is flipped then set them to be the same
    // to avoid changing one offset "pushing" the other we check which one changed and correct that
    // NOTE: To see if an offset is flipped we have to take into account all the parents, the calculation
    // below is based on the calculation in GetCanvasSpaceRectNoScaleRotate but needs to be able to do
    // it in reverse also
    // NOTE: if a parent changes size this can cause offsets to flip and this is OK - we treat it as a zero
    // rect in that dimension in GetCanvasSpaceRectNoScaleRotate. But if the offsets on this element are
    // being changed then we do enforce the "no flipping" rule.

    Rect parentRect;
    parentTransformComponent->GetCanvasSpaceRectNoScaleRotate(parentRect);

    AZ::Vector2 parentSize = parentRect.GetSize();

    float left   = parentRect.left + parentSize.GetX() * m_anchors.m_left   + offsets.m_left;
    float right  = parentRect.left + parentSize.GetX() * m_anchors.m_right  + offsets.m_right;
    float top    = parentRect.top  + parentSize.GetY() * m_anchors.m_top    + offsets.m_top;
    float bottom = parentRect.top  + parentSize.GetY() * m_anchors.m_bottom + offsets.m_bottom;

    if (left > right)
    {
        // left/right offsets are flipped
        bool leftChanged = offsets.m_left != m_offsets.m_left;
        bool rightChanged = offsets.m_right != m_offsets.m_right;

        if (leftChanged && rightChanged)
        {
            // Both changed. This usually happens when resizing by gizmo, which is about the pivot.
            // So rather than taking the midpoint (which the below calculation effectively does for the normal
            // case of pivot.GetX() = 0.5f) we take the point between the two values using the pivot as a ratio.
            // This makes sense even if not resizing by gizmo. When the width is zero the pivot position
            // is always co-incident with the left and right edges. So this calculation moves the two points
            // together without moving the pivot position.
            float newValue = left * (1.0f - m_pivot.GetX()) + right * m_pivot.GetX();
            offsets.m_left  = newValue - (parentRect.left + parentSize.GetX() * m_anchors.m_left);
            offsets.m_right = newValue - (parentRect.left + parentSize.GetX() * m_anchors.m_right);
        }
        else if (rightChanged)
        {
            // the right offset changed, correct that one
            offsets.m_right = left - (parentRect.left + parentSize.GetX() * m_anchors.m_right);
        }
        else if (leftChanged)
        {
            // the left offset changed, correct that one
            offsets.m_left = right - (parentRect.left + parentSize.GetX() * m_anchors.m_left);
        }
    }

    if (top > bottom)
    {
        // top/bottom offsets are flipped
        bool topChanged = offsets.m_top != m_offsets.m_top;
        bool bottomChanged = offsets.m_bottom != m_offsets.m_bottom;

        if (topChanged && bottomChanged)
        {
            // Both changed. This usually happens when resizing by gizmo, which is about the pivot.
            // So rather than taking the midpoint (which the below calculation effectively does for the normal
            // case of pivot.GetY() = 0.5f) we take the point between the two values using the pivot as a ratio.
            float newValue = top * (1.0f - m_pivot.GetY()) + bottom * m_pivot.GetY();
            offsets.m_top    = newValue - (parentRect.top + parentSize.GetY() * m_anchors.m_top);
            offsets.m_bottom = newValue - (parentRect.top + parentSize.GetY() * m_anchors.m_bottom);
        }
        else if (bottomChanged)
        {
            // the bottom offset changed, correct that one
            offsets.m_bottom = top - (parentRect.top + parentSize.GetY() * m_anchors.m_bottom);
        }
        else if (topChanged)
        {
            // the top offset changed, correct that one
            offsets.m_top = bottom - (parentRect.top + parentSize.GetY() * m_anchors.m_top);
        }
    }

    if (m_offsets != offsets)
    {
        m_offsets = offsets;
        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetPivotAndAdjustOffsets(AZ::Vector2 pivot)
{
    if (m_pivot == pivot)
    {
        return;
    }

    // if the element has local rotation or scale then we have to modify the offsets to keep the rect from moving
    // in transformed space.
    if (HasScaleOrRotation())
    {
        // Get the untransformed canvas space points and rect before we change the pivot
        RectPoints oldCanvasSpacePoints;
        GetCanvasSpacePointsNoScaleRotate(oldCanvasSpacePoints);
        Rect oldCanvasSpaceRect;
        GetCanvasSpaceRectNoScaleRotate(oldCanvasSpaceRect);

        // apply just this elements rotate and scale (must be done before changing pivot)
        // NOTE: this element's pivot only affects the local transformation so there is no need to apply all the
        // transforms up the hierarchy.
        AZ::Matrix4x4 localTransform;
        GetLocalTransform(localTransform);
        RectPoints localTransformedPoints = oldCanvasSpacePoints.Transform(localTransform);

        // Set the new pivot
        SetPivot(pivot);

        // Now work out what the canvas space pivot point would have to be to result in the same transformed points
        AZ::Vector2 rightVec = localTransformedPoints.TopRight() - localTransformedPoints.TopLeft();
        AZ::Vector2 downVec = localTransformedPoints.BottomLeft() - localTransformedPoints.TopLeft();
        AZ::Vector2 canvasSpacePivot = localTransformedPoints.TopLeft() + pivot.GetX() * rightVec + pivot.GetY() * downVec;

        // We know that changing the pivot will not change the size of the canvas space rect, just its position.
        // So from this new canvas space pivot point work out where the top left of the new canvas space rect would be
        AZ::Vector2 oldSize = oldCanvasSpaceRect.GetSize();
        float newLeft = canvasSpacePivot.GetX() - oldSize.GetX() * pivot.GetX();
        float newTop = canvasSpacePivot.GetY() - oldSize.GetY() * pivot.GetY();

        // we can then compute how much the rect has moved and just apply that delta to the offsets
        float deltaX = newLeft - oldCanvasSpaceRect.left;
        float deltaY = newTop - oldCanvasSpaceRect.top;

        m_offsets.m_left += deltaX;
        m_offsets.m_right += deltaX;
        m_offsets.m_top += deltaY;
        m_offsets.m_bottom += deltaY;

        SetRecomputeFlags(UiTransformInterface::Recompute::RectOnly);
    }
    else
    {
        // no scale or rotation, just set the pivot
        SetPivot(pivot);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetLocalWidth(float width)
{
    // If anchors are different the local width isn't a fixed quantity
    if (m_anchors.m_left == m_anchors.m_right)
    {
        Offsets offsets = GetOffsets();
        float curWidth = m_offsets.m_right - m_offsets.m_left;
        float diff = width - curWidth;
        offsets.m_left -= diff * m_pivot.GetX();
        offsets.m_right += diff * (1.0f - m_pivot.GetX());
        SetOffsets(offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetLocalWidth()
{
    float width = 0;
    // If anchors are different the local width isn't a fixed quantity
    if (m_anchors.m_left == m_anchors.m_right)
    {
        width = m_offsets.m_right - m_offsets.m_left;
    }
    else
    {
        width = GetCanvasSpaceSizeNoScaleRotate().GetX();
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::SetLocalHeight(float height)
{
    // If anchors are different the local height isn't a fixed quantity
    if (m_anchors.m_top == m_anchors.m_bottom)
    {
        Offsets offsets = GetOffsets();
        float curHeight = m_offsets.m_bottom - m_offsets.m_top;
        float diff = height - curHeight;
        offsets.m_top -= diff * m_pivot.GetY();
        offsets.m_bottom += diff * (1.0f - m_pivot.GetY());
        SetOffsets(offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTransform2dComponent::GetLocalHeight()
{
    float height = 0;
    // If anchors are different the local height isn't a fixed quantity
    if (m_anchors.m_top == m_anchors.m_bottom)
    {
        height = m_offsets.m_bottom - m_offsets.m_top;
    }
    else
    {
        height = GetCanvasSpaceSizeNoScaleRotate().GetY();
    }

    return height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::PropertyValuesChanged()
{
    SetRecomputeFlags(UiTransformInterface::Recompute::RectAndTransform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::RecomputeTransformsAndSendNotifications()
{
    NotifyAndResetCanvasSpaceRectChange();
    RecomputeTransformToViewportIfNeeded();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTransform2dComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiTransform2dComponent, AZ::Component>()

            ->Version(4, &VersionConverter)
            ->Field("Anchors", &UiTransform2dComponent::m_anchors)
            ->Field("Offsets", &UiTransform2dComponent::m_offsets)
            ->Field("Pivot", &UiTransform2dComponent::m_pivot)
            ->Field("Rotation", &UiTransform2dComponent::m_rotation)
            ->Field("Scale", &UiTransform2dComponent::m_scale)
            ->Field("IsFlooringOffsets", &UiTransform2dComponent::m_isFlooringOffsets)
            ->Field("ScaleToDevice", &UiTransform2dComponent::m_scaleToDeviceMode);

        // EditContext. Note that the Transform component is unusual in that we want to hide the
        // properties when the transform is controlled by the parent. There is not a standard
        // way to hide all the properties and replace them by a message. We could hide them all
        // using the "Visibility" attribute, but then the component name itself is not even shown.
        // We really want to be able to display a message indicating why the properties are not shown.
        // Alternatively we could make them all read-only using the "ReadOnly" property. Again this
        // doesn't tell the user why.
        // So the approach we use is:
        // - Hide all of the properties except Anchors using the "Visibility" property
        // - Set the Anchors property to ReadOnly and change the ProertyHandler for Anchors to
        //   display a message in this case (and have a different tooltip)
        // - Dynamically change the property name of the Anchors property using the
        //   "NameLabelOverride" attribute.
        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiTransform2dComponent>("Transform2D",
                    "All 2D UI elements have this component.\n"
                    "It controls the placement of the element's rectangle relative to its parent");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiTransform2d.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiTransform2d.png")
                ->Attribute(AZ::Edit::Attributes::AddableByUser, false)     // Cannot be added or removed by user
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("Anchor", &UiTransform2dComponent::m_anchors, "Anchors",
                "The anchors specify proportional positions within the parent element's rectangle.\n"
                "If the anchors are together (e.g. left = right or top = bottom) then, in that dimension,\n"
                "there is a single anchor point that the element is offset from.\n"
                "If they are apart, then there are two anchor points and as the parent changes size\n"
                "this element will change size also")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues")) // Refresh attributes for scale to device mode
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, "%")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show) // needed because sub-elements are hidden
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &UiTransform2dComponent::IsControlledByParent)
                ->Attribute(AZ_CRC_CE("LayoutFitterType"), &UiTransform2dComponent::GetLayoutFitterType)
                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &UiTransform2dComponent::GetAnchorPropertyLabel);

            editInfo->DataElement("Offset", &UiTransform2dComponent::m_offsets, "Offsets",
                "The offsets (in pixels) from the anchors.\n"
                "When anchors are together, the offset to the pivot plus the size is displayed.\n"
                "When they are apart, the offsets to each edge of the element's rect are displayed")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshValues"))
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiTransform2dComponent::IsNotControlledByParent)
                ->Attribute(AZ_CRC_CE("LayoutFitterType"), &UiTransform2dComponent::GetLayoutFitterType)
                ->Attribute(AZ::Edit::Attributes::Min, -AZ::Constants::MaxFloatBeforePrecisionLoss)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::MaxFloatBeforePrecisionLoss);

            editInfo->DataElement("Pivot", &UiTransform2dComponent::m_pivot, "Pivot",
                "Rotation and scaling happens around the pivot point.\n"
                "If the anchors are together then the offsets specify the offset from the anchor to the pivot")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshValues"))
                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Min, -AZ::Constants::MaxFloatBeforePrecisionLoss)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::MaxFloatBeforePrecisionLoss);

            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiTransform2dComponent::m_rotation, "Rotation",
                "The rotation in degrees about the pivot point")
                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTransform2dComponent::OnTransformPropertyChanged);

            editInfo->DataElement(0, &UiTransform2dComponent::m_scale, "Scale",
                "The X and Y scale around the pivot point")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTransform2dComponent::OnTransformPropertyChanged)
                ->Attribute(AZ::Edit::Attributes::Min, -AZ::Constants::MaxFloatBeforePrecisionLoss)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::MaxFloatBeforePrecisionLoss);

            editInfo->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &UiTransform2dComponent::m_isFlooringOffsets,
                "Floor offsets",
                "When checked, this element's offsets are floored");

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTransform2dComponent::m_scaleToDeviceMode, "Scale to device",
                "Controls how this element and all its children will be scaled to allow for\n"
                "the difference between the authored canvas size and the actual viewport size")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::None, "None")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit,  "Scale to fit (uniformly)")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::UniformScaleToFill, "Scale to fill (uniformly)")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::UniformScaleToFitX, "Scale to fit X (uniformly)")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::UniformScaleToFitY, "Scale to fit Y (uniformly)")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::NonUniformScale,    "Stretch to fill (non-uniformly)")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::ScaleXOnly,         "Stretch to fit X (non-uniformly)")
                ->EnumAttribute(UiTransformInterface::ScaleToDeviceMode::ScaleYOnly,         "Stretch to fit Y (non-uniformly)")
                ->Attribute("Warning", &UiTransform2dComponent::GetScaleToDeviceModeWarningTooltipText)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"));
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiTransformInterface::ScaleToDeviceMode::None>("eUiScaleToDeviceMode_None")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit>("eUiScaleToDeviceMode_UniformScaleToFit")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::UniformScaleToFill>("eUiScaleToDeviceMode_UniformScaleToFill")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::UniformScaleToFitX>("eUiScaleToDeviceMode_UniformScaleToFitX")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::UniformScaleToFitY>("eUiScaleToDeviceMode_UniformScaleToFitY")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::NonUniformScale>("eUiScaleToDeviceMode_NonUniformScale")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::ScaleXOnly>("eUiScaleToDeviceMode_ScaleXOnly")
            ->Enum<(int)UiTransformInterface::ScaleToDeviceMode::ScaleYOnly>("eUiScaleToDeviceMode_ScaleYOnly");

        behaviorContext->EBus<UiTransformBus>("UiTransformBus")
            ->Event("GetZRotation", &UiTransformBus::Events::GetZRotation)
            ->Event("SetZRotation", &UiTransformBus::Events::SetZRotation)
            ->Event("GetScale", &UiTransformBus::Events::GetScale)
            ->Event("SetScale", &UiTransformBus::Events::SetScale)
            ->Event("GetScaleX", &UiTransformBus::Events::GetScaleX)
            ->Event("SetScaleX", &UiTransformBus::Events::SetScaleX)
            ->Event("GetScaleY", &UiTransformBus::Events::GetScaleY)
            ->Event("SetScaleY", &UiTransformBus::Events::SetScaleY)
            ->Event("GetPivot", &UiTransformBus::Events::GetPivot)
            ->Event("SetPivot", &UiTransformBus::Events::SetPivot)
            ->Event("GetPivotX", &UiTransformBus::Events::GetPivotX)
            ->Event("SetPivotX", &UiTransformBus::Events::SetPivotX)
            ->Event("GetPivotY", &UiTransformBus::Events::GetPivotY)
            ->Event("SetPivotY", &UiTransformBus::Events::SetPivotY)
            ->Event("GetScaleToDeviceMode", &UiTransformBus::Events::GetScaleToDeviceMode)
            ->Event("SetScaleToDeviceMode", &UiTransformBus::Events::SetScaleToDeviceMode)
            ->Event("GetViewportPosition", &UiTransformBus::Events::GetViewportPosition)
            ->Event("SetViewportPosition", &UiTransformBus::Events::SetViewportPosition)
            ->Event("GetCanvasPosition", &UiTransformBus::Events::GetCanvasPosition)
            ->Event("SetCanvasPosition", &UiTransformBus::Events::SetCanvasPosition)
            ->Event("GetLocalPosition", &UiTransformBus::Events::GetLocalPosition)
            ->Event("SetLocalPosition", &UiTransformBus::Events::SetLocalPosition)
            ->Event("GetLocalPositionX", &UiTransformBus::Events::GetLocalPositionX)
            ->Event("SetLocalPositionX", &UiTransformBus::Events::SetLocalPositionX)
            ->Event("GetLocalPositionY", &UiTransformBus::Events::GetLocalPositionY)
            ->Event("SetLocalPositionY", &UiTransformBus::Events::SetLocalPositionY)
            ->Event("MoveViewportPositionBy", &UiTransformBus::Events::MoveViewportPositionBy)
            ->Event("MoveCanvasPositionBy", &UiTransformBus::Events::MoveCanvasPositionBy)
            ->Event("MoveLocalPositionBy", &UiTransformBus::Events::MoveLocalPositionBy)
            ->VirtualProperty("ScaleX", "GetScaleX", "SetScaleX")
            ->VirtualProperty("ScaleY", "GetScaleY", "SetScaleY")
            ->VirtualProperty("PivotX", "GetPivotX", "SetPivotX")
            ->VirtualProperty("PivotY", "GetPivotY", "SetPivotY")
            ->VirtualProperty("LocalPositionX", "GetLocalPositionX", "SetLocalPositionX")
            ->VirtualProperty("LocalPositionY", "GetLocalPositionY", "SetLocalPositionY")
            ->VirtualProperty("Rotation", "GetZRotation", "SetZRotation");

        behaviorContext->EBus<UiTransform2dBus>("UiTransform2dBus")
            ->Event("GetAnchors", &UiTransform2dBus::Events::GetAnchors)
            ->Event("SetAnchors", &UiTransform2dBus::Events::SetAnchors)
            ->Event("GetOffsets", &UiTransform2dBus::Events::GetOffsets)
            ->Event("SetOffsets", &UiTransform2dBus::Events::SetOffsets)
            ->Event("SetPivotAndAdjustOffsets", &UiTransform2dBus::Events::SetPivotAndAdjustOffsets)
            ->Event("GetLocalWidth", &UiTransform2dBus::Events::GetLocalWidth)
            ->Event("SetLocalWidth", &UiTransform2dBus::Events::SetLocalWidth)
            ->Event("GetLocalHeight", &UiTransform2dBus::Events::GetLocalHeight)
            ->Event("SetLocalHeight", &UiTransform2dBus::Events::SetLocalHeight)
            ->VirtualProperty("LocalWidth", "GetLocalWidth", "SetLocalWidth")
            ->VirtualProperty("LocalHeight", "GetLocalHeight", "SetLocalHeight");

        behaviorContext->Class<UiTransform2dComponent>()
            ->RequestBus("UiTransformBus")
            ->RequestBus("UiTransform2dBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::Activate()
{
    UiTransformBus::Handler::BusConnect(m_entity->GetId());
    UiTransform2dBus::Handler::BusConnect(m_entity->GetId());
    UiAnimateEntityBus::Handler::BusConnect(m_entity->GetId());

    if (!m_elementComponent)
    {
        m_elementComponent = GetEntity()->FindComponent<UiElementComponent>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::Deactivate()
{
    UiTransformBus::Handler::BusDisconnect();
    UiTransform2dBus::Handler::BusDisconnect();
    UiAnimateEntityBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::IsControlledByParent() const
{
    bool isControlledByParent = false;

    if (IsFullyInitialized())
    {
        AZ::Entity* parentElement = GetElementComponent()->GetParent();
        if (parentElement)
        {
            UiLayoutBus::EventResult(isControlledByParent, parentElement->GetId(), &UiLayoutBus::Events::IsControllingChild, GetEntityId());
        }
    }
    else
    {
        EmitNotInitializedWarning();
    }

    return isControlledByParent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutFitterInterface::FitType UiTransform2dComponent::GetLayoutFitterType() const
{
    UiLayoutFitterInterface::FitType fitType = UiLayoutFitterInterface::FitType::None;
    UiLayoutFitterBus::EventResult(fitType, GetEntityId(), &UiLayoutFitterBus::Events::GetFitType);

    return fitType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::IsNotControlledByParent() const
{
    return !IsControlledByParent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiTransform2dComponent::GetAncestorWithSameDimensionScaleToDevice(ScaleToDeviceMode scaleToDeviceMode) const
{
    AZ::EntityId ancestor;

    if (IsFullyInitialized())
    {
        AZ::EntityId prevParent;
        AZ::EntityId parent;
        UiElementBus::EventResult(parent, GetEntityId(), &UiElementBus::Events::GetParentEntityId);
        while (parent.IsValid())
        {
            ScaleToDeviceMode parentScaleToDeviceMode = ScaleToDeviceMode::None;
            UiTransformBus::EventResult(parentScaleToDeviceMode, parent, &UiTransformBus::Events::GetScaleToDeviceMode);
            if (parentScaleToDeviceMode != ScaleToDeviceMode::None)
            {
                if ((DoesScaleToDeviceModeAffectX(scaleToDeviceMode) && DoesScaleToDeviceModeAffectX(parentScaleToDeviceMode))
                    || (DoesScaleToDeviceModeAffectY(scaleToDeviceMode) && DoesScaleToDeviceModeAffectY(parentScaleToDeviceMode)))
                {
                    ancestor = parent;
                    break;
                }
            }

            prevParent = parent;
            parent.SetInvalid();
            UiElementBus::EventResult(parent, prevParent, &UiElementBus::Events::GetParentEntityId);
        }
    }
    else
    {
        EmitNotInitializedWarning();
    }

    return ancestor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiTransform2dComponent::GetDescendantsWithSameDimensionScaleToDevice(ScaleToDeviceMode scaleToDeviceMode) const
{
    // Check if any descendants have their scale to device mode set in the same dimension
    auto HasSameDimensionScaleToDevice = [scaleToDeviceMode](const AZ::Entity* entity)
    {
        ScaleToDeviceMode descendantScaleToDeviceMode = ScaleToDeviceMode::None;
        UiTransformBus::EventResult(descendantScaleToDeviceMode, entity->GetId(), &UiTransformBus::Events::GetScaleToDeviceMode);

        return ((DoesScaleToDeviceModeAffectX(descendantScaleToDeviceMode) && DoesScaleToDeviceModeAffectX(scaleToDeviceMode))
            || (DoesScaleToDeviceModeAffectY(descendantScaleToDeviceMode) && DoesScaleToDeviceModeAffectY(scaleToDeviceMode)));
    };

    LyShine::EntityArray descendants;
    UiElementBus::Event(GetEntityId(), &UiElementBus::Events::FindDescendantElements, HasSameDimensionScaleToDevice, descendants);

    return descendants;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::AreAnchorsApartInSameScaleToDeviceDimension(ScaleToDeviceMode scaleToDeviceMode) const
{
    return ((m_anchors.m_left != m_anchors.m_right && DoesScaleToDeviceModeAffectX(scaleToDeviceMode))
        || (m_anchors.m_top != m_anchors.m_bottom && DoesScaleToDeviceModeAffectY(scaleToDeviceMode)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTransform2dComponent::GetScaleToDeviceModeWarningText() const
{
    AZStd::string warningText;

    if (m_scaleToDeviceMode != ScaleToDeviceMode::None)
    {
        // Check if anchors are apart in the same dimension as the scale to device mode
        if (AreAnchorsApartInSameScaleToDeviceDimension(m_scaleToDeviceMode))
        {
            warningText = AZStd::string::format("Element's anchors are not together");
        }

        if (warningText.empty())
        {
            // Check if any ancestors already have their scale to device mode set in the same dimension
            AZ::EntityId ancestor = GetAncestorWithSameDimensionScaleToDevice(m_scaleToDeviceMode);
            if (ancestor.IsValid())
            {
                warningText = AZStd::string::format("Element will be double scaled");
            }
        }

        if (warningText.empty())
        {
            // Check if any descendants have their scale to device mode set in the same dimension
            LyShine::EntityArray descendants = GetDescendantsWithSameDimensionScaleToDevice(m_scaleToDeviceMode);
            if (!descendants.empty())
            {
                warningText = AZStd::string::format("Descendants will be double scaled");
            }
        }
    }

    return warningText;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTransform2dComponent::GetScaleToDeviceModeWarningTooltipText() const
{
    AZStd::string warningTooltipText;

    if (m_scaleToDeviceMode != ScaleToDeviceMode::None)
    {
        // Check if anchors are apart in the same dimension as the scale to device mode
        if (AreAnchorsApartInSameScaleToDeviceDimension(m_scaleToDeviceMode))
        {
            warningTooltipText = AZStd::string::format("This scale to device mode affects the same dimension as the element's anchors that are not together. This will result in undesired behavior.");
        }

        if (warningTooltipText.empty())
        {
            // Check if any ancestors already have their scale to device mode set in the same dimension
            AZ::EntityId ancestor = GetAncestorWithSameDimensionScaleToDevice(m_scaleToDeviceMode);
            if (ancestor.IsValid())
            {
                const char* ancestorName = "";
                AZ::Entity* ancestorEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(ancestorEntity, &AZ::ComponentApplicationBus::Events::FindEntity, ancestor);
                if (ancestorEntity)
                {
                    ancestorName = ancestorEntity->GetName().c_str();
                }

                warningTooltipText = AZStd::string::format("This element has an ancestor called \"%s\" who's scale to device mode affects the same dimension. This will result in double scaling.", ancestorName);
            }
        }

        if (warningTooltipText.empty())
        {
            // Check if any descendants have their scale to device mode set in the same dimension
            LyShine::EntityArray descendants = GetDescendantsWithSameDimensionScaleToDevice(m_scaleToDeviceMode);
            if (!descendants.empty())
            {
                warningTooltipText = AZStd::string::format("This element has at least one descendant who's scale to device mode affects the same dimension. This will result in double scaling for those descendants.");
            }
        }
    }

    return warningTooltipText;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* UiTransform2dComponent::GetAnchorPropertyLabel() const
{
    const char* label = "Anchors";

    if (IsControlledByParent())
    {
        label = "Disabled";
    }

    return label;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiTransform2dComponent::GetCanvasEntityId()
{
    AZ::EntityId canvasEntityId;
    if (m_elementComponent)
    {
        m_elementComponent->GetCanvasEntityId();
    }
    else
    {
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    }

    return canvasEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiTransform2dComponent::GetCanvasComponent() const
{
    return GetElementComponent()->GetCanvasComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::OnTransformPropertyChanged()
{
    SetRecomputeFlags(UiTransformInterface::Recompute::TransformOnly);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::RecomputeTransformToViewportIfNeeded()
{
    // if we already computed the transform, don't recompute.
    if (!m_recomputeTransformToViewport)
    {
        return;
    }

    // first get the transform to canvas space
    RecomputeTransformToCanvasSpaceIfNeeded();

    // then get the transform from canvas to viewport space
    AZ::Matrix4x4 canvasToViewportMatrix;
    if (IsFullyInitialized())
    {
        canvasToViewportMatrix = GetCanvasComponent()->GetCanvasToViewportMatrix();
    }
    else
    {
        EmitNotInitializedWarning();
        canvasToViewportMatrix = AZ::Matrix4x4::CreateIdentity();
    }

    // add the transform to viewport space to the matrix
    m_transformToViewport = canvasToViewportMatrix * m_transformToCanvasSpace;

    m_recomputeTransformToViewport = false;

    UiTransformChangeNotificationBus::Event(GetEntityId(), &UiTransformChangeNotificationBus::Events::OnTransformToViewportChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::RecomputeTransformToCanvasSpaceIfNeeded()
{
    // if we already computed the transform, don't recompute.
    if (!m_recomputeTransformToCanvasSpace)
    {
        return;
    }

    // this takes a matrix and builds the concatenation of this elements rotate and scale about the pivot
    // with the transforms for all parent elements into one 3x4 matrix.
    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (parentTransformComponent)
    {
        parentTransformComponent->GetTransformToCanvasSpace(m_transformToCanvasSpace);

        AZ::Matrix4x4 transformToParent;
        if (HasScaleOrRotation())
        {
            GetLocalTransform(transformToParent);
        }
        else
        {
            transformToParent = AZ::Matrix4x4::CreateIdentity();
        }

        m_transformToCanvasSpace = m_transformToCanvasSpace * transformToParent;
    }
    else
    {
        m_transformToCanvasSpace = AZ::Matrix4x4::CreateIdentity();
    }

    m_recomputeTransformToCanvasSpace = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetScaleAdjustedForDevice()
{
    AZ::Vector2 scale = m_scale;

    if (m_scaleToDeviceMode != ScaleToDeviceMode::None)
    {
        if (IsFullyInitialized())
        {
            ApplyDeviceScale(scale);
        }
        else
        {
            EmitNotInitializedWarning();
        }
    }

    return scale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::CalculateCanvasSpaceRect()
{
    if (!m_recomputeCanvasSpaceRect)
    {
        return;
    }

    Rect rect;

    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (parentTransformComponent)
    {
        Rect parentRect;

        parentTransformComponent->GetCanvasSpaceRectNoScaleRotate(parentRect);

        AZ::Vector2 parentSize = parentRect.GetSize();

        float left = parentRect.left + parentSize.GetX() * m_anchors.m_left + m_offsets.m_left;
        float right = parentRect.left + parentSize.GetX() * m_anchors.m_right + m_offsets.m_right;
        float top = parentRect.top + parentSize.GetY() * m_anchors.m_top + m_offsets.m_top;
        float bottom = parentRect.top + parentSize.GetY() * m_anchors.m_bottom + m_offsets.m_bottom;

        rect.Set(left, right, top, bottom);
    }
    else
    {
        // this is the root element, its offset and anchors are ignored

        AZ::Vector2 size = UiCanvasComponent::s_defaultCanvasSize;
        if (IsFullyInitialized())
        {
            size = GetCanvasComponent()->GetCanvasSize();
        }
        else
        {
            EmitNotInitializedWarning();
        }

        rect.Set(0.0f, size.GetX(), 0.0f, size.GetY());
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

    m_rect = rect;
    if (!m_rectInitialized)
    {
        m_prevRect = m_rect;
        m_rectChangedByInitialization = true;
        m_rectInitialized = true;
    }
    else
    {
        // If the rect is being changed after it was initialized, but before the first
        // update, keep prev rect in sync with current rect. On a canvas space rect
        // change callback, prev rect and current rect can be used to determine whether
        // the canvas rect size has changed. Equal rects implies a change due to initialization
        if (m_rectChangedByInitialization)
        {
            m_prevRect = m_rect;
        }
    }
    m_recomputeCanvasSpaceRect = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTransform2dComponent::GetCanvasSpaceAnchorsCenterNoScaleRotate()
{
    // Get the position of the element's anchors in canvas space
    UiTransform2dComponent* parentTransformComponent = GetParentTransformComponent();
    if (!parentTransformComponent)
    {
        return AZ::Vector2(0.0f, 0.0f); // this is the root element
    }

    // Get parent's rect in canvas space
    UiTransformInterface::Rect parentRect;
    parentTransformComponent->GetCanvasSpaceRectNoScaleRotate(parentRect);

    // Get the anchor center in canvas space
    UiTransformInterface::Rect anchorRect;
    anchorRect.left = parentRect.left + m_anchors.m_left * parentRect.GetWidth();
    anchorRect.right = parentRect.left + m_anchors.m_right * parentRect.GetWidth();
    anchorRect.top = parentRect.top + m_anchors.m_top * parentRect.GetHeight();
    anchorRect.bottom = parentRect.top + m_anchors.m_bottom * parentRect.GetHeight();

    return anchorRect.GetCenter();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementComponent* UiTransform2dComponent::GetElementComponent() const
{
    AZ_Assert(m_elementComponent, "UiTransform2dComponent: m_elementComponent used when not initialized");
    return m_elementComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent* UiTransform2dComponent::GetParentTransformComponent() const
{
    if (IsFullyInitialized())
    {
        UiElementComponent* parentElementComponent = GetElementComponent()->GetParentElementComponent();
        if (parentElementComponent)
        {
            return parentElementComponent->GetTransform2dComponent();
        }
    }
    else
    {
        EmitNotInitializedWarning();
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransform2dComponent* UiTransform2dComponent::GetChildTransformComponent(int index) const
{
    if (IsFullyInitialized())
    {
        UiElementComponent* childElementComponent = GetElementComponent()->GetChildElementComponent(index);
        if (childElementComponent)
        {
            return childElementComponent->GetTransform2dComponent();
        }
    }
    else
    {
        EmitNotInitializedWarning();
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::IsFullyInitialized() const
{
    return (m_elementComponent && m_elementComponent->IsFullyInitialized());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::EmitNotInitializedWarning() const
{
    AZ_Warning("UI", false, "UiTransform2dComponent used before fully initialized, possibly on activate before FixupPostLoad was called on this element")
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTransform2dComponent::ApplyDeviceScale(AZ::Vector2& scale)
{
    AZ::Vector2 deviceScale = GetCanvasComponent()->GetDeviceScale();

    switch (m_scaleToDeviceMode)
    {
    case ScaleToDeviceMode::UniformScaleToFit:
    {
        float uniformScale = AZStd::min(deviceScale.GetX(), deviceScale.GetY());
        scale *= uniformScale;
        break;
    }
    case ScaleToDeviceMode::UniformScaleToFill:
    {
        float uniformScale = AZStd::max(deviceScale.GetX(), deviceScale.GetY());
        scale *= uniformScale;
        break;
    }
    case ScaleToDeviceMode::UniformScaleToFitX:
    {
        float uniformScale = deviceScale.GetX();
        scale *= uniformScale;
        break;
    }
    case ScaleToDeviceMode::UniformScaleToFitY:
    {
        float uniformScale = deviceScale.GetY();
        scale *= uniformScale;
        break;
    }
    case ScaleToDeviceMode::NonUniformScale:
        scale *= deviceScale;
        break;
    case ScaleToDeviceMode::ScaleXOnly:
        scale.SetX(scale.GetX() * deviceScale.GetX());
        break;
    case ScaleToDeviceMode::ScaleYOnly:
        scale.SetY(scale.GetY() * deviceScale.GetY());
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to convert Vec2 to AZ::Vector2
    if (classElement.GetVersion() <= 1)
    {
        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "Pivot"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "Scale"))
        {
            return false;
        }
    }

    // conversion from version 2:
    // - Need to convert ScaleToDevice from a bool to an enum
    if (classElement.GetVersion() <= 2)
    {
        if (!ConvertScaleToDeviceFromBoolToEnum(context, classElement))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::DoesScaleToDeviceModeAffectX(ScaleToDeviceMode scaleToDeviceMode)
{
    return (scaleToDeviceMode != ScaleToDeviceMode::None && scaleToDeviceMode != ScaleToDeviceMode::ScaleYOnly);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTransform2dComponent::DoesScaleToDeviceModeAffectY(ScaleToDeviceMode scaleToDeviceMode)
{
    return (scaleToDeviceMode != ScaleToDeviceMode::None && scaleToDeviceMode != ScaleToDeviceMode::ScaleXOnly);
}

#include "Tests/internal/test_UiTransform2dComponent.cpp"
