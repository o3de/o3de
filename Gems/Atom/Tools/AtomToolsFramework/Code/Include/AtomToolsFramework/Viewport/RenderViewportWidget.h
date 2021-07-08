/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
#include <QElapsedTimer>
#include <Atom/RPI.Public/Base.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Viewport/ViewportControllerInterface.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <AzCore/Component/TickBus.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>

namespace AtomToolsFramework
{
    //! The RenderViewportWidget class is a Qt wrapper around an Atom viewport.
    //! RenderViewportWidget renders to an internal window using RPI::ViewportContext
    //! and delegates input via its internal ViewportControllerList.
    //! @see AZ::RPI::ViewportContext for Atom's API for setting up 
    class RenderViewportWidget
        : public QWidget
        , public AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler
        , public AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler
        , public AzFramework::WindowRequestBus::Handler
        , protected AzFramework::InputChannelEventListener
        , protected AZ::TickBus::Handler
    {
    public:
        //! Creates a RenderViewportWidget.
        //! Requires the Atom RPI to be initialized in order
        //! to internally construct an RPI::ViewportContext.
        //! If initializeViewportContext is set to false, nothing will be displayed on-screen until InitiliazeViewportContext is called.
        explicit RenderViewportWidget(QWidget* parent = nullptr, bool shouldInitializeViewportContext = true);
        ~RenderViewportWidget();

        //! Initializes the underlying ViewportContext, if it hasn't already been.
        //! If id is specified, the target ViewportContext will be overridden.
        //! NOTE: ViewportContext IDs must be unique.
        //! Returns true if the ViewportContext is available
        //! (i.e. GetViewportContext will return a valid pointer).
        bool InitializeViewportContext(AzFramework::ViewportId id = AzFramework::InvalidViewportId);
        //! Gets the name associated with this viewport's ViewportContext.
        //! This context name can be used to adjust the current Camera
        //! independently of the underlying viewport.
        AZ::Name GetCurrentContextName() const;
        //! Sets the name associated with this viewport's ViewportContext.
        //! The viewport may inherit a new Camera from the new context name.
        void SetCurrentContextName(const AZ::Name& contextName);
        //! Gets this Viewport's unique idenitifer.
        //! @see AzFramework::ViewportRequestBus
        AzFramework::ViewportId GetId() const;
        //! Gets the controller list responsible for handling this viewport's input.
        //! ViewportControllerLists may be shared between viewports, so long as none
        //! of the lists contain SingleViewportControllers.
        AzFramework::ViewportControllerListPtr GetControllerList();
        AzFramework::ConstViewportControllerListPtr GetControllerList() const;
        //! Sets the controller list responsible for handling this viewport's input.
        //! ViewportControllerLists may be shared between viewports, so long as none
        //! of the lists contain SingleViewportControllers.
        void SetControllerList(AzFramework::ViewportControllerListPtr controllerList);
        //! Locks the target render resolution of this viewport to a given resolution.
        //! This can be used to ensure a uniform resolution for testing.
        void LockRenderTargetSize(uint32_t width, uint32_t height);
        //! Allows this viewport to be freely resized.
        void UnlockRenderTargetSize();
        //! Gets the underlying ViewportContext associated with this RenderViewportWidget.
        AZ::RPI::ViewportContextPtr GetViewportContext();
        AZ::RPI::ConstViewportContextPtr GetViewportContext() const;
        //! Creates an AZ::RPI::ScenePtr for the given scene and assigns it to the current ViewportContext.
        //! If useDefaultRenderPipeline is specified, this will initialize the scene with a rendering pipeline.
        void SetScene(const AZStd::shared_ptr<AzFramework::Scene>& scene, bool useDefaultRenderPipeline = true);
        //! Gets the default camera that's been automatically registered to our ViewportContext.
        AZ::RPI::ViewPtr GetDefaultCamera();
        AZ::RPI::ConstViewPtr GetDefaultCamera() const;

        // AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler ...
        AzFramework::CameraState GetCameraState() override;
        bool GridSnappingEnabled() override;
        float GridSize() override;
        bool ShowGrid() override;
        bool AngleSnappingEnabled() override;
        float AngleStep() override;
        AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) override;
        AZStd::optional<AZ::Vector3> ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition, float depth) override;
        AZStd::optional<AzToolsFramework::ViewportInteraction::ProjectedViewportRay> ViewportScreenToWorldRay(
            const AzFramework::ScreenPoint& screenPosition) override;
        float DeviceScalingFactor() override;

        //! Set interface for providing viewport specific settings (e.g. snapping properties).
        void SetViewportSettings(const AzToolsFramework::ViewportInteraction::ViewportSettings* viewportSettings);

        // AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler ...
        void BeginCursorCapture() override;
        void EndCursorCapture() override;
        AzFramework::ScreenPoint ViewportCursorScreenPosition() override;
        AZStd::optional<AzFramework::ScreenPoint> PreviousViewportCursorScreenPosition() override;
        bool IsMouseOver() const override;

        // AzFramework::WindowRequestBus::Handler ...
        void SetWindowTitle(const AZStd::string& title) override;
        AzFramework::WindowSize GetClientAreaSize() const override;
        void ResizeClientArea(AzFramework::WindowSize clientAreaSize) override;
        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;
        bool CanToggleFullScreenState() const override;
        void ToggleFullScreenState() override;
        float GetDpiScaleFactor() const override;

    protected:
        // AzFramework::InputChannelEventListener ...
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        // AZ::TickBus::Handler ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // QWidget ...
        void resizeEvent(QResizeEvent *event) override;
        bool event(QEvent* event) override;
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;

    private:
        void SendWindowResizeEvent();
        bool CanInputGrantFocus(const AzFramework::InputChannel& inputChannel) const;

        // The underlying ViewportContext, our entry-point to the Atom RPI.
        AZ::RPI::ViewportContextPtr m_viewportContext;
        // Rather than handling input and supplemental rendering within the viewport or a subclass,
        // we provide this controller list to allow handlers to listen for input and update events.
        AzFramework::ViewportControllerListPtr m_controllerList;
        // The default camera for our viewport i.e. the one used when a camera entity hasn't been activated.
        AZ::RPI::ViewPtr m_defaultCamera;
        // Our viewport-local auxgeom pipeline for supplemental rendering.
        AZ::RPI::AuxGeomDrawPtr m_auxGeom;
        // Used to keep track of a pending resize event to avoid initialization before window activate.
        bool m_windowResizedEvent = false;
        // Tracks whether the cursor is currently over our viewport, used for mouse input event book-keeping.
        bool m_mouseOver = false;
        // The last recorded mouse position, in local viewport screen coordinates.
        QPointF m_mousePosition;
        // Captures the time between our render events to give controllers a time delta.
        QElapsedTimer m_renderTimer;
        // The time of the last recorded tick event from the system tick bus.
        AZ::ScriptTimePoint m_time;
        // Whether the Viewport is currently hiding and capturing the cursor position.
        bool m_capturingCursor = false;
        // The last known position of the mouse cursor, if one is available.
        AZStd::optional<QPoint> m_lastCursorPosition;
        // The viewport settings (e.g. grid snapping, grid size) for this viewport.
        const AzToolsFramework::ViewportInteraction::ViewportSettings* m_viewportSettings = nullptr;
    };
} //namespace AtomToolsFramework
