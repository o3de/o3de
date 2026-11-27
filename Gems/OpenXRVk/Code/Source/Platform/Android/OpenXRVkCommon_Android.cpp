/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVkCommon.h>
#include <AzCore/Android/AndroidEnv.h>
#include <OpenXRVk/OpenXRVkUtils.h>


namespace OpenXRVk::Platform
{
    bool OpenXRInitializeLoader()
    {
        PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
        XrResult result = xrGetInstanceProcAddr(
            XR_NULL_HANDLE, "xrInitializeLoaderKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&initializeLoader));
        if (IsError(result))
        {
            return false;
        }

        AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();
        AZ_Assert(androidEnv != nullptr, "Invalid android environment");

        JavaVM* javaVM = nullptr;
        androidEnv->GetJniEnv()->GetJavaVM(&javaVM);
        AZ_Assert(javaVM != nullptr, "Invalid Java VM");
        jobject javaActivity = androidEnv->GetActivityRef();

        XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid;
        memset(&loaderInitInfoAndroid, 0, sizeof(loaderInitInfoAndroid));
        loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInitInfoAndroid.next = nullptr;
        loaderInitInfoAndroid.applicationVM = javaVM;
        loaderInitInfoAndroid.applicationContext = javaActivity;

        result = initializeLoader(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loaderInitInfoAndroid));
        if (IsError(result))
        {
            return false;
        }

        return true;
    }

    void OpenXRBeginFrameInternal()
    {
    }

    void OpenXREndFrameInternal()
    {
        // OpenXR's xrEndFrame function internally uses the application's Java VM (passed in OpenXRInitializeLoader).
        // xrEndFrame function is called from the thread related to the presentation queue (not the main thread) and
        // that causes to create a temporary JNI Environment for this thread, which is not optimal.
        // Calling GetJniEnv() will attach the JNI Environment to this thread.
        AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();
        AZ_Assert(androidEnv != nullptr, "Invalid android environment");
        androidEnv->GetJniEnv();
    }

    void OpenXRPostFrameInternal()
    {
        // Now that EndFrame has finished, calling GetJniEnv() again from the main thread
        // to attach the JNI Environment back to the main thread.
        AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();
        AZ_Assert(androidEnv != nullptr, "Invalid android environment");
        androidEnv->GetJniEnv();
    }
}
