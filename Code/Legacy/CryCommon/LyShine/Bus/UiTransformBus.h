/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// The UiTransformInterface provides an abstract bus interface for UI components that define the
// position and size of a UI element.
class UiTransformInterface
    : public AZ::ComponentBus
{
public: // types

    //! Struct that stores the 4 points of a (transformed) rectangle and provides access either as an array or via access functions
    struct RectPoints
    {
        enum Corner     // can be used as an index into RectPoints pt array but access via member functions preferred
        {
            Corner_TopLeft,
            Corner_TopRight,
            Corner_BottomRight,
            Corner_BottomLeft,

            Corner_Count
        };

        AZ::Vector2    pt[Corner_Count];  // in clockwise order: top left, top right, bottom right, bottom left

        RectPoints();
        RectPoints(float left, float right, float top, float bottom);

        AZ::Vector2& TopLeft();
        const AZ::Vector2& TopLeft() const;

        AZ::Vector2& TopRight();
        const AZ::Vector2& TopRight() const;

        AZ::Vector2& BottomRight();
        const AZ::Vector2& BottomRight() const;

        AZ::Vector2& BottomLeft();
        const AZ::Vector2& BottomLeft() const;

        AZ::Vector2 GetCenter() const;

        AZ::Vector2 GetAxisAlignedSize() const;
        AZ::Vector2 GetAxisAlignedTopLeft() const;
        AZ::Vector2 GetAxisAlignedTopRight() const;
        AZ::Vector2 GetAxisAlignedBottomRight() const;
        AZ::Vector2 GetAxisAlignedBottomLeft() const;
        void SetAxisAligned(float left, float right, float top, float bottom);

        RectPoints Transform(AZ::Matrix4x4& transform) const;
        RectPoints operator-(const RectPoints& rhs) const;
    };

    using RectPointsArray = AZStd::vector < UiTransformInterface::RectPoints >;

    //! Struct that stores the bounds of an axis-aligned rectangle
    struct Rect
    {
        float left;
        float right;
        float top;
        float bottom;

        void Set(float l, float r, float t, float b);

        float GetWidth() const;
        float GetHeight() const;

        float GetCenterX();
        float GetCenterY();

        AZ::Vector2 GetSize() const;
        AZ::Vector2 GetCenter();

        void MoveBy(AZ::Vector2 offset);

        RectPoints GetPoints();

        bool operator==(const Rect& rhs) const;

        bool operator!=(const Rect& rhs) const;
    };

    //! Enum used as a parameter to SetRecomputeFlags
    enum class Recompute
    {
        RectOnly,               //!< Only the rect (offsets or anchors for example) changed (this may affect transform if local scale or rotation)
        TransformOnly,          //!< Only the transform changed (canvas and viewport transforms must be recomputed)
        ViewportTransformOnly,  //!< Only the viewport transform changed (viewport transform must be recomputed)
        RectAndTransform        //!< Both rect and transform changed (all cached data must be recomputed)
    };

    //! Enum used as a parameter to SetScaleToDeviceMode
    //! The value determines how an element is scaled when the canvas reference size and actual size are different
    //! The comments below reference the canvas's "deviceScale". The deviceScale is target (actual) canvas size divided the reference canvas size
    enum class ScaleToDeviceMode
    {
        None,               //!< Default, this element is not affected by device resolution changes
        UniformScaleToFit,  //!< Apply a uniform scale which is the minimum of deviceScale.x and deviceScale.y
        UniformScaleToFill, //!< Apply a uniform scale which is the maximum of deviceScale.x and deviceScale.y
        UniformScaleToFitX, //!< Apply a uniform scale of deviceScale.x
        UniformScaleToFitY, //!< Apply a uniform scale of deviceScale.y
        NonUniformScale,    //!< Apply a non-uniform scale which is simply deviceScale
        ScaleXOnly,         //!< Scale the element only in the X dimension by deviceScale.x
        ScaleYOnly          //!< Scale the element only in the Y dimension by deviceScale.y
    };

public: // member functions

    virtual ~UiTransformInterface() {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods to get and set the properties of the transform component
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get the rotation about the Z axis
    virtual float GetZRotation() = 0;

    //! Set the rotation about the Z axis
    virtual void SetZRotation(float rotation) = 0;

    //! Get the scale
    virtual AZ::Vector2 GetScale() = 0;

    //! Set the scale
    virtual void SetScale(AZ::Vector2 scale) = 0;

    //! Get the scale X
    virtual float GetScaleX() = 0;

    //! Set the scale X
    virtual void SetScaleX(float scale) = 0;

    //! Get the scale Y
    virtual float GetScaleY() = 0;

    //! Set the scale Y
    virtual void SetScaleY(float scale) = 0;

    //! Get the pivot point for the element
    virtual AZ::Vector2 GetPivot() = 0;

    //! Set the pivot point for the element
    virtual void SetPivot(AZ::Vector2 pivot) = 0;

    //! Get the pivot point for the element
    virtual float GetPivotX() = 0;

    //! Set the pivot point for the element
    virtual void SetPivotX(float pivot) = 0;

    //! Get the pivot point for the element
    virtual float GetPivotY() = 0;

    //! Set the pivot point for the element
    virtual void SetPivotY(float pivot) = 0;

    //! Get how the element and all its children will be scaled to allow for the difference
    //! between the authored canvas size and the actual viewport size.
    virtual ScaleToDeviceMode GetScaleToDeviceMode() = 0;

    //! Set how the element and all its children will be scaled to allow for the difference
    //! between the authored canvas size and the actual viewport size.
    virtual void SetScaleToDeviceMode(ScaleToDeviceMode scaleToDeviceMode) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods to get data in viewport space
    //
    // Viewport space is a 1-1 mapping to whatever viewport the UI canvas is being rendered to.
    // A position in viewport space is an offset in pixels from the top left of the viewport.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get the four points defining the rectangle of this element, in viewport space pixel coords
    virtual void GetViewportSpacePoints(RectPoints& points) = 0;

    //! Get the pivot for this element in viewport space
    virtual AZ::Vector2 GetViewportSpacePivot() = 0;

    //! Get the transform matrix to transform from canvas (noScaleRotate) space to viewport space
    virtual void GetTransformToViewport(AZ::Matrix4x4& mat) = 0;

    //! Get the transform matrix to transform from viewport space to canvas (noScaleRotate) space
    virtual void GetTransformFromViewport(AZ::Matrix4x4& mat) = 0;

    //! Apply the "to viewport" transform to the given points
    virtual void RotateAndScalePoints(RectPoints& points) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods to get data in canvas space
    //
    // Often canvas space and viewport space are the same thing. For example if a canvas is being
    // displayed full screen.
    // However, in other cases they are different. For example in the UI editor when Zoom and Pan allow
    // the canvas to be moved around in the viewport. In that case an offset of 1 does not mean 1 pixel
    // in the viewport. Canvas space is defined by the canvas size stored in the canvas.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get the four points defining the rectangle of this element, in canvas space coords
    virtual void GetCanvasSpacePoints(RectPoints& points) = 0;

    //! Get the pivot for this element in canvas space
    virtual AZ::Vector2 GetCanvasSpacePivot() = 0;

    //! Get the transform matrix to transform from canvasNoScaleRotate space to canvas space
    virtual void GetTransformToCanvasSpace(AZ::Matrix4x4& mat) = 0;
    
    //! Get the transform matrix to transform from canvas space to canvasNoScaleRotate space
    virtual void GetTransformFromCanvasSpace(AZ::Matrix4x4& mat) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods to get data in unrotated and unscaled canvas space (CanvasNoScaleRotate space)
    //
    // CanvasNoScaleRotate space is like canvas space but without any of the rotation or scaling in
    // any of the elments applied. So if none of the elements in the canvas have scale or rotation
    // the two are the same.
    //
    // CanvasNoScaleRotate space is like local space for an element but it does include all of the
    // parent's anchor and offset calculations so it is not really local space.
    //
    // This is a useful space to do calculations in because all elements are axis aligned and their
    // rectangle can be represented by a Rect struct rather requiring a RectPoints struct.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get the axis-aligned rect for the element without accounting for rotation or scale
    virtual void GetCanvasSpaceRectNoScaleRotate(Rect& rect) = 0;

    //! Get the rect points for the element without accounting for rotation or scale
    virtual void GetCanvasSpacePointsNoScaleRotate(RectPoints& points) = 0;

    //! Get the size for the element without accounting for rotation or scale
    virtual AZ::Vector2 GetCanvasSpaceSizeNoScaleRotate() = 0;

    //! Get the pivot for the element without accounting for rotation or scale
    virtual AZ::Vector2 GetCanvasSpacePivotNoScaleRotate() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods to get data in/about local space
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get the transform matrix to apply this element's rotation and scale about pivot
    virtual void GetLocalTransform(AZ::Matrix4x4& mat) = 0;

    //! Get the transform matrix to apply the inverse of this element's rotation and scale about pivot
    virtual void GetLocalInverseTransform(AZ::Matrix4x4& mat) = 0;

    //! Test whether this transform component has any scale or rotation
    virtual bool HasScaleOrRotation() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods to get/set the element's position
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get the position for this element in viewport space (same as GetViewportSpacePivot)
    virtual AZ::Vector2 GetViewportPosition() = 0;

    //! Set the position for this element in viewport space
    virtual void SetViewportPosition(const AZ::Vector2& position) = 0;

    //! Get the position for this element in canvas space (same as GetCanvasSpacePivot)
    virtual AZ::Vector2 GetCanvasPosition() = 0;

    //! Set the position for this element in canvas space
    virtual void SetCanvasPosition(const AZ::Vector2& position) = 0;

    //! Get the position for this element relative to the center of the element's anchors
    virtual AZ::Vector2 GetLocalPosition() = 0;

    //! Set the position for this element relative to the center of the element's anchors
    virtual void SetLocalPosition(const AZ::Vector2& position) = 0;

    //! Get the X position for this element relative to the center of the element's anchors
    virtual float GetLocalPositionX() = 0;

    //! Set the X position for this element relative to the center of the element's anchors
    virtual void SetLocalPositionX(float position) = 0;

    //! Get the Y position for this element relative to the center of the element's anchors
    virtual float GetLocalPositionY() = 0;

    //! Set the Y position for this element relative to the center of the element's anchors
    virtual void SetLocalPositionY(float position) = 0;

    //! Move this element in viewport space
    virtual void MoveViewportPositionBy(const AZ::Vector2& offset) = 0;

    //! Move this element in canvas space
    virtual void MoveCanvasPositionBy(const AZ::Vector2& offset) = 0;

    //! Move this element relative to the center of the element's anchors
    virtual void MoveLocalPositionBy(const AZ::Vector2& offset) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Query functions
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Test if the given point (in viewport space) is in the rectangle of this element
    virtual bool IsPointInRect(AZ::Vector2 point) = 0;

    //! Test if the given rect (in viewport space) is in the rectangle of this element
    virtual bool BoundsAreOverlappingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Optimization and caching
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Set the required dirty flags for the cached transforms and rect on this element and all its children
    virtual void SetRecomputeFlags(Recompute recompute) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Canvas space rect change
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //! Get whether the canvas space rect has changed since the last call to NotifyAndResetCanvasSpaceRectChange.
    //! May trigger a recompute of the rect if the recompute flag is dirty
    virtual bool HasCanvasSpaceRectChanged() = 0;

    //! Get whether the canvas space size has changed since the last call to NotifyAndResetCanvasSpaceRectChange.
    //! May trigger a recompute of the rect if the recompute flag is dirty
    virtual bool HasCanvasSpaceSizeChanged() = 0;

    //! Get whether the canvas space rect was changed due to initialization
    virtual bool HasCanvasSpaceRectChangedByInitialization() = 0;

    //! Send notification of canvas space rect change and reset to unchanged
    virtual void NotifyAndResetCanvasSpaceRectChange() = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiTransformInterface> UiTransformBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement
class UiTransformChangeNotification
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiTransformChangeNotification(){}

    //! Called when an entity's transform (canvas space) has been modified
    virtual void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) = 0;

    //! Called when an entity's transform (viewport space) has been modified
    virtual void OnTransformToViewportChanged() {}
};

