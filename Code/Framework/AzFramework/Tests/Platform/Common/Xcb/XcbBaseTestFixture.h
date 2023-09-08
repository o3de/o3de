/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gtest/gtest.h>
#include <xcb/xcb.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/NativeUISystemComponent.h>

#include "MockXcbInterface.h"

namespace AzFramework
{
    // Sets up mock behavior for the xcb library, providing an xcb_connection_t that is returned from a call to xcb_connect
    class XcbBaseTestFixture
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        XcbBaseTestFixture();

        void SetUp() override;

        template<typename T>
        static xcb_generic_event_t MakeEvent(T event)
        {
            return *reinterpret_cast<xcb_generic_event_t*>(&event);
        }

    protected:
        testing::NiceMock<MockXcbInterface> m_interface;
        xcb_connection_t m_connection{};
        AZStd::unique_ptr<AzFramework::NativeUISystemComponent> m_nativeUiComponent;
    };
} // namespace AzFramework
