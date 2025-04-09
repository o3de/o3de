/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzFramework
{
    struct ScreenPoint;

    namespace RenderGeometry
    {
        struct RayRequest;
    }
} // namespace AzFramework

namespace AzToolsFramework
{
    enum class CursorInputMode;

    namespace ViewportInteraction
    {
        //! Result of handling mouse interaction.
        enum class MouseInteractionResult
        {
            Manipulator, //!< The manipulator manager handled the interaction.
            Viewport, //!< The viewport handled the interaction.
            None //!< The interaction was not handled.
        };

        //! Interface for handling mouse viewport events.
        class MouseViewportRequests
        {
        public:
            //! @cond
            virtual ~MouseViewportRequests() = default;
            //! @endcond

            //! Implement this function to handle a particular mouse event.
            virtual bool HandleMouseInteraction(const MouseInteractionEvent& /*mouseInteraction*/)
            {
                return false;
            }
        };

        //! Interface for internal handling mouse viewport events.
        class InternalMouseViewportRequests
        {
        public:
            //! @cond
            virtual ~InternalMouseViewportRequests() = default;
            //! @endcond

            //! Implement this function to have the viewport handle this mouse event.
            virtual bool InternalHandleMouseViewportInteraction(const MouseInteractionEvent& /*mouseInteraction*/)
            {
                return false;
            }

            //! Implement this function to have manipulators handle this mouse event.
            virtual bool InternalHandleMouseManipulatorInteraction(const MouseInteractionEvent& /*mouseInteraction*/)
            {
                return false;
            }

            //! Helper to call both viewport and manipulator handle mouse events
            //! @note Manipulators always attempt to intercept the event first.
            MouseInteractionResult InternalHandleAllMouseInteractions(const MouseInteractionEvent& mouseInteraction);
        };

        //! Interface for viewport selection behaviors.
        class ViewportDisplayNotifications
        {
        public:
            //! @cond
            virtual ~ViewportDisplayNotifications() = default;
            //! @endcond

            //! Display drawing in world space.
            //! \ref DisplayViewportSelection is called from \ref EditorInteractionSystemComponent::DisplayViewport.
            //! DisplayViewport exists on the \ref AzFramework::ViewportDebugDisplayEventBus and is called from \ref CRenderViewport.
            //! \ref DisplayViewportSelection is called after \ref CalculateVisibleEntityDatas on the \ref EditorVisibleEntityDataCache,
            //! this ensures usage of the entity cache will be up to date (do not implement \ref AzFramework::ViewportDebugDisplayEventBus
            //! directly if wishing to use the \ref EditorVisibleEntityDataCache).
            virtual void DisplayViewportSelection(
                const AzFramework::ViewportInfo& /*viewportInfo*/, AzFramework::DebugDisplayRequests& /*debugDisplay*/)
            {
            }
            //! Display drawing in screen space.
            //! \ref DisplayViewportSelection2d is called after \ref DisplayViewportSelection when the viewport has been
            //! configured to be orthographic in \ref CRenderViewport. All screen space drawing can be performed here.
            virtual void DisplayViewportSelection2d(
                const AzFramework::ViewportInfo& /*viewportInfo*/, AzFramework::DebugDisplayRequests& /*debugDisplay*/)
            {
            }
        };

        //! Interface for internal handling mouse viewport events and display notifications.
        //! Implement this for types wishing to provide viewport functionality and
        //! set it by using \ref EditorInteractionSystemViewportSelectionRequestBus.
        class InternalViewportSelectionRequests
            : public ViewportDisplayNotifications
            , public InternalMouseViewportRequests
        {
        };

        //! Interface for handling mouse viewport events and display notifications.
        //! Use this interface for composition types used by InternalViewportSelectionRequests.
        class ViewportSelectionRequests
            : public ViewportDisplayNotifications
            , public MouseViewportRequests
        {
        };

        //! The EBusTraits for ViewportInteractionRequests.
        //! @deprecated ViewportEBusTraits is deprecated, please use ViewportRequestsEBusTraits.
        //! O3DE_DEPRECATION_NOTICE(GHI-13429)
        class ViewportEBusTraits : public AZ::EBusTraits
        {
        public:
            using BusIdType = AzFramework::ViewportId; //!< ViewportId - used to address requests to this EBus.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        //! The EBusTraits for ViewportInteractionRequests.
        using ViewportRequestsEBusTraits = ViewportEBusTraits;

