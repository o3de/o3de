/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
