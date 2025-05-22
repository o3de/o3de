/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/string/string.h>

#include <jni.h>
#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/native_window.h>


namespace AZ
{
    namespace Android
    {
        namespace Utils
        {
            //! Request the global reference to the activity class
            AZCORE_API jclass GetActivityClassRef();

            //! Request the global reference to the activity instance
            AZCORE_API jobject GetActivityRef();

            //! Get the global pointer to the Android asset manager, which is used for APK file i/o.
            AZCORE_API AAssetManager* GetAssetManager();

            //! Get the global pointer to the device/application configuration,
            AZCORE_API AConfiguration* GetConfiguration();

            //! If the AndroidEnv owns the native configuration, it will be updated with the latest configuration
            //! information, otherwise nothing will happen.
            AZCORE_API void UpdateConfiguration();

            //! Get the hidden internal storage, typically this is where the application is installed
            //! on the device.
            //! e.g. /data/data/<package_name>/files
            AZCORE_API const char* GetAppPrivateStoragePath();

            //! Get the application specific directory for public public storage.
            //! e.g. <public_storage>/Android/data/<package_name>/files
            AZCORE_API const char* GetAppPublicStoragePath();

            //! Get the application specific directory for obb files.
            //! e.g. <public_storage>/Android/obb/<package_name>/files
            AZCORE_API const char* GetObbStoragePath();

            //! Get the dot separated package name for the current application.
            //! e.g. org.o3de.samples for SamplesProject
            AZCORE_API const char* GetPackageName();

            //! Get the app version code (android:versionCode in the manifest).
            AZCORE_API int GetAppVersionCode();

            //! Get the filename of the obb. This doesn't include the path to the obb folder.
            AZCORE_API const char* GetObbFileName(bool mainFile);

            //! Check to see if the path is prefixed with "/APK"
            AZCORE_API bool IsApkPath(const char* filePath);

            //! Will first check to verify the argument is an apk asset path and if so
            //! will strip the prefix from the path.
            //! \return The pointer position of the relative asset path
            AZCORE_API AZ::IO::FixedMaxPath StripApkPrefix(const char* filePath);

            //! Searches application storage and the APK for engine.json.  Will return nullptr
            //! if engine.json is not found.
            AZCORE_API const char* FindAssetsDirectory();

            //! Calls into Java to show the splash screen on the main UI (Java) thread
            AZCORE_API void ShowSplashScreen();

            //! Calls into Java to dismiss the splash screen on the main UI (Java) thread
            AZCORE_API void DismissSplashScreen();

            //! Get the native android window
            AZCORE_API ANativeWindow* GetWindow();

            //! Query the pixel dimensions of the window
            //! \param[out] widthPixels Returns the pixel width of the window
            //! \param[out] heightPixels Returns the pixel height of the window
            //! \return True if successful, False otherwise
            AZCORE_API bool GetWindowSize(int& widthPixels, int& heightPixels);

            //! Set the filenames for files to be loaded to memory
            AZCORE_API void SetLoadFilesToMemory(const char* fileNames);
        }
    }
}
