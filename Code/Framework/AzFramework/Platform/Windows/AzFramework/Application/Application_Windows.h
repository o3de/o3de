/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Application/Application.h>
#include <AzFramework/API/ApplicationAPI_Windows.h>
#include <AzCore/Memory/SystemAllocator.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationWindows
        : public Application::Implementation
        , public WindowsLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationWindows, AZ::SystemAllocator);
        ApplicationWindows();
        ~ApplicationWindows() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // WindowsLifecycleEvents
        void OnMinimized() override; // Suspend
        void OnRestored() override;  // Resume

        void OnKillFocus() override; // Constrain
        void OnSetFocus() override;  // Unconstrain

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    protected:
        void ProcessSystemEvent(MSG& msg);

    private:
        ApplicationLifecycleEvents::Event m_lastEvent;
    };
} // namespace AzFramework