        //! A bus to listen to just the MouseViewportRequests.
        using ViewportMouseRequestBus = AZ::EBus<MouseViewportRequests, ViewportRequestsEBusTraits>;

        //! Requests that can be made to the viewport to query and modify its state.
        class ViewportInteractionRequests
        {
        public:
            //! Returns the current camera state for this viewport.
            virtual AzFramework::CameraState GetCameraState() = 0;
            //! Transforms a point in world space to screen space coordinates in viewport pixel space
            virtual AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) = 0;
            //! Transforms a point in viewport pixel space to world space based on the given clip space depth.
            //! Returns the world space position if successful.
            virtual AZ::Vector3 ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition) = 0;
            //! Casts a point in screen space to a ray in world space originating from the viewport camera frustum's near plane.
            //! Returns a ray containing the ray's origin and a direction normal, if successful.
            virtual ProjectedViewportRay ViewportScreenToWorldRay(const AzFramework::ScreenPoint& screenPosition) = 0;
            //! Gets the DPI scaling factor that translates Qt widget space into viewport pixel space.
            virtual float DeviceScalingFactor() = 0;

        protected:
            ~ViewportInteractionRequests() = default;
        };

        //! Type to inherit to implement ViewportInteractionRequests.
        using ViewportInteractionRequestBus = AZ::EBus<ViewportInteractionRequests, ViewportRequestsEBusTraits>;

        //! Utility function to return a viewport ray using the ViewportInteractionRequestBus.
        ProjectedViewportRay ViewportScreenToWorldRay(const AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint);

        //! The EBusTraits for ViewportInteractionNotifications.
        class ViewportNotificationsEBusTraits : public AZ::EBusTraits
        {
        public:
            using BusIdType = AzFramework::ViewportId; //!< ViewportId - used to address requests to this EBus.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        };

        //! Notifications for a specific viewport relating to user input/interactions.
        class ViewportInteractionNotifications
        {
        public:
            //! Notification to indicate when the viewport has gained focus.
            virtual void OnViewportFocusIn()
            {
            }

            //! Notification to indicate when the viewport has lost focus.
            virtual void OnViewportFocusOut()
            {
            }

        protected:
            ~ViewportInteractionNotifications() = default;
        };

        //! Type to inherit to implement ViewportInteractionNotifications.
        using ViewportInteractionNotificationBus = AZ::EBus<ViewportInteractionNotifications, ViewportNotificationsEBusTraits>;

        //! Interface to return only viewport specific settings (e.g. snapping).
        class ViewportSettingsRequests
        {
        public:
            //! Return if grid snapping is enabled.
            virtual bool GridSnappingEnabled() const = 0;
            //! Return the grid snapping size.
            virtual float GridSize() const = 0;
            //! Does the grid currently want to be displayed.
            virtual bool ShowGrid() const = 0;
            //! Return if angle snapping is enabled.
            virtual bool AngleSnappingEnabled() const = 0;
            //! Return the angle snapping/step size.
            virtual float AngleStep() const = 0;
            //! Returns the current line bound width for manipulators.
            virtual float ManipulatorLineBoundWidth() const = 0;
            //! Returns the current circle (torus) bound width for manipulators.
            virtual float ManipulatorCircleBoundWidth() const = 0;
            //! Returns if sticky select is enabled or not.
            virtual bool StickySelectEnabled() const = 0;
            //! Returns the default viewport camera position.
            virtual AZ::Vector3 DefaultEditorCameraPosition() const = 0;
            //! Returns the default viewport camera orientation (pitch and yaw in degrees).
            virtual AZ::Vector2 DefaultEditorCameraOrientation() const = 0;
            //! Returns if icons are visible in the viewport.
            virtual bool IconsVisible() const = 0;
            //! Returns if viewport helpers (additional debug drawing) are visible in the viewport.
            virtual bool HelpersVisible() const = 0;
            //! Returns if viewport helpers are only drawn for selected entities in the viewport.
            virtual bool OnlyShowHelpersForSelectedEntities() const = 0;

        protected:
            ~ViewportSettingsRequests() = default;
        };

