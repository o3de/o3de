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

#include <IConsole.h>
#include <ISystem.h>

namespace AZ
{
    namespace VR
    {
        class HMDCVars
        {
        public:

            static int hmd_social_screen;
            static int hmd_debug_info;
            static int hmd_debug_camera;

            static void Register()
            {
                REGISTER_CVAR2("hmd_social_screen", &hmd_social_screen, hmd_social_screen,
                    VF_NULL, "Selects the social screen mode: \n"
                    "-1- Off\n"
                    "0 - Undistorted left eye\n"
                    "1 - Undistorted right eye\n"
                    );

                REGISTER_INT_CB("hmd_debug_info", 0, VF_ALWAYSONCHANGE,
                    "Enable/disable HMD and VR controller debug info/rendering",
                    OnHMDDebugInfo);

                REGISTER_INT_CB("hmd_debug_camera", 0, VF_ALWAYSONCHANGE,
                    "Enable/disable HMD debug camera",
                    OnHMDDebugCamera);

                REGISTER_COMMAND("hmd_tracking_level", &OnHMDTrackingLevelChange,
                    VF_NULL, "Set the HMD center reference point.\n"
                    "0 - Camera (Actor's head)\n"
                    "1 - Actor's feet (floor)\n");

                REGISTER_COMMAND("hmd_recenter_pose", &OnHMDRecenter,
                    VF_NULL, "Re-centers sensor orientation of the HMD.");

                REGISTER_INT_CB("output_to_hmd", 0, VF_ALWAYSONCHANGE,
                    "Enable/disable output to any connected HMD (for VR)",
                    OnOutputToHMDChanged);
            }

        private:

            static void OnHMDRecenter(IConsoleCmdArgs* args);
            static void OnHMDTrackingLevelChange(IConsoleCmdArgs* args);
            static void OnOutputToHMDChanged(ICVar* var);
            static void OnHMDDebugInfo(ICVar* var);
            static void OnHMDDebugCamera(ICVar* var);
        };
    } // namespace VR
} // namespace AZ
