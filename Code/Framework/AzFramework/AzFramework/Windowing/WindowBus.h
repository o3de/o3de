/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    // These bus interfaces are designed to be used by any system that
    // implements any sort of window that provides a surface that a swapchain can
    // attach to.

    using NativeWindowHandle = void*; 

    //! A simple structure to contain window size.
    struct WindowSize
    {
        WindowSize() = default;

        WindowSize(const uint32_t width, const uint32_t height)
            : m_width(width)
            , m_height(height)
        {}

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        
        bool operator==(const WindowSize& rhs) const
        {
            return m_width == rhs.m_width && m_height == rhs.m_height;
        }

        bool operator!=(const WindowSize& rhs) const
        {
            return m_width != rhs.m_width || m_height != rhs.m_height;
        }
    };

    //! Options for resizing and moving the window.
    struct WindowPosOptions
    {
        //! This flag will allow the window to be resized bigger than the screen width or height.
        //! The default setting of false will clamp the window size to the maximum possible size that can
        //! fit on the screen, independent of window position. A portion of the window still may not be
        //! visible, but the window has the ability to fit fully on the screen.
        bool m_ignoreScreenSizeLimit = false;
    };

    //! Bus for sending requests to any kind of window.
    //! It could be a NativeWindow or an Editor window.
    class WindowRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowRequests() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = NativeWindowHandle;

        //! For platforms that support it, set the title of the window.
        virtual void SetWindowTitle(const AZStd::string& title) = 0;

        //! Get the client area size. This is the size that can be rendered to.
        //! On some platforms this may not be the correct size until Activate is called.
        virtual WindowSize GetClientAreaSize() const = 0;

        //! Get the maximum supported client area size for this window.
        //! This can return different sizes depending on which monitor the window is on.
        //! On some platforms this may not be the correct size until Activate is called.
        virtual WindowSize GetMaximumClientAreaSize() const
        {
            AZ_Assert(false, "GetMaximumClientAreaSize() not supported.");
            return { AZStd::numeric_limits<uint32_t>::max(), AZStd::numeric_limits<uint32_t>::max() };
        }

        //! Set the client area size. This is the size that can be rendered to.
        //! \param[in] clientAreaSize Size of the client area in pixels
        //! \param[in] options Options for resizing and moving the window.
        virtual void ResizeClientArea(WindowSize clientAreaSize, const WindowPosOptions& options) = 0;

        //! Does this platform support window resizing.
        //! Generally desktop platforms support resizing, mobile platforms don't.
        virtual bool SupportsClientAreaResize() const = 0;

        //! Enable custom render resolution which is different than client area size.
        //! If custom render resolution is disabled, the render resolution is same as client area size
        AZ_DEPRECATED_MESSAGE("Custom resolution is always enabled now. No need to use this function anymore.")
        void SetEnableCustomizedResolution([[maybe_unused]] bool enable){}

        //! If the custom render resolution is enabled.
        AZ_DEPRECATED_MESSAGE("Custom resolution is always enabled now. No need to use this function anymore.")
        bool IsCustomizedResolutionEnabled() const { return true; }

        //! Get the render resolution.
        //! If customized resolution is not enabled, it would return client area size
        virtual WindowSize GetRenderResolution() const = 0;

        //! Set render resolution for the window.
        //! It should only be called when customized resolution is enabled. 
        virtual void SetRenderResolution(WindowSize resolution) = 0;

        //! Get the full screen state of the window.
        //! \return True if the window is currently in full screen, false otherwise.
        virtual bool GetFullScreenState() const = 0;

        //! Set the full screen state of the window.
        //! \param[in] fullScreenState The full screen state to set.
        virtual void SetFullScreenState(bool fullScreenState) = 0;

        //! Can the full screen state of the window be changed/toggled?
        //! \return True if the window can enter/exit full screen, false otherwise.
        virtual bool CanToggleFullScreenState() const = 0;

        //! Toggle the full screen state of the window.
        virtual void ToggleFullScreenState() = 0;

        //! Returns a scalar multiplier representing how many dots-per-inch this window has, compared
        //! to a "standard" value of 96, the default for Windows in a DPI unaware setting. This can
        //! be used to scale user interface elements to ensure legibility on high density displays.
        virtual float GetDpiScaleFactor() const = 0;

        //! Returns the sync interval which tells the drivers the number of v-blanks to synchronize with
        virtual uint32_t GetSyncInterval() const = 0;

        //! Sets the sync interval which tells the drivers the number of v-blanks to synchronize with
        //! Returns if the sync interval was successfully set
        virtual bool SetSyncInterval(uint32_t newSyncInterval) = 0;

        //! Returns the refresh rate of the main display
        virtual uint32_t GetDisplayRefreshRate() const = 0;
    };
    using WindowRequestBus = AZ::EBus<WindowRequests>;

    //! Bus for listening for notifications from a window.
    //! It could be a NativeWindow or an Editor window.
    class WindowNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowNotifications() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = NativeWindowHandle;

        //! This is called once when the window is Activated and also called if the user resizes the window.
        virtual void OnWindowResized(uint32_t width, uint32_t height) { AZ_UNUSED(width); AZ_UNUSED(height); };
        
        //! This is called when the window's desired render resolution is changed.
        virtual void OnResolutionChanged(uint32_t width, uint32_t height) { AZ_UNUSED(width); AZ_UNUSED(height); };

        //! This is called if the window's underyling DPI scaling factor changes.
        virtual void OnDpiScaleFactorChanged(float dpiScaleFactor) { AZ_UNUSED(dpiScaleFactor); }

        //! This is called when the fullscreen mode of the window changes.
        virtual void OnFullScreenModeChanged([[maybe_unused]] bool fullscreen) {}

        //! This is called when the window is deactivated from code or if the user closes the window.
        virtual void OnWindowClosed() {};

        //! This is called when vsync interval is changed.
        virtual void OnVsyncIntervalChanged(uint32_t interval) { AZ_UNUSED(interval); };

        //! This is called if the main display's refresh rate changes
        virtual void OnRefreshRateChanged([[maybe_unused]] uint32_t refreshRate) {}
    };
    using WindowNotificationBus = AZ::EBus<WindowNotifications>;

    //! Bus used by the NativeWindow class to send exclusive full screen related requests to the renderer.
    //! 
    //! This is currently only needed if the Windows DX12 renderer does not support tearing, in which case
    //! the renderer will connect to this bus and assume responsibility for maintaining and transitioning
    //! the full screen state of the corresponding NativeWindowHandle. On platforms where full screen state
    //! is handled exclusively by AzFramework::NativeWindow (eg. Mac) this will never be used, and nor will
    //! it ever be used for platforms that cannot perform full screen transitions at all (eg. iOS/Android).
    //! 
    //! Do not call these directly, use the WindowRequests bus instead to get or set the full screen state.
    class ExclusiveFullScreenRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~ExclusiveFullScreenRequests() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = NativeWindowHandle;

        //! This will be called when the NativeWindow needs to know whether the renderer prefers to use exclusive full screen mode.
        //!
        //! Do not call directly, use WindowRequests::GetFullScreenState instead to get the current full screen state of a window.
        //!
        //! \return True if the renderer prefers to use their exclusive full screen mode, false otherwise.
        virtual bool IsExclusiveFullScreenPreferred() const { return false; }

        //! This will be called when the full screen state of a NativeWindow is requested using WindowRequests::GetFullScreenState,
        //! but only if the renderer prefers to use their exclusive full screen mode (ie. IsExclusiveFullScreenPreferred() == true).
        //!
        //! Do not call directly, use WindowRequests::GetFullScreenState instead to get the current full screen state of a window.
        //!
        //! \return True if the renderer prefers to use their exclusive full screen mode and it is currently true, false otherwise.
        virtual bool GetExclusiveFullScreenState() const { return false; }

        //! This will be called when the full screen state of a NativeWindow is set using WindowRequests::SetFullScreenState,
        //! but only if the renderer prefers to use exclusive full screen mode (ie. IsExclusiveFullScreenPreferred() == true).
        //!
        //! Do not call directly, use WindowRequests::GetFullScreenState instead to set the full screen state of a window.
        //!
        //! \param[in] fullScreenState The full screen state that was passed to WindowRequests::SetFullScreenState.
        //! \return True if the renderer prefers to use exclusive full screen mode and a transition happened, false otherwise.
        virtual bool SetExclusiveFullScreenState([[maybe_unused]]bool fullScreenState) { return false; }
    };
    using ExclusiveFullScreenRequestBus = AZ::EBus<ExclusiveFullScreenRequests>;

    //! The WindowSystemRequestBus is a broadcast bus for sending requests to the window system.
    class WindowSystemRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowSystemRequests() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Get the window handle for the default window
        virtual NativeWindowHandle GetDefaultWindowHandle() = 0;
    };
    using WindowSystemRequestBus = AZ::EBus<WindowSystemRequests>;

    //! The WindowSystemNotificationBus is used to broadcast an event whenever a new window is created.
    class WindowSystemNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowSystemNotifications() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! A notification that a new window was created with the given window ID
        virtual void OnWindowCreated(NativeWindowHandle windowHandle) = 0;
    };
    using WindowSystemNotificationBus = AZ::EBus<WindowSystemNotifications>;
} // namespace AzFramework
