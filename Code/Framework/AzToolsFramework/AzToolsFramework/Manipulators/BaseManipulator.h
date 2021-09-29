/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorSpace.h>

namespace AzFramework
{
    struct CameraState;
    class DebugDisplayRequests;
} // namespace AzFramework

namespace AzToolsFramework
{
    AZ_CVAR_EXTERNED(bool, cl_manipulatorDrawDebug);

    namespace UndoSystem
    {
        class URSequencePoint;
    }

    namespace ViewportInteraction
    {
        struct MouseInteraction;
    }

    struct ManipulatorManagerState;

    //! The base class for manipulators, providing interfaces for users of manipulators to talk to.
    class BaseManipulator : public AZStd::enable_shared_from_this<BaseManipulator>
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(BaseManipulator, "{3D1CD58D-C589-464C-BC9A-480D59341AB4}")

        BaseManipulator(const BaseManipulator&) = delete;
        BaseManipulator& operator=(const BaseManipulator&) = delete;

        virtual ~BaseManipulator();

        using EntityComponentIds = AZStd::unordered_set<AZ::EntityComponentIdPair>;

        //! Callback for the event when the mouse pointer is over this manipulator and the left mouse button is pressed.
        //! @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera
        //! through the mouse pointer.
        //! @param rayIntersectionDistance The parameter value in the ray's explicit equation that represents the intersecting point on the
        //! target manipulator in world space.
        //! @return Return true if OnLeftMouseDownImpl was attached and will be used.
        bool OnLeftMouseDown(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance);

        //! Callback for the event when this manipulator is active and the left mouse button is released.
        //! @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera
        //! through the mouse pointer.
        void OnLeftMouseUp(const ViewportInteraction::MouseInteraction& interaction);

        //! Callback for the event when the mouse pointer is over this manipulator and the right mouse button is pressed .
        //! @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera
        //! through the mouse pointer.
        //! @param rayIntersectionDistance The parameter value in the ray's explicit equation that represents the intersecting point on the
        //! target manipulator in world space.
        //! @return Return true if OnRightMouseDownImpl was attached and will be used.
        bool OnRightMouseDown(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance);

        //! Callback for the event when this manipulator is active and the right mouse button is released.
        //! @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera
        //! through the mouse pointer.
        void OnRightMouseUp(const ViewportInteraction::MouseInteraction& interaction);

        //! Callback for the event when this manipulator is active and the mouse is moved.
        //! @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera
        //! through the mouse pointer.
        void OnMouseMove(const ViewportInteraction::MouseInteraction& interaction);

        //! Callback for the event when this manipulator is active and the mouse wheel is scrolled.
        //! @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera
        //! through the mouse pointer.
        void OnMouseWheel(const ViewportInteraction::MouseInteraction& interaction);

        //! This function changes the state indicating whether the manipulator is under the mouse pointer.
        //! It is called in the event of OnMouseMove and OnMouseWheel only when there is no manipulator currently performing actions.
        bool OnMouseOver(ManipulatorId manipulatorId, const ViewportInteraction::MouseInteraction& interaction);

        //! Register itself to a manipulator manager so that it can receive various mouse events and perform manipulations.
        //! @param managerId The id identifying a unique manipulator manager.
        void Register(ManipulatorManagerId managerId);

        //! Unregister itself from the manipulator manager it was registered with.
        void Unregister();

        //! Bounds will need to be recalculated next time we render.
        void SetBoundsDirty();

        //! Is this manipulator currently registered with a manipulator manager.
        bool Registered() const
        {
            return m_manipulatorId != InvalidManipulatorId && m_manipulatorManagerId != InvalidManipulatorManagerId;
        }

        //! Is the manipulator in the middle of an action (between mouse down and mouse up).
        bool PerformingAction() const
        {
            return m_performingAction;
        }

        //! Is the mouse currently over the manipulator (intersecting manipulator bound).
        bool MouseOver() const
        {
            return m_mouseOver;
        }

        //! The unique id of this manipulator.
        ManipulatorId GetManipulatorId() const
        {
            return m_manipulatorId;
        }

        //! The unique id of the manager this manipulator was registered with.
        ManipulatorManagerId GetManipulatorManagerId() const
        {
            return m_manipulatorManagerId;
        }

