/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "ScriptCanvasTestFixture.h"

namespace ScriptCanvasTests
{
    Application* ScriptCanvasTestFixture::s_application = nullptr;
    AZStd::atomic_bool ScriptCanvasTestFixture::s_asyncOperationActive = {};
    bool ScriptCanvasTestFixture::s_setupSucceeded = false;
}