typedef AZ::EBus<UiTransformChangeNotification> UiTransformChangeNotificationBus;


////////////////////////////////////////////////////////////////////////////////////////////////////
// RectPoints inline methods
////////////////////////////////////////////////////////////////////////////////////////////////////

inline UiTransformInterface::RectPoints::RectPoints()
{
}

inline UiTransformInterface::RectPoints::RectPoints(float left, float right, float top, float bottom)
{
    SetAxisAligned(left, right, top, bottom);
}

inline AZ::Vector2& UiTransformInterface::RectPoints::TopLeft()
{
    return pt[Corner_TopLeft];
}

inline const AZ::Vector2& UiTransformInterface::RectPoints::TopLeft() const
{
    return pt[Corner_TopLeft];
}

inline AZ::Vector2& UiTransformInterface::RectPoints::TopRight()
{
    return pt[Corner_TopRight];
}

inline const AZ::Vector2& UiTransformInterface::RectPoints::TopRight() const
{
    return pt[Corner_TopRight];
}

inline AZ::Vector2& UiTransformInterface::RectPoints::BottomRight()
{
    return pt[Corner_BottomRight];
}

inline const AZ::Vector2& UiTransformInterface::RectPoints::BottomRight() const
{
    return pt[Corner_BottomRight];
}

