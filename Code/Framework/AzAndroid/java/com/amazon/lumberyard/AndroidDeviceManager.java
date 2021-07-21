/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
