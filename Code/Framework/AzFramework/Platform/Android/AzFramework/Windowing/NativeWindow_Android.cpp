/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzCore/Android/Utils.h>
#include <android/native_window.h>

namespace AzFramework
{
    class NativeWindowImpl_Android final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Android, AZ::SystemAllocator);
        NativeWindowImpl_Android() = default;
        ~NativeWindowImpl_Android() override = default;

        // NativeWindow::Implementation overrides...
        void InitWindowInternal(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
        uint32_t GetDisplayRefreshRate() const override;
        void SetRenderResolution(WindowSize resolution) override;


    private:
        ANativeWindow* m_nativeWindow = nullptr;
    };

    void NativeWindowImpl_Android::InitWindowInternal(
        [[maybe_unused]] const AZStd::string& title,
        const WindowGeometry& geometry,
        [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        m_nativeWindow = AZ::Android::Utils::GetWindow();

        int windowWidth = 0;
        int windowHeight = 0;
        // Window size is determinate by the OS. We cannot set it like in other systems.
        if (AZ::Android::Utils::GetWindowSize(windowWidth, windowHeight))
        {
            // Use the size returned by the OS (most likely the fullscreen size).
            m_width = static_cast<uint32_t>(windowWidth);
            m_height = static_cast<uint32_t>(windowHeight);
        }
        else
        {
            AZ_Error("NativeWindow", false, "Failed to get native window size");
            m_width = geometry.m_width;
            m_height = geometry.m_height;
        }

        if (m_nativeWindow)
        {
            // Set the resolution to the same size of the window.
            ANativeWindow_setBuffersGeometry(m_nativeWindow, m_width, m_height, ANativeWindow_getFormat(m_nativeWindow));
        }
    }

    NativeWindowHandle NativeWindowImpl_Android::GetWindowHandle() const
    {
        return reinterpret_cast<NativeWindowHandle>(m_nativeWindow);
    }

    uint32_t NativeWindowImpl_Android::GetDisplayRefreshRate() const
    {
        // [GFX TODO][GHI - 2678]
        // Using 60 for now until proper support is added
        return 60;
    }

    void NativeWindowImpl_Android::SetRenderResolution(WindowSize resolution)
    {
        WindowSize newResolution = resolution;
        if (m_nativeWindow)
        {
            // Fit the aspect ratio of the resolution so it matches the aspect ratio of the window
            // so when the window image is scaled by the OS compositor, it doesn't look stretched in any direction.
            float aspectRatio = static_cast<float>(m_width) / m_height;
            if (aspectRatio > 1.0f)
            {
                newResolution.m_width = AZStd::max(resolution.m_width, resolution.m_height);
                newResolution.m_height = newResolution.m_width / aspectRatio;
            }
            else
            {
                newResolution.m_height = AZStd::max(resolution.m_width, resolution.m_height);
                newResolution.m_width = newResolution.m_height * aspectRatio;
            }
            ANativeWindow_setBuffersGeometry(
                m_nativeWindow, newResolution.m_width, newResolution.m_height, ANativeWindow_getFormat(m_nativeWindow));
        }
        NativeWindow::Implementation::SetRenderResolution(newResolution);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class AndroidNativeWindowFactory 
        : public NativeWindow::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<NativeWindow::Implementation> Create() override
        {
            return AZStd::make_unique<NativeWindowImpl_Android>();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeNativeWindowImplementationFactory()
    {
        m_nativeWindowImplFactory = AZStd::make_unique<AndroidNativeWindowFactory>();
        AZ::Interface<NativeWindow::ImplementationFactory>::Register(m_nativeWindowImplFactory.get());
    }
} // namespace AzFramework
