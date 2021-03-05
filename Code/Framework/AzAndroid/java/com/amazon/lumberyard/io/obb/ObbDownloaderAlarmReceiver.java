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