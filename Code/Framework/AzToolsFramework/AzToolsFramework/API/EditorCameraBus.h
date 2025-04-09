/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/Component.h>

namespace AzFramework
{
    struct CameraState;
}

namespace Camera
{
    /**
     * This bus allows you to get and set the current editor viewport camera
     */
    class EditorCameraRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorCameraRequests>;
        virtual ~EditorCameraRequests() = default;

        static void Reflect(AZ::ReflectContext* context);

        /**
         * Sets the view from the entity's perspective
         * @param entityId the id of the entity whose perspective is to be used
         */
        virtual void SetViewFromEntityPerspective(const AZ::EntityId& /*entityId*/) {}

        /**
         * Gets the id of the current view entity. Invalid EntityId is returned for the default editor camera
         * @return the entityId of the entity currently being used as the view.  The Invalid entity id is returned for the default editor camera
         */
        virtual AZ::EntityId GetCurrentViewEntityId() { return AZ::EntityId(); }

        /**
         * Gets the position of the currently active Editor camera.
         * The Editor can have multiple viewports displayed, though at most only one is active at any point in time.
         * (Active is not the same as "has focus" - a different editor pane can have focus, but there's still one
         * active viewport that's updating every frame, and the others are not)
         * @param cameraPos On return, the current camera position in the one active Editor viewport.
         * @return True if the camera position was successfully retrieved, false if not.
         */
        virtual bool GetActiveCameraPosition(AZ::Vector3& /*cameraPos*/) { return false; }

        /**
         * Gets the transform of the currently active Editor camera.
         * The Editor can have multiple viewports displayed, though at most only one is active at any point in time.
         * (Active is not the same as "has focus" - a different editor pane can have focus, but there's still one
         * active viewport that's updating every frame, and the others are not)
         * @return the current camera transform in the one active Editor viewport.
         */
        virtual AZStd::optional<AZ::Transform> GetActiveCameraTransform() { return AZStd::nullopt; }

        /**
         * Gets the field of view of the currently active Editor camera.
         * The Editor can have multiple viewports displayed, though at most only one is active at any point in time.
         * (Active is not the same as "has focus" - a different editor pane can have focus, but there's still one
         * active viewport that's updating every frame, and the others are not)
         * @return the current camera field of view in the one active Editor viewport.
         */
        virtual AZStd::optional<float> GetCameraFoV() { return 60.0f; }

        /**
         * Gets the position of the currently active Editor camera.
         * The Editor can have multiple viewports displayed, though at most only one is active at any point in time.
         * (Active is not the same as "has focus" - a different editor pane can have focus, but there's still one
         * active viewport that's updating every frame, and the others are not).
         * @param cameraView The current camera view in the one active Editor viewport.
         * @return True if the camera view was successfully retrieved, false if not.
         */
        virtual bool GetActiveCameraState([[maybe_unused]] AzFramework::CameraState& cameraState) { return false; }
    };

    using EditorCameraRequestBus = AZ::EBus<EditorCameraRequests>;

    /**
     * This is the bus to interface with the camera system component
     */
    class EditorCameraSystemRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorCameraSystemRequests>;
        virtual ~EditorCameraSystemRequests() = default;

        virtual void CreateCameraEntityFromViewport() {}
    };
    using EditorCameraSystemRequestBus = AZ::EBus<EditorCameraSystemRequests>;

    /**
     * Handle this bus to be notified when the current editor viewport entity id changes
     */
    class EditorCameraNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~EditorCameraNotifications() = default;

        /**
         * Handle this message to know when the current viewports view entity has changed
         * @param newViewId the id of the entity the current view has switched to
         */
        virtual void OnViewportViewEntityChanged(const AZ::EntityId& /*newViewId*/) {}
    };

    using EditorCameraNotificationBus = AZ::EBus<EditorCameraNotifications>;

    /**
     * This bus is for requesting any camera-view-related changes or information
     */
    class EditorCameraViewRequests : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~EditorCameraViewRequests() = default;

        /**
         * Sets this camera as the active view in the scene, otherwise restores the default editor camera if it was already active
         */
        virtual void ToggleCameraAsActiveView() = 0;

        /**
         * Aligns this camera with the active view in the scene and sets it as the active camera
         */
        virtual void MatchViewport() = 0;

        /**
         * Returns true if this is the active camera.
         */
        virtual bool IsActiveCamera() const = 0;

        /**
        * Gets the camera state associated with this view.
        */
        virtual bool GetCameraState(AzFramework::CameraState& cameraState) = 0;
    };

    using EditorCameraViewRequestBus = AZ::EBus<EditorCameraViewRequests>;

} // namespace Camera
