/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

package ${ANDROID_PACKAGE};

import android.util.Log;
import com.amazon.lumberyard.LumberyardActivity;

////////////////////////////////////////////////////////////////////////////////////////////////////
public class ${ANDROID_PROJECT_ACTIVITY} extends LumberyardActivity
{
    // since we are using the NativeActivity, all we need to manually load is the
    // shared c++ libray we are using
    static
    {
        Log.d("LMBR", "BootStrap: Starting Library load");
        System.loadLibrary("c++_shared");
        Log.d("LMBR", "BootStrap: Finished Library load");
    }
}
