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

namespace Alert
{
    enum Buttons
    {
        Ok = 0x1,
        Yes = 0x2,
        No = 0x4,
        YesNo = Yes | No
    };

    Buttons ShowMessage(const char* message, int buttons);
    Buttons ShowMessage(const char* title, const char* message, int buttons);
}
