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

#include <AzCore/PlatformDef.h>

// include standard core headers
#include "Config.h"
#include "Macros.h"
#include "MemoryCategoriesCore.h"

// standard includes
#include <math.h>
#include <string.h>
#include <stdlib.h>
//#include <limits.h>
#include <limits>
#include <assert.h>
#include <cstdio> // stdio.h doesn't work for std::rename and std::remove on all platforms
#include <float.h>
#include <stdarg.h>
#include <wchar.h>
//#include <complex.h>

// include the system
#include "MCoreSystem.h"
#include "MemoryManager.h"
