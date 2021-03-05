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

// Description : Defines OS version and GPU family which can then be used to query if
//               certain API calls are enabled/disabled.

#include "RenderDll_precompiled.h"
#include "GLCommon.hpp"

namespace NCryMetal
{
    bool s_isIosMinVersion9_0 = false;
    bool s_isOsxMinVersion10_11 = false;
    bool s_isIOSGPUFamily3 = false;
};
