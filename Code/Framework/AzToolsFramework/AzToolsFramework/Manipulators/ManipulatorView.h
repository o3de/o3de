/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Manipulators/BaseManipulator.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>

namespace AzToolsFramework
{
    class PlanarManipulator;
    class LinearManipulator;
    class AngularManipulator;
    class PaintBrushManipulator;
    class LineSegmentSelectionManipulator;
    class SplineSelectionManipulator;

    using DecideColorFn =
        AZStd::function<AZ::Color(const ViewportInteraction::MouseInteraction&, bool mouseOver, const AZ::Color& defaultColor)>;

    extern const float g_defaultManipulatorSphereRadius;

    //! State of an individual manipulator.
    struct ManipulatorState
    {
        AZ::Transform m_worldFromLocal;
        AZ::Vector3 m_nonUniformScale;
        AZ::Vector3 m_localPosition;
        bool m_mouseOver;

        //! Transforms a point, taking non-uniform scale into account.
        AZ::Vector3 TransformPoint(const AZ::Vector3& point) const;

        //! Rotates a direction into the space of the manipulator and normalizes it.
        //! Non-uniform scaling and translation are not applied.
        AZ::Vector3 TransformDirectionNoScaling(const AZ::Vector3& direction) const;
    };

    //! The base interface for the visual representation of manipulators.
    //! The View represents the appearance and bounds of the manipulator for
    //! the user to interact with. Any manipulator can have any view (some may
    //! be more appropriate than others in certain cases).
    class ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorView, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorView, "{7529E3E9-39B3-4D15-899A-FA13770113B2}")

        ManipulatorView();
        explicit ManipulatorView(bool screenSizeFixed);
        virtual ~ManipulatorView();
        ManipulatorView(ManipulatorView&&) = default;
        ManipulatorView& operator=(ManipulatorView&&) = default;

        void SetBoundDirty(ManipulatorManagerId managerId);
        void RefreshBound(ManipulatorManagerId managerId, ManipulatorId manipulatorId, const Picking::BoundRequestShapeBase& bound);
        void Invalidate(ManipulatorManagerId managerId);

        virtual void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) = 0;

        bool ScreenSizeFixed() const
        {
            return m_screenSizeFixed;
        }

    protected:
        AZ::Color m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor; //!< What color should the manipulator
                                                                               //!< be when the mouse is hovering over it.
        //! Scale the manipulator based on the distance
        //! from the camera if m_screenSizeFixed is true.
        float ManipulatorViewScaleMultiplier(const AZ::Vector3& worldPosition, const AzFramework::CameraState& cameraState) const;

        //! Wrap the logic for updating a bound.
        //! Should be called at the end of the Draw function once a concrete BoundRequestShape has
        //! been created to use for dimensions for rendering.
        void RefreshBoundInternal(ManipulatorManagerId managerId, ManipulatorId manipulatorId, const Picking::BoundRequestShapeBase& bound);

    private:
        Picking::RegisteredBoundId m_boundId = Picking::InvalidBoundId; //!< Used for hit detection.
        ManipulatorManagerId m_managerId = InvalidManipulatorManagerId; //! The manipulator manager this view has been registered with.
        bool m_screenSizeFixed = true; //!< Should manipulator size be adjusted based on camera distance.
        bool m_boundDirty = true; //!< Do the bounds need to be recalculated.
    };

    // A collection of views (a manipulator may have 1 - * views)
    using ManipulatorViews = AZStd::vector<AZStd::shared_ptr<ManipulatorView>>;

    //! Display a quad representing part of a plane, rendered as 4 lines.
    class ManipulatorViewQuad : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewQuad, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewQuad, "{D85E1B45-495E-4755-BCF2-6AE45F8BB2B0}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_axis1 = AZ::Vector3(1.0f, 0.0f, 0.0f);
        AZ::Vector3 m_axis2 = AZ::Vector3(0.0f, 1.0f, 0.0f);
        AZ::Vector3 m_offset = AZ::Vector3::CreateZero();
        AZ::Color m_axis1Color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        AZ::Color m_axis2Color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_size = 0.06f; //!< size to render and do mouse ray intersection tests against.

    private:
        AZ::Vector3 m_cameraCorrectedAxis1; //!< First axis of quad (should be orthogonal to second axis).
        AZ::Vector3 m_cameraCorrectedAxis2; //!< Second axis of quad (should be orthogonal to first axis).
        AZ::Vector3 m_cameraCorrectedOffsetAxis1; //!< Offset along first axis (parallel with first axis).
        AZ::Vector3 m_cameraCorrectedOffsetAxis2; //!< Offset along second axis (parallel with second axis).
    };

    //! A screen aligned quad, centered at the position of the manipulator, display filled.
    class ManipulatorViewQuadBillboard : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewQuadBillboard, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewQuadBillboard, "{C205E967-E8C6-4A73-A31B-41EE5529B15B}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_size = 0.005f; //!< size to render and do mouse ray intersection tests against.
    };

    //! Displays a debug style line starting from the manipulator's transform,
    //! width determines the click area.
    class ManipulatorViewLine : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewLine, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewLine, "{831EEF66-4A5C-450C-B152-EA4A0BC8A272}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_axis;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_length = 0.0f;
        float m_width = 0.0f;

    private:
        AZ::Vector3 m_cameraCorrectedAxis;
    };

    //! Variant of ManipulatorViewLine which instead of using an axis, provides begin and end
    //! points for the line. Used for selection when inserting points along a line.
    class ManipulatorViewLineSelect : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewLineSelect, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewLineSelect, "{BF26A947-91F8-4595-9A5B-481876EB2C48}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_localStart;
        AZ::Vector3 m_localEnd;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_width = 0.0f;
    };

    //! Displays a filled cone along the specified axis, offset is local translation from
    //! the manipulator transform (often used in conjunction with other views to build
    //! aggregate views such as arrows - e.g. a line and cone).
    class ManipulatorViewCone : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewCone, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewCone, "{BF042887-1F51-4FD8-8CA5-4A649B4AF356}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_offset;
        AZ::Vector3 m_axis;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_length = 0.0f;
        float m_radius = 0.0f;

    private:
        AZ::Vector3 m_cameraCorrectedAxis;
        AZ::Vector3 m_cameraCorrectedOffset;
        bool m_shouldCorrect = false;
    };

    //! Displays a filled box, offset is local translation from the manipulator
    //! transform, box is often used in conjunction with other views, orientation allows
    //! the box to be orientated separately from the manipulator transform.
    class ManipulatorViewBox : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewBox, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewBox, "{2D082201-7878-4C1B-A3DD-7A629E5AD598}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_offset;
        AZ::Quaternion m_orientation;
        AZ::Vector3 m_halfExtents;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);

    private:
        AZ::Vector3 m_cameraCorrectedOffset;
    };

    //! Displays a filled cylinder along the axis provided.
    class ManipulatorViewCylinder : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewCylinder, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewCylinder, "{9B8E5EF4-0F85-4CD0-A5FF-3C7097DF58AC}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_axis;
        float m_length = 0.0f;
        float m_radius = 0.0f;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);

    private:
        AZ::Vector3 m_cameraCorrectedAxis;
    };

    //! Displays a filled sphere at the transform of the manipulator, often used as
    //! a selection manipulator. DecideColorFn allows more complex logic to be used
    //! to decide the color of the manipulator (based on hover state etc.)
    class ManipulatorViewSphere : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewSphere, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewSphere, "{324D8329-6E7B-4A5D-AC8A-8C0E1C984E38}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        float m_radius = 0.0f;
        DecideColorFn m_decideColorFn;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        bool m_depthTest = false;
    };

    //! Displays a wire circle that's projected and rotated into world space (useful for paint brush manipulators).
    class ManipulatorViewProjectedCircle : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewProjectedCircle, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewProjectedCircle, "{2F6C918C-3B2D-41A1-871A-B143463324B1}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetRadius(float radius);

        AZ::Vector3 m_axis = AZ::Vector3::CreateAxisZ();
        float m_width = 0.5f;
        float m_radius = 2.0f;
        AZ::Color m_color = AZ::Colors::Red;
    };

    //! Displays a wire circle. DrawCircleFunc can be used to either draw a full
    //! circle or a half dotted circle where the part of the circle facing away
    //! from the camera is dotted (useful for angular/rotation manipulators).
    class ManipulatorViewCircle : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewCircle, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewCircle, "{26563A03-3E48-49EB-9DCF-30EE4F567FCD}", ManipulatorView)

        using DrawCircleFunc = void (*)(AzFramework::DebugDisplayRequests&, const AZ::Vector3&, float, const AZ::Vector3&);

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZ::Vector3 m_axis;
        float m_width = 0.0f;
        float m_radius = 0.0f;
        AZ::Color m_color = AZ::Colors::Red;
        DrawCircleFunc m_drawCircleFunc = nullptr;
    };

    // helpers to provide consistent function pointer interface for deciding
    // on type of circle to draw (see DrawCircleFunc in ManipulatorViewCircle above)
    void DrawHalfDottedCircle(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& position, float radius, const AZ::Vector3& viewPos);
    void DrawFullCircle(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& position, float radius, const AZ::Vector3& viewPos);

    //! Used for interaction with spline primitive - it will generate a spline bound
    //! to be interacted with and will display the intersection point on the spline
    //! where a user may wish to insert a point.
    class ManipulatorViewSplineSelect : public ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorViewSplineSelect, AZ::SystemAllocator)
        AZ_RTTI(ManipulatorViewSplineSelect, "{60996E49-D6BF-4817-BAA3-D27A407DD21A}", ManipulatorView)

        void Draw(
            ManipulatorManagerId managerId,
            const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId,
            const ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZStd::weak_ptr<const AZ::Spline> m_spline;
        float m_width = 0.0f;
        AZ::Color m_color = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    };

    //! Returns true if axis is pointing away from us (we should flip it).
    inline bool ShouldFlipCameraAxis(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& localPosition,
        const AZ::Vector3& axis,
        const AzFramework::CameraState& cameraState)
    {
        return (worldFromLocal.TransformPoint(localPosition) - cameraState.m_position)
                   .Dot(TransformDirectionNoScaling(worldFromLocal, axis)) > 0.0f;
    }

    //! @brief Return the world transform of the entity with uniform scale - choose
    //! the largest element.
    AZ::Transform WorldFromLocalWithUniformScale(AZ::EntityId entityId);

    //! Get the non-uniform scale for this entity id.
    AZ::Vector3 GetNonUniformScale(AZ::EntityId entityId);

    // Helpers to create various manipulator views.

    AZStd::unique_ptr<ManipulatorViewQuad> CreateManipulatorViewQuad(
        const AZ::Vector3& axis1,
        const AZ::Vector3& axis2,
        const AZ::Color& axis1Color,
        const AZ::Color& axis2Color,
        const AZ::Vector3& offset,
        float size);

    AZStd::unique_ptr<ManipulatorViewQuadBillboard> CreateManipulatorViewQuadBillboard(const AZ::Color& color, float size);

    AZStd::unique_ptr<ManipulatorViewLine> CreateManipulatorViewLine(
        const LinearManipulator& linearManipulator, const AZ::Color& color, float length, float width);

    AZStd::unique_ptr<ManipulatorViewLineSelect> CreateManipulatorViewLineSelect(
        const LineSegmentSelectionManipulator& lineSegmentManipulator, const AZ::Color& color, float width);

    AZStd::unique_ptr<ManipulatorViewCone> CreateManipulatorViewCone(
        const LinearManipulator& linearManipulator, const AZ::Color& color, const AZ::Vector3& offset, float length, float radius);

    AZStd::unique_ptr<ManipulatorViewBox> CreateManipulatorViewBox(
        const AZ::Transform& transform, const AZ::Color& color, const AZ::Vector3& offset, const AZ::Vector3& halfExtents);

    AZStd::unique_ptr<ManipulatorViewCylinder> CreateManipulatorViewCylinder(
        const LinearManipulator& linearManipulator, const AZ::Color& color, float length, float radius);

    AZStd::unique_ptr<ManipulatorViewSphere> CreateManipulatorViewSphere(
        const AZ::Color& color, float radius, const DecideColorFn& decideColor, bool enableDepthTest = false);

    AZStd::unique_ptr<ManipulatorViewProjectedCircle> CreateManipulatorViewProjectedCircle(
        const PaintBrushManipulator& brushManipulator, const AZ::Color& color, float radius, float width);

    AZStd::unique_ptr<ManipulatorViewCircle> CreateManipulatorViewCircle(
        const AngularManipulator& angularManipulator,
        const AZ::Color& color,
        float radius,
        float width,
        ManipulatorViewCircle::DrawCircleFunc drawFunc);

    AZStd::unique_ptr<ManipulatorViewSplineSelect> CreateManipulatorViewSplineSelect(
        const SplineSelectionManipulator& splineManipulator, const AZ::Color& color, float width);

    //! Returns the vector between the view (camera) and the manipulator in the space
    //! of the Manipulator (manipulator space + local transform).
    AZ::Vector3 CalculateViewDirection(const Manipulators& manipulators, const AZ::Vector3& worldViewPosition);

} // namespace AzToolsFramework
