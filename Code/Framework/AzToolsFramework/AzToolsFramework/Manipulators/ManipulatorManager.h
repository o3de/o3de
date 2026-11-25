/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBoundManager.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzFramework
{
    struct CameraState;
    class DebugDisplayRequests;
} // namespace AzFramework

namespace AzToolsFramework
{
    namespace Picking
    {
        class DefaultContextBoundManager;
    }

    namespace ViewportInteraction
    {
        struct MouseInteraction;
    }

    class BaseManipulator;
    class LinearManipulator;

    //! State of overall manipulator manager.
    struct ManipulatorManagerState
    {
        bool m_interacting;
    };

    //! The button that was used to start the manipulator behavior.
    enum class ManipulatorMouseDownButton
    {
        Left,
        Right
    };

    //! This class serves to manage all relevant mouse events and coordinate all registered manipulators to function properly.
    //! ManipulatorManager does not manage the life cycle of specific manipulators. The users of manipulators are responsible
    //! for creating and deleting them at right time, as well as registering and unregistering accordingly.
    class AZTF_API ManipulatorManager
        : private ManipulatorManagerRequestBus::Handler
        , private EditorEntityInfoNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorManager, AZ::SystemAllocator)

        explicit ManipulatorManager(ManipulatorManagerId managerId);
        ~ManipulatorManager();

        //! The result of consuming a mouse move.
        enum class ConsumeMouseMoveResult
        {
            None,
            Hovering, // Note: unused
            Interacting,
        };

        // Note: These are not EBus messages, they are called by the owner of the manipulator manager and they will
        // return true if they have handled the interaction - if this is the case you should not process it yourself.
        bool ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction&);
        ConsumeMouseMoveResult ConsumeViewportMouseMove(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseWheel(const ViewportInteraction::MouseInteraction&);

        // ManipulatorManagerRequestBus ...
        void RegisterManipulator(AZStd::shared_ptr<BaseManipulator> manipulator) override;
        void UnregisterManipulator(BaseManipulator* manipulator) override;
        void DeleteManipulatorBound(Picking::RegisteredBoundId boundId) override;
        void SetBoundDirty(Picking::RegisteredBoundId boundId) override;
        Picking::RegisteredBoundId UpdateBound(
            ManipulatorId manipulatorId, Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData,
            AzFramework::ViewportId viewportId) override;
        bool Interacting() const override
        {
            return m_activeManipulator != nullptr;
        }

        void DrawManipulators(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            ViewportInteraction::MouseInteraction mouseInteraction,
            AzFramework::ViewportId viewportId = AzFramework::InvalidViewportId);

    protected:
        //! Intersects the input ray with bounds registered for its viewport.
        //! @param[out] rayIntersectionDistance The result intersecting point equals "rayOrigin + rayIntersectionDistance * rayDirection".
        //! @return A pointer to a manipulator that the ray intersects. Null pointer if no intersection is detected.
        AZStd::shared_ptr<BaseManipulator> PerformRaycast(
            const ViewportInteraction::MouseInteraction& interaction, float& rayIntersectionDistance);

        // EditorEntityInfoNotifications ...
        void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;

        //! Alias for a Manipulator and intersection distance.
        using PickedManipulator = AZStd::tuple<AZStd::shared_ptr<BaseManipulator>, float>;
        //! Alias for a ManipulatorId and intersection distance.
        using PickedManipulatorId = AZStd::tuple<ManipulatorId, float>;

        //! Return the picked manipulator and intersection distance if a manipulator was intersected.
        AZStd::optional<PickedManipulator> PickManipulator(const ViewportInteraction::MouseInteraction& interaction);
        //! Wrapper for PickManipulator to return the ManipulatorId directly.
        PickedManipulatorId PickManipulatorId(const ViewportInteraction::MouseInteraction& interaction);

        //! Called once per frame after all manipulators have been drawn (and their
        //! bounds updated if required).
        void RefreshMouseOverState(const ViewportInteraction::MouseInteraction& interaction);

        ManipulatorManagerId m_manipulatorManagerId; //!< This manipulator manager's id.
        ManipulatorId m_nextManipulatorIdToGenerate; //!< Id to use for the next manipulator that is registered with this manager.

        //! Mapping from a manipulatorId to the corresponding manipulator.
        AZStd::unordered_map<ManipulatorId, AZStd::shared_ptr<BaseManipulator>> m_manipulatorIdToPtrMap;
        //! Mapping from a boundId to the corresponding manipulatorId.
        struct BoundOwner
        {
            ManipulatorId m_manipulatorId;
            AzFramework::ViewportId m_viewportId;
        };
        AZStd::unordered_map<Picking::RegisteredBoundId, BoundOwner> m_boundOwners;

        AZStd::shared_ptr<BaseManipulator> m_activeManipulator; //!< The manipulator we are currently interacting with.
        Picking::ManipulatorBoundManager m_boundManager; //!< All active manipulator bounds that could be interacted with.
        //! The mouse button that is currently pressed (empty if no button is held).
        AZStd::optional<ManipulatorMouseDownButton> m_mouseDownButton;
    };

    // The main/default ManipulatorManagerId to be used for
    // registering manipulators used by components in the level
    AZTF_API ManipulatorManagerId GetMainManipulatorManagerId();

} // namespace AzToolsFramework
