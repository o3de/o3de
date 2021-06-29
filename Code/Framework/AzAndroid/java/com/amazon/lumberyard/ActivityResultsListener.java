/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

package com.amazon.lumberyard;

import android.app.Activity;
import android.content.Intent;


////////////////////////////////////////////////////////////////
public abstract class ActivityResultsListener
{
    public ActivityResultsListener(Activity activity)
    {
        ((LumberyardActivity)activity).RegisterActivityResultsListener(this);
    }

    public abstract boolean ProcessActivityResult(int requestCode, int resultCode, Intent data);
}
