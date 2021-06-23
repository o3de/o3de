/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

namespace ScriptCanvasDeveloper
{
    typedef AZStd::string AutomationStateModelId;
    namespace StateModelIds
    {
        static const char* GraphCanvasId = "::GraphCanvasId";
        static const char* ScriptCanvasId = "::ScriptCanvasId";
        static const char* ViewId = "::ViewId";
        static const char* MinorStep = "::MinorStep";
    }
}
