/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

typedef unsigned long DWORD;

//Due to some laptops not auto-switching to the discrete GPU correctly we are adding these
//__declspec as defined in the AMD and NVidia white papers to 'force on' the use of the
//discrete chips.  This will be overridden by users setting application profiles
//and may not work on older drivers or bios. In theory this should be enough to always force on
//the discrete chips.

//http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
//https://community.amd.com/thread/169965

// It is unclear if this is also needed for Linux or macOS at this time (22/02/2017)
extern "C"
{
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
