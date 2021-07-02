/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <vector>

#define ATOMFONT_EXPORTS

#include <platform.h>

#include <IFont.h>

#include <ILog.h>
#include <IConsole.h>
#include <IRenderer.h>
#include <CrySizer.h>

#define USE_NULLFONT

#if defined(DEDICATED_SERVER)
#define USE_NULLFONT_ALWAYS 1
#endif

