/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/ViewGroup.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
    } //namespace RHI

    namespace RPI
    {
        //! Manages ViewportContexts, which can be created and looked up by name.
        //! Contexts are mapped to a stack of default Views which can be used to
        //! push camera state to an arbitrary ViewportContext.
        //!
        //! All methods are thread-safe, but the underlying ViewportContext may not be.
        class ATOM_RPI_PUBLIC_API ViewportContextRequestsInterface
        {
        public:
            AZ_RTTI(ViewportContextRequestsInterface, "{FDB82F02-7021-433B-AAD3-25B97EC69962}");

            //! Parameters for creating a ViewportContext.
            struct CreationParameters
            {
                //! The hardware device to bind the native window to, must not be null.
                RHI::Device* device = nullptr;
                //! The native window to create a swap chain for, must be valid.
                AzFramework::NativeWindowHandle windowHandle = {};
                //! The scene to render, optional.
                ScenePtr renderScene;
                //! The ID to use, if specified. This ID must be unique to this ViewportContext.
                //! If an invalid ID is specified (the default) then an ID will be assigned automatically.
                AzFramework::ViewportId id = AzFramework::InvalidViewportId;
            };

            //! Gets the name of the default, primary ViewportContext, for common single-viewport scenarios.
            virtual AZ::Name GetDefaultViewportContextName() const = 0;

            //! Get the ViewportConext which has default ViewportContext name
            virtual ViewportContextPtr GetDefaultViewportContext() const = 0;

            //! Creates a ViewportContext and registers it by name.
            //! There may only be one context registered to a given name at any time.
            //! The ViewportContext will be automatically assigned a View from the stack registered to this context name.
            //! The ViewportContextManager does *not* take ownership of this ViewportContext,
            //! its lifecycle is the responsibility of the caller. ViewportContexts shall automatically unregister
            //! when they are destroyed.
            virtual ViewportContextPtr CreateViewportContext(const Name& contextName, const CreationParameters& params) = 0;
            //! Gets the ViewportContext registered to the given name, if any.
            virtual ViewportContextPtr GetViewportContextByName(const Name& contextName) const = 0;
            //! Gets the registered ViewportContext with the corresponding ID, if any.
            virtual ViewportContextPtr GetViewportContextById(AzFramework::ViewportId id) const = 0;
            //! Gets the registered ViewportContext with matching RPI::Scene, if any.
            //! This function will return the first result.
            virtual ViewportContextPtr GetViewportContextByScene(const Scene* scene) const = 0;
            //! Maps a ViewportContext to a new name, inheriting the View stack (if any) registered to that context name.
            //! This can be used to switch "default" viewports by registering a viewport with the default ViewportContext name
            //! but note that only one ViewportContext can be mapped to a context name at a time.
            virtual void RenameViewportContext(ViewportContextPtr viewportContext, const Name& newContextName) = 0;
            //! Enumerates all registered ViewportContexts, calling visitorFunction once for each registered viewport.
            virtual void EnumerateViewportContexts(AZStd::function<void(ViewportContextPtr)> visitorFunction) = 0;

            //! Pushes a view group to the stack for a given context name. A view group manages all stereoscopic and non-stereoscopic views.
            //! The Views within a View Group must be declared a camera by having the View::UsageFlags::UsageCamera usage flag set.
            //! This View Group will be registered as the context's pipeline's default view group until the top of the camera stack changes.
            virtual void PushViewGroup(const Name& contextName, ViewGroupPtr viewGroup) = 0;

            //! Pops a view group off the stack for a given context name. A view group manages all stereoscopic and non-stereoscopic views.
            //! Returns true if the camera was successfully removed or false if the view wasn't removed,
            //! either because it wasn't found within any existing view groups or its removal was not allowed.
            //! @note The default camera's view group for a given viewport may not be removed from the view stack.
            //! You must push an additional camera view groups to override the default view group instead.
            virtual bool PopViewGroup(const Name& contextName, ViewGroupPtr viewGroup) = 0;

            //! Gets the view group currently registered to a given context, assuming the context exists.
            //! This will be null if there is no registered ViewportContext and no view groups have been pushed for this context name.
            virtual ViewGroupPtr GetCurrentViewGroup(const Name& contextName) = 0;
        };

        using ViewportContextRequests = AZ::Interface<ViewportContextRequestsInterface>;

        class ViewportContextManagerNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy  = EBusAddressPolicy::Single;

            virtual void OnViewportContextAdded(ViewportContextPtr viewportContext){AZ_UNUSED(viewportContext);}
            virtual void OnViewportContextRemoved(AzFramework::ViewportId viewportId){AZ_UNUSED(viewportId);}
        };

        using ViewportContextManagerNotificationsBus = AZ::EBus<ViewportContextManagerNotifications>;

        class ViewportContextNotifications
        {
        public:
            //! Called when the underlying native window size changes for a given viewport context.
            virtual void OnViewportSizeChanged(AzFramework::WindowSize size){AZ_UNUSED(size);}
            //! Called when the window DPI scaling changes for a given viewport context.
            virtual void OnViewportDpiScalingChanged(float dpiScale){AZ_UNUSED(dpiScale);}
            //! Called when the active view for a given viewport context name changes.
            virtual void OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view){AZ_UNUSED(view);}
            //! Called when the viewport is to be rendered.
            //! Add draws to this functions if they only need to be rendered to this viewport. 
            virtual void OnRenderTick(){};
            //! Called as a sync point for any render jobs in flight
            virtual void WaitForRender(){};

        protected:
            ~ViewportContextNotifications() = default;
        };

        class NotifyByViewportNameTraits
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy  = EBusAddressPolicy::ById;
            using BusIdType = Name;
            using EventQueueMutexType = AZStd::mutex;
        };

        class NotifyByViewportIdTraits
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy  = EBusAddressPolicy::ById;
            using BusIdType = AzFramework::ViewportId;
            using EventQueueMutexType = AZStd::mutex;
        };

        using ViewportContextNotificationBus = AZ::EBus<ViewportContextNotifications, NotifyByViewportNameTraits>;
        using ViewportContextIdNotificationBus = AZ::EBus<ViewportContextNotifications, NotifyByViewportIdTraits>;
    } // namespace RPI
} // namespace AZ
