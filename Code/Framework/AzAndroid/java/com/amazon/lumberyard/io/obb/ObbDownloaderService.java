/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
package com.amazon.lumberyard.io.obb;


import android.content.Context;
import android.content.res.Resources;
import android.util.Log;

import com.google.android.vending.expansion.downloader.impl.DownloaderService;
import com.google.android.vending.licensing.util.Base64;
import com.google.android.vending.licensing.util.Base64DecoderException;


////////////////////////////////////////////////////////////////
// Service needed by the downloader library in order to get the public key, salt and alarm receiver class.
public class ObbDownloaderService extends DownloaderService
{
    ////////////////////////////////////////////////////////////////
    @Override
    public void onCreate ()
    {
        super.onCreate();
        Context context = getApplicationContext();
        Resources resources = getResources();
        int stringId = resources.getIdentifier("public_key", "string", context.getPackageName());
        m_base64PublicKey = resources.getString(stringId);
        stringId = resources.getIdentifier("obfuscator_salt", "string", context.getPackageName());
        String base64Salt = resources.getString(stringId);
        if (!base64Salt.isEmpty())
        {
            try
            {
                m_salt = Base64.decode(base64Salt);
            }
            catch (Base64DecoderException e)
            {
                Log.e("ObbDownloaderService", "Failed to decode the salt string");
            }
        }
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public String getPublicKey()
    {
        return m_base64PublicKey;
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public byte[] getSALT()
    {
        return m_salt;
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public String getAlarmReceiverClassName()
    {
        return ObbDownloaderAlarmReceiver.class.getName();
    }

    ////////////////////////////////////////////////////////////////
    private String m_base64PublicKey;
    private byte[] m_salt = new byte[] { 23, 12, 4, -12, -34, 23,
            -120, 122, -23, -104, -2, -4, 12, 3, -21, 123, -11, 4, -11, 32
    };
}