inline AZ::Vector2& UiTransformInterface::RectPoints::BottomLeft()
{
    return pt[Corner_BottomLeft];
}

inline const AZ::Vector2& UiTransformInterface::RectPoints::BottomLeft() const
{
    return pt[Corner_BottomLeft];
}

inline AZ::Vector2 UiTransformInterface::RectPoints::GetCenter() const
{
    return ((GetAxisAlignedTopLeft() + GetAxisAlignedBottomRight()) * 0.5f);
}

inline AZ::Vector2 UiTransformInterface::RectPoints::GetAxisAlignedSize() const
{
    return AZ::Vector2(BottomRight().GetX() - TopLeft().GetX(), BottomRight().GetY() - TopLeft().GetY());
}

inline AZ::Vector2 UiTransformInterface::RectPoints::GetAxisAlignedTopLeft() const
{
    return AZ::Vector2(AZStd::min(AZStd::min(AZStd::min(TopLeft().GetX(), TopRight().GetX()), BottomRight().GetX()), BottomLeft().GetX()),
        AZStd::min(AZStd::min(AZStd::min(TopLeft().GetY(), TopRight().GetY()), BottomRight().GetY()), BottomLeft().GetY()));
}

inline AZ::Vector2 UiTransformInterface::RectPoints::GetAxisAlignedTopRight() const
{
    return AZ::Vector2(AZStd::max(AZStd::max(AZStd::max(TopLeft().GetX(), TopRight().GetX()), BottomRight().GetX()), BottomLeft().GetX()),
        AZStd::min(AZStd::min(AZStd::min(TopLeft().GetY(), TopRight().GetY()), BottomRight().GetY()), BottomLeft().GetY()));
}

