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
package com.amazon.lumberyard;

import android.app.ActivityManager;
import android.content.Context;

////////////////////////////////////////////////////////////////////////////////////////////////////
public class AndroidDeviceManager
{
    public static Context context;
    public static final float bytesInGB = (1024.0f * 1024.0f * 1024.0f);

    public static float GetDeviceRamInGB()
    {
        ActivityManager actManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
        actManager.getMemoryInfo(memInfo);
        float totalMemory = memInfo.totalMem / bytesInGB;
        return totalMemory;
    }
}
