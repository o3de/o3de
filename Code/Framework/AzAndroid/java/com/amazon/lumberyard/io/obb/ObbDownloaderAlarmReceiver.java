/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
package com.amazon.lumberyard.io.obb;


import com.google.android.vending.expansion.downloader.DownloaderClientMarshaller;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;


////////////////////////////////////////////////////////////////
// Alarm receiver needeed by the Downloader library for reasuming the download of the Obb.
public class ObbDownloaderAlarmReceiver extends BroadcastReceiver
{
    ////////////////////////////////////////////////////////////////
    @Override
    public void onReceive(Context context, Intent intent)
    {
        try
        {
            DownloaderClientMarshaller.startDownloadServiceIfRequired(context, intent, ObbDownloaderService.class);
        }
        catch (NameNotFoundException e)
        {
            e.printStackTrace();
        }
    }
}
