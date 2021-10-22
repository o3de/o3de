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
}

namespace AzToolsFramework
{
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

        inline MouseInteractionResult InternalMouseViewportRequests::InternalHandleAllMouseInteractions(
            const MouseInteractionEvent& mouseInteraction)
        {
            if (InternalHandleMouseManipulatorInteraction(mouseInteraction))
            {
                return MouseInteractionResult::Manipulator;
            }
            else if (InternalHandleMouseViewportInteraction(mouseInteraction))
            {
                return MouseInteractionResult::Viewport;
            }
            else
            {
                return MouseInteractionResult::None;
            }
        }

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
        class ViewportEBusTraits : public AZ::EBusTraits
        {
        public:
            using BusIdType = AzFramework::ViewportId; //!< ViewportId - used to address requests to this EBus.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        //! A ray projection, originating from a point and extending in a direction specified as a normal.
        struct ProjectedViewportRay
        {
            AZ::Vector3 origin;
            AZ::Vector3 direction;
        };

        //! Requests that can be made to the viewport to query and modify its state.
        class ViewportInteractionRequests
        {
        public:
            //! Returns the current camera state for this viewport.
            virtual AzFramework::CameraState GetCameraState() = 0;
            //! Transforms a point in world space to screen space coordinates in Qt Widget space.
            //! Multiply by DeviceScalingFactor to get the position in viewport pixel space.
            virtual AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) = 0;
            //! Transforms a point from Qt widget screen space to world space based on the given clip space depth.
            //! Depth specifies a relative camera depth to project in the range of [0.f, 1.f].
            //! Returns the world space position if successful.
            virtual AZStd::optional<AZ::Vector3> ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition, float depth) = 0;
            //! Casts a point in screen space to a ray in world space originating from the viewport camera frustum's near plane.
            //! Returns a ray containing the ray's origin and a direction normal, if successful.
            virtual AZStd::optional<ProjectedViewportRay> ViewportScreenToWorldRay(const AzFramework::ScreenPoint& screenPosition) = 0;
            //! Gets the DPI scaling factor that translates Qt widget space into viewport pixel space.
            virtual float DeviceScalingFactor() = 0;

        protected:
            ~ViewportInteractionRequests() = default;
        };

        //! Type to inherit to implement ViewportInteractionRequests.
        using ViewportInteractionRequestBus = AZ::EBus<ViewportInteractionRequests, ViewportEBusTraits>;

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

        protected:
            ~ViewportSettingsRequests() = default;
        };

        //! Type to inherit to implement ViewportSettingsRequests.
        using ViewportSettingsRequestBus = AZ::EBus<ViewportSettingsRequests, ViewportEBusTraits>;

        //! An interface to notify when changes to viewport settings have happened.
        class ViewportSettingNotifications
        {
        public:
            virtual void OnGridSnappingChanged([[maybe_unused]] bool enabled)
            {
            }
            virtual void OnDrawHelpersChanged([[maybe_unused]] bool enabled)
            {
            }

        protected:
            ~ViewportSettingNotifications() = default;
        };

        using ViewportSettingsNotificationBus = AZ::EBus<ViewportSettingNotifications, ViewportEBusTraits>;

        //! Viewport requests that are only guaranteed to be serviced by the Main Editor viewport.
        class MainEditorViewportInteractionRequests
        {
        public:
            //! Given a point in screen space, return the picked entity (if any).
            //! Picked EntityId will be returned, InvalidEntityId will be returned on failure.
            virtual AZ::EntityId PickEntity(const AzFramework::ScreenPoint& point) = 0;
            //! Given a point in screen space, return the terrain position in world space.
            virtual AZ::Vector3 PickTerrain(const AzFramework::ScreenPoint& point) = 0;
            //! Return the terrain height given a world position in 2d (xy plane).
            virtual float TerrainHeight(const AZ::Vector2& position) = 0;
            //! Is the user holding a modifier key to move the manipulator space from local to world.
            virtual bool ShowingWorldSpace() = 0;
            //! Return the widget to use as the parent for the viewport context menu.
            virtual QWidget* GetWidgetForViewportContextMenu() = 0;

        protected:
            ~MainEditorViewportInteractionRequests() = default;
        };

        //! Type to inherit to implement MainEditorViewportInteractionRequests.
        using MainEditorViewportInteractionRequestBus = AZ::EBus<MainEditorViewportInteractionRequests, ViewportEBusTraits>;

        //! Editor entity requests to be made about the viewport.
        class EditorEntityViewportInteractionRequests
        {
        public:
            //! Given the current view frustum (viewport) return all visible entities.
            virtual void FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntities) = 0;

        protected:
            ~EditorEntityViewportInteractionRequests() = default;
        };

        using EditorEntityViewportInteractionRequestBus = AZ::EBus<EditorEntityViewportInteractionRequests, ViewportEBusTraits>;

        //! An interface to query editor modifier keys.
        class EditorModifierKeyRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Returns the current state of the keyboard modifier keys.
            virtual KeyboardModifiers QueryKeyboardModifiers() = 0;

        protected:
            ~EditorModifierKeyRequests() = default;
        };

        using EditorModifierKeyRequestBus = AZ::EBus<EditorModifierKeyRequests>;

        inline KeyboardModifiers QueryKeyboardModifiers()
        {
            KeyboardModifiers keyboardModifiers;
            EditorModifierKeyRequestBus::BroadcastResult(keyboardModifiers, &EditorModifierKeyRequestBus::Events::QueryKeyboardModifiers);
            return keyboardModifiers;
        }

        //! An interface to deal with time requests relating to viewports.
        //! @note The bus is global and not per viewport.
        class EditorViewportInputTimeNowRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

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
            //! Is the mouse over the viewport.
            virtual bool IsMouseOver() const = 0;
            //! Set the cursor style override.
            virtual void SetOverrideCursor(CursorStyleOverride cursorStyleOverride) = 0;
            //! Clear the cursor style override.
            virtual void ClearOverrideCursor() = 0;

        protected:
            ~ViewportMouseCursorRequests() = default;
        };

        //! Type to inherit to implement MainEditorViewportInteractionRequests.
        using ViewportMouseCursorRequestBus = AZ::EBus<ViewportMouseCursorRequests, ViewportEBusTraits>;
    } // namespace ViewportInteraction

    //! Utility function to return EntityContextId.
    inline AzFramework::EntityContextId GetEntityContextId()
    {
        auto entityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(entityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        return entityContextId;
    }

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
