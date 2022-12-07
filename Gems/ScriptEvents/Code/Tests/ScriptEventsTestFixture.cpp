/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventsTestFixture.h"
#include "ScriptEventsTestApplication.h"

namespace ScriptEventsTests
{
    ScriptEventsTests::Application* ScriptEventsTestFixture::s_application = nullptr;

    ScriptEventsTests::Application* ScriptEventsTestFixture::GetApplication()
    {
        return s_application;
    }

}