        //! Type to inherit to implement ViewportSettingsRequests.
        using ViewportSettingsRequestBus = AZ::EBus<ViewportSettingsRequests, ViewportRequestsEBusTraits>;

        //! An interface to notify when changes to viewport settings have happened.
        class ViewportSettingNotifications
        {
        public:
            virtual void OnAngleSnappingChanged([[maybe_unused]] bool enabled)
            {
            }
            virtual void OnGridSnappingChanged([[maybe_unused]] bool enabled)
            {
            }
            virtual void OnGridShowingChanged([[maybe_unused]] bool showing)
            {
            }
            virtual void OnDrawHelpersChanged([[maybe_unused]] bool enabled)
            {
            }
            virtual void OnIconsVisibilityChanged([[maybe_unused]] bool enabled)
            {
            }
            virtual void OnCameraFovChanged([[maybe_unused]] float fovRadians)
            {
            }
            virtual void OnCameraSpeedScaleChanged([[maybe_unused]] float value)
            {
            }

        protected:
            ~ViewportSettingNotifications() = default;
        };

        using ViewportSettingsNotificationBus = AZ::EBus<ViewportSettingNotifications, ViewportRequestsEBusTraits>;

        //! Viewport requests that are only guaranteed to be serviced by the Main Editor viewport.
        class MainEditorViewportInteractionRequests
        {
        public:
            //! Is the user holding a modifier key to move the manipulator space from local to world.
            virtual bool ShowingWorldSpace() = 0;
            //! Return the widget to use as the parent for the viewport context menu.
            virtual QWidget* GetWidgetForViewportContextMenu() = 0;

        protected:
            ~MainEditorViewportInteractionRequests() = default;
        };

        //! Type to inherit to implement MainEditorViewportInteractionRequests.
        using MainEditorViewportInteractionRequestBus = AZ::EBus<MainEditorViewportInteractionRequests, ViewportRequestsEBusTraits>;

        //! Editor entity requests to be made about the viewport.
        class EditorEntityViewportInteractionRequests
        {
        public:
            //! Given the current view frustum (viewport) return all visible entities.
            virtual void FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntities) = 0;

        protected:
            ~EditorEntityViewportInteractionRequests() = default;
        };

        using EditorEntityViewportInteractionRequestBus = AZ::EBus<EditorEntityViewportInteractionRequests, ViewportRequestsEBusTraits>;

        //! An interface to query editor modifier keys.
        class EditorModifierKeyRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            //! Returns the current state of the keyboard modifier keys.
            virtual KeyboardModifiers QueryKeyboardModifiers() = 0;

