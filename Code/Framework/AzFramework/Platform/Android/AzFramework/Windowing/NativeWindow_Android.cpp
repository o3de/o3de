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
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
        uint32_t GetDisplayRefreshRate() const override;
    private:
        ANativeWindow* m_nativeWindow = nullptr;
    };

    void NativeWindowImpl_Android::InitWindow([[maybe_unused]]const AZStd::string& title,
                                              const WindowGeometry& geometry,
                                              [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        m_nativeWindow = AZ::Android::Utils::GetWindow();

        int windowWidth = 0;
        int windowHeight = 0;
        if (AZ::Android::Utils::GetWindowSize(windowWidth, windowHeight))
        {
            // Use native window size from the device if available
            m_width = static_cast<uint32_t>(windowWidth);
            m_height = static_cast<uint32_t>(windowHeight);
        }
        else
        {
            m_width = geometry.m_width;
            m_height = geometry.m_height;
        }

        if (m_nativeWindow)
        {
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
