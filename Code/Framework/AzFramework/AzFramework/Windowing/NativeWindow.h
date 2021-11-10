/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AzFramework
{
    //! A simple structure to contain window geometry.
    //! The defaults here reflect the defaults when creating a new
    //! WindowGeometry object. Different window implementations may 
    //! have their own separate default window sizes.
    struct WindowGeometry
    {
        WindowGeometry() = default;

        WindowGeometry(const uint32_t posX, const uint32_t posY, const uint32_t width, const uint32_t height)
            : m_posX(posX)
            , m_posY(posY)
            , m_width(width)
            , m_height(height)
        {}

        uint32_t m_posX = 0;
        uint32_t m_posY = 0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

    //! A simple structure to encapsulate different native window style masks.
    struct WindowStyleMasks
    {
        //! Platform agnostic window style bitmasks.
        static constexpr uint32_t WINDOW_STYLE_BORDERED     = 0x0001; //!< Should the window have a border?
        static constexpr uint32_t WINDOW_STYLE_RESIZEABLE   = 0x0002; //!< Should the window be resizeable? (Implies WINDOW_STYLE_BORDERED)
        static constexpr uint32_t WINDOW_STYLE_TITLED       = 0x0004; //!< Should the window have a title bar? (Implies WINDOW_STYLE_BORDERED)
        static constexpr uint32_t WINDOW_STYLE_TITLED_MENU  = 0x0008; //!< Should the window have a title bar with a menu? (Implies WINDOW_STYLE_TITLED)
        
        static constexpr uint32_t WINDOW_STYLE_CLOSABLE     = 0x0010; //!< Should the window have a close button? (Implies WINDOW_STYLE_TITLED_MENU)
        static constexpr uint32_t WINDOW_STYLE_MAXIMIZE     = 0x0020; //!< Should the window have a maximize button? (Implies WINDOW_STYLE_TITLED_MENU)
        static constexpr uint32_t WINDOW_STYLE_MINIMIZE     = 0x0040; //!< Should the window have a minimize button? (Implies WINDOW_STYLE_TITLED_MENU)

        //! Default constructor.
        WindowStyleMasks() = default;

        //! Constructor (deliberately not explicit).
        //! \param[in] platformAgnosticStyleMask Bitmask of platform agnostic window style flags.
        //! \param[in] platformSpecificStyleMask Bitmask of platform specific window style flags.
        WindowStyleMasks(const uint32_t platformAgnosticStyleMask,
                         const uint32_t platformSpecificStyleMask = 0)
            : m_platformAgnosticStyleMask(platformAgnosticStyleMask)
            , m_platformSpecificStyleMask(platformSpecificStyleMask)
        {}

        //! Bitmask of platform agnostic window style flags that will be translated
        //! to their equivalent platform specific window style flags (if it exists),
        //! and then combined with m_platformSpecificStyleMask before being applied.
        uint32_t m_platformAgnosticStyleMask = 0;

        //! Bitmask of platform specific window style flags that will be combined
        //! with any flags from m_platformAgnosticStyleMask before being applied.
        uint32_t m_platformSpecificStyleMask = 0;
    };

    //! Provides a basic window.
    //!
    //! This is mainly designed to be used as a base window for rendering.
    //! The window provides just a simple surface and handles implementation
    //! details for each platform. 
    //! 
    //! Window events are pumped via the AzFramework's application. No event messaging
    //! or message translation/dispatch needs to be handled by the underlying implementation.
    //!
    //! Multiple NativeWindows are supported by this system. This is impractical for most standalone 
    //! game applications, and unnecessary for some platforms, but still possible.
    //!
    //! The Window implementation will be created when the NativeWindow is constructed and the
    //! platform specific native window will be created (if it doesn't already exist).
    //! On platforms that support it, the window will become visible when Activate is called and
    //! hidden when Deactivate is called. On other platforms the window becomes visible on construction
    //! and hidden on destruction.
    //! To provide a consistent API to the client across platforms:
    //! - Calling Activate (when not active) will result in the OnWindowResized notification
    //! - Calling Deactivate (when active) will result in the OnWindowClosed notification
    //! - destroying the NativeWindow when active will result in Deactivate being called
    class NativeWindow final
        : public WindowRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindow, AZ::SystemAllocator, 0);

        //! Constructor
        //! \param[in] title The title of the window (may or may not be displayed depending on the platform).
        //! \param[in] geometry The geometry used to set the initial position and size of the window.
        //! \param[in] styleMasks The style masks applied to the window on creation. If none are specified,
        //!                       the default window style of the underlying platform will be used instead.
        NativeWindow(const AZStd::string& title,
                     const WindowGeometry& geometry,
                     const WindowStyleMasks styleMasks = {});
        ~NativeWindow() override;

        //! Activate the window.
        //! The window will be visible after this call (on some platforms it will be also visible before activation).
        void Activate();

        //! Deactivate the window.
        //! This will result in the OnWindowClosed notification being sent.
        //! On some platforms this will hide the window. On others the window will remain visible.
        void Deactivate();

        bool IsActive() const;

        //! Get the native window handle. This is used as the bus id for the WindowRequestBus and WindowNotificationBus
        NativeWindowHandle GetWindowHandle() const { return m_pimpl->GetWindowHandle(); }

        // WindowRequestBus::Handler overrides ...
        void SetWindowTitle(const AZStd::string& title) override;
        WindowSize GetClientAreaSize() const override;
        void ResizeClientArea(WindowSize clientAreaSize) override;
        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;
        bool CanToggleFullScreenState() const override;
        void ToggleFullScreenState() override;
        float GetDpiScaleFactor() const override;
        uint32_t GetSyncInterval() const override;
        uint32_t GetDisplayRefreshRate() const override;

        //! Get the full screen state of the default window.
        //! \return True if the default window is currently in full screen, false otherwise.
        static bool GetFullScreenStateOfDefaultWindow();

        //! Set the full screen state of the default window.
        //! \param[in] fullScreenState The full screen state to set.
        static void SetFullScreenStateOfDefaultWindow(bool fullScreenState);

        //! Can the full screen state of the default window be changed/toggled?
        //! \return True if the default window can enter/exit full screen, false otherwise.
        static bool CanToggleFullScreenStateOfDefaultWindow();

        //! Toggle the full screen state of the default window.
        static void ToggleFullScreenStateOfDefaultWindow();

        //! The NativeWindow implementation.
        //! Extend this to provide windowing capabilities per platform.
        //! It's expected that only one Implementation::Create method will be available per-platform.
        class Implementation
        {
        public:
            static Implementation* Create();
            virtual ~Implementation() = default;

            virtual void InitWindow(const AZStd::string& title,
                                    const WindowGeometry& geometry,
                                    const WindowStyleMasks& styleMasks) = 0;

            virtual void Activate();
            virtual void Deactivate();
            bool IsActive() const { return m_activated; }

            virtual NativeWindowHandle GetWindowHandle() const = 0;

            virtual void SetWindowTitle(const AZStd::string& title);
            virtual WindowSize GetClientAreaSize() const;
            virtual void ResizeClientArea(WindowSize clientAreaSize);
            virtual bool GetFullScreenState() const;
            virtual void SetFullScreenState(bool fullScreenState);
            virtual bool CanToggleFullScreenState() const;
            virtual float GetDpiScaleFactor() const;
            virtual uint32_t GetDisplayRefreshRate() const;

        protected:
            uint32_t m_width = 0;
            uint32_t m_height = 0;
            bool m_activated = false;
        };

    private:
        AZStd::unique_ptr<Implementation> m_pimpl;
    };
} // namespace AzFramework
