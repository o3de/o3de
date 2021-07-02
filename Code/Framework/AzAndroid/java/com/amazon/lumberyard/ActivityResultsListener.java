/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
