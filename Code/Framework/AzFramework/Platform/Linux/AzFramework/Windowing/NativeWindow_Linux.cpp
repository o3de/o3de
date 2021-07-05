/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Windowing/NativeWindow.h>

namespace AzFramework
{
    class NativeWindowImpl_Linux final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Linux, AZ::SystemAllocator, 0);
        NativeWindowImpl_Linux() = default;
        ~NativeWindowImpl_Linux() override = default;

        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title,
                        const WindowGeometry& geometry,
                        const WindowStyleMasks& styleMasks) override;
        NativeWindowHandle GetWindowHandle() const override;
    };

    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Linux();
    }

    void NativeWindowImpl_Linux::InitWindow([[maybe_unused]]const AZStd::string& title,
                                            const WindowGeometry& geometry,
                                            [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        m_width = geometry.m_width;
        m_height = geometry.m_height;
    }

    NativeWindowHandle NativeWindowImpl_Linux::GetWindowHandle() const
    {
        AZ_Assert(false, "NativeWindow not implemented for Linux");
        return nullptr;
    }

} // namespace AzFramework