inline AZ::Vector2 UiTransformInterface::RectPoints::GetAxisAlignedBottomRight() const
{
    return AZ::Vector2(AZStd::max(AZStd::max(AZStd::max(TopLeft().GetX(), TopRight().GetX()), BottomRight().GetX()), BottomLeft().GetX()),
        AZStd::max(AZStd::max(AZStd::max(TopLeft().GetY(), TopRight().GetY()), BottomRight().GetY()), BottomLeft().GetY()));
}

inline AZ::Vector2 UiTransformInterface::RectPoints::GetAxisAlignedBottomLeft() const
{
    return AZ::Vector2(AZStd::min(AZStd::min(AZStd::min(TopLeft().GetX(), TopRight().GetX()), BottomRight().GetX()), BottomLeft().GetX()),
        AZStd::max(AZStd::max(AZStd::max(TopLeft().GetY(), TopRight().GetY()), BottomRight().GetY()), BottomLeft().GetY()));
}

inline void UiTransformInterface::RectPoints::SetAxisAligned(float left, float right, float top, float bottom)
{
    pt[Corner_TopLeft] = AZ::Vector2(left, top);
    pt[Corner_TopRight] = AZ::Vector2(right, top);
    pt[Corner_BottomRight] = AZ::Vector2(right, bottom);
    pt[Corner_BottomLeft] = AZ::Vector2(left, bottom);
}

inline UiTransformInterface::RectPoints UiTransformInterface::RectPoints::Transform(AZ::Matrix4x4& transform) const
{
    RectPoints result;
    for (int i = 0; i < Corner_Count; ++i)
    {
        AZ::Vector3 point3(pt[i].GetX(), pt[i].GetY(), 0.0f);
        point3 = transform * point3;
        result.pt[i] = AZ::Vector2(point3.GetX(), point3.GetY());
    }
    return result;
}

inline UiTransformInterface::RectPoints UiTransformInterface::RectPoints::operator-(const RectPoints& rhs) const
{
    RectPoints result(*this);
    for (int i = 0; i < Corner_Count; ++i)
    {
        result.pt[i] -= rhs.pt[i];
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Rect inline methods
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void UiTransformInterface::Rect::Set(float l, float r, float t, float b)
{
    left = l;
    right = r;
    top = t;
    bottom = b;
}

inline float UiTransformInterface::Rect::GetWidth() const
{
    return right - left;
}

inline float UiTransformInterface::Rect::GetHeight() const
{
    return bottom - top;
}

inline float UiTransformInterface::Rect::GetCenterX()
{
    return (left + right) * 0.5f;
}

inline float UiTransformInterface::Rect::GetCenterY()
{
    return (top + bottom) * 0.5f;
}

inline AZ::Vector2 UiTransformInterface::Rect::GetSize() const
{
    return AZ::Vector2(GetWidth(), GetHeight());
}

inline AZ::Vector2 UiTransformInterface::Rect::GetCenter()
{
    return AZ::Vector2(GetCenterX(), GetCenterY());
}

inline void UiTransformInterface::Rect::MoveBy(AZ::Vector2 offset)
{
    left += offset.GetX();
    right += offset.GetX();
    top += offset.GetY();
    bottom += offset.GetY();
}

inline UiTransformInterface::RectPoints UiTransformInterface::Rect::GetPoints()
{
    return UiTransformInterface::RectPoints(left, right, top, bottom);
}

inline bool UiTransformInterface::Rect::operator==(const Rect& rhs) const
{
    return left == rhs.left
            && right == rhs.right
            && top == rhs.top
            && bottom == rhs.bottom;
}

inline bool UiTransformInterface::Rect::operator!=(const Rect& rhs) const
{
    return !(*this == rhs);
}
