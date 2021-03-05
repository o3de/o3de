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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <vector>

#define CRYFONT_EXPORTS

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

