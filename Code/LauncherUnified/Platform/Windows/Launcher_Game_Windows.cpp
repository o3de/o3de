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
