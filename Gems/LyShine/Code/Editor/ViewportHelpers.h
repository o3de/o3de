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
#pragma once

namespace ViewportHelpers
{
    //-------------------------------------------------------------------------------

    const AZ::Color backgroundColorLight(0.85f, 0.85f, 0.85f, 1.0f);
    const AZ::Color backgroundColorDark(0.133f, 0.137f, 0.149f, 1.0f);      // #222236, RGBA: 34, 35, 38, 255
    const AZ::Color selectedColor(1.000f, 1.000f, 1.000f, 1.0f);            // #FFFFFF, RGBA: 255, 255, 255, 255
    const AZ::Color unselectedColor(0.800f, 0.800f, 0.800f, 0.500f);        // #CCCCCC, RGBA: 204, 204, 204, 128
    const AZ::Color highlightColor(1.000f, 0.600f, 0.000f, 1.0f);           // #FF9900, RGBA: 255, 153, 0, 255
    const AZ::Color anchorColor(0.192f, 0.565f, 0.933f, 1.0f);              // #3190EE, RGBA: 49, 144, 238, 255
    const AZ::Color anchorColorDisabled(0.85f, 0.85f, 0.85f, 0.5f);
    const AZ::Color pivotColor(anchorColor);
    const AZ::Color xColor(1.00f, 0.00f, 0.00f, 1.0f);
    const AZ::Color yColor(0.00f, 1.00f, 0.00f, 1.0f);
    const AZ::Color zColor(0.10f, 0.30f, 1.00f, 1.0f);

    //-------------------------------------------------------------------------------

    // Determines whether the element is being controlled by a layout component on its parent
    bool IsControlledByLayout(const AZ::Entity* element);

    // Determines whether the element is being horizontally fit by a LayoutFitter
    bool IsHorizontallyFit(const AZ::Entity* element);

    // Determines whether the element is being vertically fit by a LayoutFitter
    bool IsVerticallyFit(const AZ::Entity* element);

    // Returns a perpendicular angle between -180 and 180 degrees.
    float GetPerpendicularAngle(float angle);

    // Assumes that the provided angle is between -180 and 180 degrees.
    // Returns a sizing cursor that is perpendicular to a line at that angle,
    // where a 0 degree line points to the right, and a 90 degree line points down.
    Qt::CursorShape GetSizingCursor(float angle);

    void TransformIconScale(AZ::Vector2& iconSize, const AZ::Matrix4x4& transform);

    AZ::Vector2 ComputeAnchorPoint(AZ::Vector2 rectTopLeft, AZ::Vector2 rectSize, float anchorX, float anchorY);

    // Determine whether the point is inside this region of the icon.
    // leftPart, rightPart, topPart, and bottomPart are [-0.5, 0.5], where 0.0 is
    // the center of the icon. They describe the portion of the icon to check.
    bool IsPointInIconRect(AZ::Vector2 point, AZ::Vector2 iconCenter, AZ::Vector2 iconSize, float leftPart, float rightPart, float topPart, float bottomPart);

    // Helper function to get the target points on the left and right sides of the rect to end anchor lines on.
    // If the given y value is within the y bounds of the rect, then the targets are as that y value,
    // otherwise they are at the the y value of the nearest edge.
    void GetHorizTargetPoints(const UiTransformInterface::RectPoints& elemRect, float y, AZ::Vector2& leftTarget, AZ::Vector2& rightTarget);

    // Helper function to get the points on the top and bottom sides of the rect to end anchor lines on.
    // If the given x value is within the x bounds of the rect, then the targets are as that x value,
    // otherwise they are at the the x value of the nearest edge.
    void GetVerticalTargetPoints(const UiTransformInterface::RectPoints& elemRect, float x, AZ::Vector2& topTarget, AZ::Vector2& bottomTarget);

    //-------------------------------------------------------------------------------

    //! Indicates which edges of an element are under consideration
    struct ElementEdges
    {
        ElementEdges()
            : m_left(false)
            , m_top(false)
            , m_right(false)
            , m_bottom(false) {}

        void SetAll(bool state) { m_left = m_right = m_top = m_bottom = state; }

        bool Any() const { return m_left || m_right || m_top || m_bottom; }
        bool None() const { return !Any(); }
        bool BothHorizontal() const { return m_left && m_right; }
        bool BothVertical() const { return m_top && m_bottom; }
        bool TopLeft() const { return m_top && m_left; }
        bool TopRight() const { return m_top && m_right; }
        bool BottomRight() const { return m_bottom && m_right; }
        bool BottomLeft() const { return m_bottom && m_left; }

        bool m_left;
        bool m_right;
        bool m_top;
        bool m_bottom;
    };

    //! Indicates which anchors of an element are under consideration
    struct SelectedAnchors
    {
        SelectedAnchors()
            : m_left(false)
            , m_top(false)
            , m_right(false)
            , m_bottom(false) {}

        SelectedAnchors(bool left, bool top, bool right, bool bottom)
            : m_left(left)
            , m_top(top)
            , m_right(right)
            , m_bottom(bottom) {}

        void SetAll(bool state) { m_left = m_right = m_top = m_bottom = state; }

        bool Any() const { return m_left || m_right || m_top || m_bottom; }
        bool All() const { return m_left && m_right && m_top && m_bottom; }
        bool TopLeft() const { return m_top && m_left; }
        bool TopRight() const { return m_top && m_right; }
        bool BottomRight() const { return m_bottom && m_right; }
        bool BottomLeft() const { return m_bottom && m_left; }

        bool m_left;
        bool m_right;
        bool m_top;
        bool m_bottom;
    };

    //! Indicates which parts of a transform gizmo are under consideration
    struct GizmoParts
    {
        GizmoParts()
            : m_top(false)
            , m_right(false) {}

        void SetBoth(bool state) { m_top = m_right = state; }

        bool Both() const { return m_right && m_top; }
        bool Single() const { return m_right ^ m_top; }

        bool m_top;
        bool m_right;
    };

    UiTransform2dInterface::Offsets MoveGrabbedEdges(const UiTransform2dInterface::Offsets& offset,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        const AZ::Vector2& v);

    UiTransform2dInterface::Anchors MoveGrabbedAnchor(const UiTransform2dInterface::Anchors& anchor,
        const ViewportHelpers::SelectedAnchors& grabbedAnchors, bool keepTogetherHorizontally, bool keepTogetherVertically,
        const AZ::Vector2& v);

    void MoveGrabbedEdges(UiTransformInterface::RectPoints& points,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        const AZ::Vector2& topEdge,
        const AZ::Vector2& leftEdge);

    //-------------------------------------------------------------------------------

    const char* InteractionModeToString(int mode);
    const char* CoordinateSystemToString(int s);
    const char* InteractionTypeToString(int type);

    void DrawRotationValue(const AZ::Entity* element,
        ViewportInteraction* viewportInteraction,
        const ViewportPivot* viewportPivot,
        Draw2dHelper& draw2d);

    void DrawCursorText(const AZStd::string& textLabel,
        Draw2dHelper& draw2d,
        const ViewportWidget* viewport);

    //-------------------------------------------------------------------------------
}   // namespace ViewportHelpers
