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

#include "MockXcbInterface.h"

namespace AzFramework
{
    // Sets up mock behavior for the xcb library, providing an xcb_connection_t that is returned from a call to xcb_connect
    class XcbBaseTestFixture
        : public testing::Test
    {
    public:
        void SetUp() override;

    protected:
        testing::NiceMock<MockXcbInterface> m_interface;
        xcb_connection_t m_connection{};
    };
} // namespace AzFramework
