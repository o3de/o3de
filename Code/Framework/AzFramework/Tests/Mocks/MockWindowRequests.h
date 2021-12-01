/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Windowing/WindowBus.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class MockWindowRequests : public AzFramework::WindowRequestBus::Handler
    {
    public:
        void Connect(AzFramework::NativeWindowHandle handle)
        {
            AzFramework::WindowRequestBus::Handler::BusConnect(handle);
        }
        void Disconnect()
        {
            AzFramework::WindowRequestBus::Handler::BusDisconnect();
        }

        // AzFramework::WindowRequestBus overrides ...
        MOCK_METHOD1(SetWindowTitle, void(const AZStd::string&));
        MOCK_CONST_METHOD0(GetClientAreaSize, AzFramework::WindowSize());
        MOCK_METHOD1(ResizeClientArea, void(AzFramework::WindowSize clientAreaSize));
        MOCK_CONST_METHOD0(GetFullScreenState, bool());
        MOCK_METHOD1(SetFullScreenState, void(bool));
        MOCK_CONST_METHOD0(CanToggleFullScreenState, bool());
        MOCK_METHOD0(ToggleFullScreenState, void());
        MOCK_CONST_METHOD0(GetDpiScaleFactor, float());
        MOCK_CONST_METHOD0(GetSyncInterval, uint32_t());
        MOCK_METHOD1(SetSyncInterval, bool(uint32_t));
        MOCK_CONST_METHOD0(GetDisplayRefreshRate, uint32_t());
    };
} // namespace UnitTest
