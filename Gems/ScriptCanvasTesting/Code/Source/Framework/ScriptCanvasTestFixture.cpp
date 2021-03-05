/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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