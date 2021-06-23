/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "ScriptCanvasTestFixture.h"

namespace ScriptCanvasTests
{
    Application* ScriptCanvasTestFixture::s_application = nullptr;
    UnitTest::AllocatorsBase ScriptCanvasTestFixture::s_allocatorSetup = {};
    AZStd::atomic_bool ScriptCanvasTestFixture::s_asyncOperationActive = {};
    bool ScriptCanvasTestFixture::s_setupSucceeded = false;
}
