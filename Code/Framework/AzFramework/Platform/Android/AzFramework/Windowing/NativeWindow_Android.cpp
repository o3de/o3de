/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Windowing/NativeWindow.h>
#include <AzCore/Android/Utils.h>
#include <android/native_window.h>

namespace AzFramework
{
    class NativeWindowImpl_Android final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Android, AZ::SystemAllocator, 0);
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

    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Android();
    }

    void NativeWindowImpl_Android::InitWindow([[maybe_unused]]const AZStd::string& title,
                                              const WindowGeometry& geometry,
                                              [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        m_nativeWindow = AZ::Android::Utils::GetWindow();

        m_width = geometry.m_width;
        m_height = geometry.m_height;

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
} // namespace AzFramework