        //! Returns all EntityComponentIdPairs associated with this manipulator.
        const EntityComponentIds& EntityComponentIdPairs() const
        {
            return m_entityComponentIdPairs;
        }

        //! Add an entity and component the manipulator is responsible for.
        void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair);

        //! Remove an entity from being affected by this manipulator.
        //! @note All components on this entity registered with the manipulator will be removed.
        EntityComponentIds::iterator RemoveEntityId(AZ::EntityId entityId);

        //! Remove a specific component (via a EntityComponentIdPair) being affected by this manipulator.
        EntityComponentIds::iterator RemoveEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair);

        //! Is this entity currently being tracked by this manipulator.
        bool HasEntityId(AZ::EntityId entityId) const;

        //! Is this entity component pair currently being tracked by this manipulator.
        bool HasEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) const;

        //! Forward a mouse over event in a case where we need the manipulator to immediately refresh.
        //! @note Only call this when a mouse over event has just happened.
        void ForwardMouseOverEvent(const ViewportInteraction::MouseInteraction& interaction);

        static const AZ::Color s_defaultMouseOverColor;

    protected:
        //! Protected constructor.
        BaseManipulator() = default;

        //! Called when unregistering - users of manipulators should not call it directly.
        void Invalidate();

        //! The implementation to override in a derived class for Invalidate.
        virtual void InvalidateImpl()
        {
        }

        //! The implementation to override in a derived class for OnLeftMouseDown.
        //! Note: When implementing this function you must also call AttachLeftMouseDownImpl to ensure
        //! m_onLeftMouseDownImpl is set to OnLeftMouseDownImpl, otherwise it will not be called
        virtual void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& /*interaction*/, float /*rayIntersectionDistance*/)
        {
        }

        void AttachLeftMouseDownImpl()
        {
            m_onLeftMouseDownImpl = &BaseManipulator::OnLeftMouseDownImpl;
        }

        //! The implementation to override in a derived class for OnRightMouseDown.
        //! Note: When implementing this function you must also call AttachRightMouseDownImpl to ensure
        //! m_onRightMouseDownImpl is set to OnRightMouseDownImpl, otherwise it will not be called
        virtual void OnRightMouseDownImpl(const ViewportInteraction::MouseInteraction& /*interaction*/, float /*rayIntersectionDistance*/)
        {
        }

        void AttachRightMouseDownImpl()
        {
            m_onRightMouseDownImpl = &BaseManipulator::OnRightMouseDownImpl;
        }

        //! The implementation to override in a derived class for OnLeftMouseUp.
        virtual void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& /*interaction*/)
        {
        }

        //! The implementation to override in a derived class for OnRightMouseUp.
        virtual void OnRightMouseUpImpl(const ViewportInteraction::MouseInteraction& /*interaction*/)
        {
        }

        //! The implementation to override in a derived class for OnMouseMove.
        virtual void OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& /*interaction*/)
        {
        }

        //! The implementation to override in a derived class for OnMouseOver.
        virtual void OnMouseOverImpl(ManipulatorId /*manipulatorId*/, const ViewportInteraction::MouseInteraction& /*interaction*/)
        {
        }

        //! The implementation to override in a derived class for OnMouseWheel.
        virtual void OnMouseWheelImpl(const ViewportInteraction::MouseInteraction& /*interaction*/)
        {
        }

        //! The implementation to override in a derived class for SetBoundsDirty.
        virtual void SetBoundsDirtyImpl()
        {
        }

        //! Rendering for the manipulator - it is recommended drawing be delegated to a ManipulatorView.
        virtual void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) = 0;

    private:
        friend class ManipulatorManager;
        AZStd::unordered_set<AZ::EntityComponentIdPair> m_entityComponentIdPairs; //!< The entities this manipulator is associated with.

        ManipulatorId m_manipulatorId = InvalidManipulatorId; //!< The unique id of this manipulator.
        ManipulatorManagerId m_manipulatorManagerId = InvalidManipulatorManagerId; //!< The manager this manipulator was registered with.
        UndoSystem::URSequencePoint* m_undoBatch = nullptr; //!< Undo active while mouse is pressed.
        bool m_performingAction = false; //!< After mouse down and before mouse up.
        bool m_mouseOver = false; //!< Is the mouse pointer over the manipulator bound.

        //! Member function pointers to OnLeftMouseDownImpl and OnRightMouseDownImpl.
        //! Set in AttachLeft/RightMouseDownImpl.
        void (BaseManipulator::*m_onLeftMouseDownImpl)(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) = nullptr;
        void (BaseManipulator::*m_onRightMouseDownImpl)(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) = nullptr;

        //! Update the mouseOver state for this manipulator.
        void UpdateMouseOver(ManipulatorId manipulatorId);

        //! Manage correctly ending the undo batch.
        void EndUndoBatch();

        //! Record an action as having started.
        void BeginAction();

        //! Record an action as having stopped.
        void EndAction();

        //! Let other systems (UI) know that a component property has been modified by a manipulator.
        void NotifyEntityComponentPropertyChanged();
    };

    //! Base class to be used when composing aggregate manipulator types - wraps some
    //! common functionality all manipulators need.
    class Manipulators
    {
    public:
        virtual ~Manipulators() = default;

        void Register(ManipulatorManagerId manipulatorManagerId);
        void Unregister();
        void SetBoundsDirty();
        void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void RemoveEntityId(AZ::EntityId entityId);
        void RemoveEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair);
        bool PerformingAction();
        bool Registered();

        //! Refresh the Manipulator and/or View based on the current view position.
        virtual void RefreshView(const AZ::Vector3& /*worldViewPosition*/)
        {
        }

        const AZ::Transform& GetLocalTransform() const;
        const AZ::Transform& GetSpace() const;
        const AZ::Vector3& GetNonUniformScale() const;
        void SetSpace(const AZ::Transform& worldFromLocal);
        void SetLocalTransform(const AZ::Transform& localTransform);
        void SetLocalPosition(const AZ::Vector3& localPosition);
        void SetLocalOrientation(const AZ::Quaternion& localOrientation);
        void SetNonUniformScale(const AZ::Vector3& nonUniformScale);

    protected:
        //! Common processing for base manipulator type - Implement for all
        //! individual manipulators used in an aggregate manipulator.
        virtual void ProcessManipulators(const AZStd::function<void(BaseManipulator*)>&) = 0;

        //!@{
        //! Allows implementers to perform additional logic when updating the location of the manipulator group.
        virtual void SetSpaceImpl([[maybe_unused]] const AZ::Transform& worldFromLocal)
        {
        }

        virtual void SetLocalTransformImpl([[maybe_unused]] const AZ::Transform& localTransform)
        {
        }

        virtual void SetLocalPositionImpl([[maybe_unused]] const AZ::Vector3& localPosition)
        {
        }

        virtual void SetLocalOrientationImpl([[maybe_unused]] const AZ::Quaternion& localOrientation)
        {
        }

        virtual void SetNonUniformScaleImpl([[maybe_unused]] const AZ::Vector3& nonUniformScale)
        {
        }
        //!@}

        ManipulatorSpaceWithLocalTransform m_manipulatorSpaceWithLocalTransform; //!< The space and local transform for the manipulators.
    };

    namespace Internal
    {
        //! This helper function calculates the intersecting point between a ray and a plane.
        //! @param rayOrigin                     The origin of the ray to test.
        //! @param rayDirection                  The direction of the ray to test.
        //! @param maxRayLength                  The maximum length of the ray to test.
        //! @param pointOnPlane                  A point on the plane.
        //! @param planeNormal                   The normal vector of the plane.
        //! @param[out] resultIntersectingPoint  This stores the result intersecting point. It will be left unchanged
        //!                                      if there is no intersection between the ray and the plane.
        //! @return                              Was there an intersection
        bool CalculateRayPlaneIntersectingPoint(
            const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection,
            const AZ::Vector3& pointOnPlane,
            const AZ::Vector3& planeNormal,
            AZ::Vector3& resultIntersectingPoint);

        //! Returns startLocalHitPosition if currentLocalHitPosition is further away than the camera's far clip plane.
        AZ::Vector3 TryConstrainHitPositionToView(
            const AZ::Vector3& currentLocalHitPosition,
            const AZ::Vector3& startLocalHitPosition,
            const AZ::Transform& localFromWorld,
            const AzFramework::CameraState& cameraState);
    } // namespace Internal
} // namespace AzToolsFramework
