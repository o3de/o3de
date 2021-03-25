/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