        protected:
            ~EditorModifierKeyRequests() = default;
        };

        using EditorModifierKeyRequestBus = AZ::EBus<EditorModifierKeyRequests>;

        //! Convenience method to call the above EditorModifierKeyRequestBus's QueryKeyModifiers event
        KeyboardModifiers QueryKeyboardModifiers();

        //! An interface to deal with time requests relating to viewports.
        //! @note The bus is global and not per viewport.
        class EditorViewportInputTimeNowRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            //! Returns the current time in seconds.
            //! This interface can be overridden for the purposes of testing to simplify viewport input requests.
            virtual AZStd::chrono::milliseconds EditorViewportInputTimeNow() = 0;

        protected:
            ~EditorViewportInputTimeNowRequests() = default;
        };

        using EditorViewportInputTimeNowRequestBus = AZ::EBus<EditorViewportInputTimeNowRequests>;

        //! The style of cursor override.
        enum class CursorStyleOverride
        {
            Forbidden
        };

        //! Viewport requests for managing the viewport cursor state.
        class ViewportMouseCursorRequests
        {
        public:
            //! Begins hiding the cursor and locking it in place, to prevent the cursor from escaping the viewport window.
            virtual void BeginCursorCapture() = 0;
            //! Restores the cursor and ends locking it in place, allowing it to be moved freely.
            virtual void EndCursorCapture() = 0;
            //!  Sets the cursor input mode.
            virtual void SetCursorMode(AzToolsFramework::CursorInputMode mode) = 0;
            //! Is the mouse over the viewport.
            virtual bool IsMouseOver() const = 0;
            //! Set the cursor style override.
            virtual void SetOverrideCursor(CursorStyleOverride cursorStyleOverride) = 0;
            //! Clear the cursor style override.
            virtual void ClearOverrideCursor() = 0;
            //! Returns the viewport position of the cursor if a valid position exists (the cursor is over the viewport).
            virtual AZStd::optional<AzFramework::ScreenPoint> MousePosition() const = 0;

        protected:
            ~ViewportMouseCursorRequests() = default;
        };

        //! Type to inherit to implement MainEditorViewportInteractionRequests.
        using ViewportMouseCursorRequestBus = AZ::EBus<ViewportMouseCursorRequests, ViewportRequestsEBusTraits>;
    } // namespace ViewportInteraction

    //! Utility function to return EntityContextId.
    AzFramework::EntityContextId GetEntityContextId();

    //! Performs an intersection test against meshes in the scene, if there is a hit (the ray intersects
    //! a mesh), that position is returned, otherwise a point projected defaultDistance from the
    //! origin of the ray will be returned.
    //! @note The intersection will only consider visible objects.
    //! @param viewportId The id of the viewport the raycast into.
    //! @param screenPoint The starting point of the ray in screen space. (The ray will cast into the screen)
    //! @param rayLength The length of the ray in meters.
    //! @param defaultDistance The distance to use for the result point if no hit is found.
    //! @return The world position of the intersection point (either from a hit or from the default distance)
    AZ::Vector3 FindClosestPickIntersection(
        AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint, float rayLength, float defaultDistance);

    //! Performs an intersection test against meshes in the scene and returns the his position only if there
    //! is a hit (the ray intersects a mesh).
    //! @note The intersection will only consider visible objects.
    //! @param viewportId The id of the viewport the raycast into.
    //! @param screenPoint The starting point of the ray in screen space. (The ray will cast into the screen)
    //! @param rayLength The length of the ray in meters.
    //! @return The world position of the intersection if there is a hit, or nothing if not.
    AZStd::optional<AZ::Vector3> FindClosestPickIntersection(
        AzFramework::ViewportId viewportId, const AzFramework::ScreenPoint& screenPoint, float rayLength);

    //! Overload of FindClosestPickIntersection taking a RenderGeometry::RayRequest directly.
    //! @note rayRequest must contain a valid ray/line segment (start/endWorldPosition must not be at the same position).
    //! @param rayRequest Information describing the start/end positions and which entities to raycast against.
    //! @param defaultDistance The distance to use for the result point if no hit is found.
    //! @return The world position of the intersection point (either from a hit or from the default distance)
    AZ::Vector3 FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest, float defaultDistance);

    //! Overload of FindClosestPickIntersection taking a RenderGeometry::RayRequest directly.
    //! @note rayRequest must contain a valid ray/line segment (start/endWorldPosition must not be at the same position).
    //! @param rayRequest Information describing the start/end positions and which entities to raycast against.
    //! @return The world position of the intersection if there is a hit, or nothing if not.
    AZStd::optional<AZ::Vector3> FindClosestPickIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest);

    //! Update the in/out parameter rayRequest based on the latest viewport ray.
    void RefreshRayRequest(
        AzFramework::RenderGeometry::RayRequest& rayRequest, const ViewportInteraction::ProjectedViewportRay& viewportRay, float rayLength);

    //! Maps a mouse interaction event to a ClickDetector event.
    //! @note Function only cares about up or down events, all other events are mapped to Nil (ignored).
    AzFramework::ClickDetector::ClickEvent ClickDetectorEventFromViewportInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

    //! Wrap EBus call to retrieve manipulator line bound width.
    //! @note It is possible to pass AzFramework::InvalidViewportId (the default) to perform a Broadcast as opposed to a targeted Event.
    float ManipulatorLineBoundWidth(AzFramework::ViewportId viewportId = AzFramework::InvalidViewportId);

    //! Wrap EBus call to retrieve manipulator circle bound width.
    //! @note It is possible to pass AzFramework::InvalidViewportId (the default) to perform a Broadcast as opposed to a targeted Event.
    float ManipulatorCicleBoundWidth(AzFramework::ViewportId viewportId = AzFramework::InvalidViewportId);
} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN_WITH_TRAITS(AzToolsFramework::ViewportInteraction::ViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportRequestsEBusTraits);
