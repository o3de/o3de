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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_MULTIMONHELPER_H
#define CRYINCLUDE_EDITOR_CONTROLS_MULTIMONHELPER_H
#pragma once

// Taken from: http://msdn.microsoft.com/en-us/library/dd162826(v=vs.85).aspx
#define MONITOR_CENTER   0x0001        // center rect to monitor
#define MONITOR_CLIP     0x0000        // clip rect to monitor
#define MONITOR_WORKAREA 0x0002        // use monitor work area
#define MONITOR_AREA     0x0000        // use monitor entire area

//
//  ClipOrCenterRectToMonitor
//
//  The most common problem apps have when running on a
//  multimonitor system is that they "clip" or "pin" windows
//  based on the SM_CXSCREEN and SM_CYSCREEN system metrics.
//  Because of app compatibility reasons these system metrics
//  return the size of the primary monitor.
//
//  This shows how you use the multi-monitor functions
//  to do the same thing.
//
// params:
// prc : pointer to QRect to modify
// flags : some combination of the MONITOR_* flags above
//
// example:
//
//  ClipOrCenterRectToMonitor(&aRect, MONITOR_CLIP | MONITOR_WORKAREA);
//
//  Takes parameter pointer to RECT "aRect" and flags MONITOR_CLIP | MONITOR_WORKAREA
//      This will modify aRect without resizing it so that it remains within the on-screen boundaries.
void ClipOrCenterRectToMonitor(QRect *prc, const UINT flags);

#endif // CRYINCLUDE_EDITOR_CONTROLS_MULTIMONHELPER_H
