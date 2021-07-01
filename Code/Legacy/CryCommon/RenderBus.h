/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    /**
    * This bus serves as a way for non-rendering systems to react to events
    * that occur inside the renderer. For now these events will probably be implemented by
    * things like CSystem and CryAction. In the future the idea is that these can be implemented
    * by a user's GameComponent.
    */
    class RenderNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~RenderNotifications() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        /**
        * This event gets posted at the beginning of CD3D9Renderer's FreeResources method, before the resources have been freed.
        */
        virtual void OnRendererFreeResources([[maybe_unused]] int flags) {};
    };

    using RenderNotificationsBus = AZ::EBus<RenderNotifications>;

    /**
    * This bus is used for renderer notifications that occur directly from the render thread
    * while scene rendering is occurring.  (In contrast, the RenderNotificationsBus above runs on the
    * main thread while the renderer is preparing the scene.)
    */
    class RenderThreadEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~RenderThreadEvents() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        /*
        * This hook enables per-frame render work at the beginning of the frame.
        *
        * This event is triggered when RT_RenderScene is called at the beginning of a frame.
        * The event will only be triggered on the render thread, if multithreaded rendering
        * is enabled.  And, it will only be triggered on a non-recursive scene render.
        */
        virtual void OnRenderThreadRenderSceneBegin() {}
    };

    using RenderThreadEventsBus = AZ::EBus<RenderThreadEvents>;

    /**
    * This bus is used for firing screenshot request to any rendering system.
    * The rendering system should implement its own screenshot function.
    */
    class RenderScreenshotRequests
        : public AZ::EBusTraits
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single; ///< EBusTraits overrides
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single; ///< EBusTraits overrides

        /** Take a screenshot and save it to a file.
        @param filepath the path where a the screenshot is saved.
        */
        virtual void WriteScreenshotToFile(const char* filepath) = 0;

        /** Take a screenshot and preserve it within a buffer
        */
        virtual void WriteScreenshotToBuffer() = 0;

        /** Fill a provided buffer with the render buffer
        @param imageBuffer The provided buffer to be filled
        */
        virtual bool CopyScreenshotToBuffer(unsigned char* imageBuffer, unsigned int width, unsigned int height) = 0;

    };

    using RenderScreenshotRequestBus = AZ::EBus<RenderScreenshotRequests>;

    class RenderScreenshotNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~RenderScreenshotNotifications() = default;

        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single; ///< EBusTraits overrides

        /** Notify waiting components that the requested screenshot is ready
        */
        virtual void OnScreenshotReady() = 0;
    };

    using RenderScreenshotNotificationBus = AZ::EBus<RenderScreenshotNotifications>;
}
