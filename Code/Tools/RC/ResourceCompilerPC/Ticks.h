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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_TICKS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_TICKS_H
#pragma once



#define TICKS_PER_SECOND (30)                       //the maximum keyframe value is 4800Hz per second
#define SECONDS_PER_TICK (1.0f / 30.0f)       //the maximum keyframe value is 4800Hz per second
#define TICKS_PER_FRAME (1)                         //if we have 30 keyframes per second, then one tick is 1
#define TICKS_CONVERT (160)

//#define TICKS_PER_SECOND (120)                    //the maximum keyframe value is 4800Hz per second
//#define SECONDS_PER_TICK (1.0f/120.0f)    //the maximum keyframe value is 4800Hz per second
//#define TICKS_PER_FRAME (4)                           //if we have 30 keyframes per second, then one tick is 4
//#define TICKS_CONVERT (40)

//#define TICKS_PER_SECOND (4800)                       //the maximum keyframe value is 4800Hz per second
//#define SECONDS_PER_TICK (1.0f/4800.0f)       //the time for 1 tick
//#define TICKS_PER_FRAME (160)                         //if we have 30 keyframes per second, then one tick is 160
//#define TICKS_CONVERT (1)

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_TICKS_H
