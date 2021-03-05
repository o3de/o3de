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