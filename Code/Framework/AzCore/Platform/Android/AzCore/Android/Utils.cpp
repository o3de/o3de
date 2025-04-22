/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Android/Utils.h>
#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/JNI/Object.h>

#include <AzCore/Debug/Trace.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ
{
    namespace Android
    {
        namespace Utils
        {
            namespace
            {
                ////////////////////////////////////////////////////////////////
                constexpr const char* GetApkAssetsPrefix()
                {
                    return "/APK";
                }
            }


            ////////////////////////////////////////////////////////////////
            jclass GetActivityClassRef()
            {
                return AndroidEnv::Get()->GetActivityClassRef();
            }

            ////////////////////////////////////////////////////////////////
            jobject GetActivityRef()
            {
                return AndroidEnv::Get()->GetActivityRef();
            }

            ////////////////////////////////////////////////////////////////
            AAssetManager* GetAssetManager()
            {
                return AndroidEnv::Get()->GetAssetManager();
            }

            ////////////////////////////////////////////////////////////////
            AConfiguration* GetConfiguration()
            {
                return AndroidEnv::Get()->GetConfiguration();
            }

            ////////////////////////////////////////////////////////////////
            void UpdateConfiguration()
            {
                return AndroidEnv::Get()->UpdateConfiguration();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetAppPrivateStoragePath()
            {
                return AndroidEnv::Get()->GetAppPrivateStoragePath();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetAppPublicStoragePath()
            {
                return AndroidEnv::Get()->GetAppPublicStoragePath();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetObbStoragePath()
            {
                return AndroidEnv::Get()->GetObbStoragePath();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetPackageName()
            {
                return AndroidEnv::Get()->GetPackageName();
            }

            ////////////////////////////////////////////////////////////////
            int GetAppVersionCode()
            {
                return AndroidEnv::Get()->GetAppVersionCode();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetObbFileName(bool mainFile)
            {
                return AndroidEnv::Get()->GetObbFileName(mainFile);
            }

            ////////////////////////////////////////////////////////////////
            bool IsApkPath(const char* filePath)
            {
                return AZ::IO::PathView(filePath).IsRelativeTo(AZ::IO::PathView(GetApkAssetsPrefix()));
            }

            ////////////////////////////////////////////////////////////////
            AZ::IO::FixedMaxPath StripApkPrefix(const char* filePath)
            {
                constexpr AZ::IO::PathView apkPrefixView = GetApkAssetsPrefix();
                return AZ::IO::PathView(filePath).LexicallyProximate(apkPrefixView);
            }

            ////////////////////////////////////////////////////////////////
            const char* FindAssetsDirectory()
            {
            #if defined(LY_NO_ASSETS)
                // The TestRunner app which runs unit tests does not have any assets.
                return GetAppPublicStoragePath();
            #endif
            #if !defined(_RELEASE)
                // first check to see if they are in public storage (application specific)
                const char* publicAppStorage = GetAppPublicStoragePath();

                OSString path = OSString::format("%s/engine.json", publicAppStorage);
                AZ_TracePrintf("Android::Utils", "Searching for %s\n", path.c_str());

                FILE* f = fopen(path.c_str(), "r");
                if (f != nullptr)
                {
                    fclose(f);
                    return publicAppStorage;
                }
            #endif // !defined(_RELEASE)

                // if they aren't in public storage, they are in private storage (APK)
                AAssetManager* mgr = GetAssetManager();
                if (mgr)
                {
                    // The assets folder in an APK will have either 'engine.json' (LOOSE mode) or 'engine_android.pak' (PAK Mode)
                    const char* asset_marker_files[] = { "engine.json", "engine_android.pak" };
                    AAsset* asset = nullptr;
                    for (const char* asset_marker : asset_marker_files)
                    {
                        asset = AAssetManager_open(mgr, asset_marker, AASSET_MODE_UNKNOWN);
                        if (asset)
                        {
                            AAsset_close(asset);
                            return GetApkAssetsPrefix();
                        }
                    }
                }

                AZ_Assert(false, "Failed to locate the engine.json path");
                return nullptr;
            }

            ////////////////////////////////////////////////////////////////
            void ShowSplashScreen()
            {
                JNI::Internal::Object<AZ::OSAllocator> activity(GetActivityClassRef(), GetActivityRef());

                activity.RegisterMethod("ShowSplashScreen", "()V");
                activity.InvokeVoidMethod("ShowSplashScreen");
            }

            ////////////////////////////////////////////////////////////////
            void DismissSplashScreen()
            {
                JNI::Internal::Object<AZ::OSAllocator> activity(GetActivityClassRef(), GetActivityRef());

                activity.RegisterMethod("DismissSplashScreen", "()V");
                activity.InvokeVoidMethod("DismissSplashScreen");
            }

            ////////////////////////////////////////////////////////////////
            ANativeWindow* GetWindow()
            {
                return AndroidEnv::Get()->GetWindow();
            }

            ////////////////////////////////////////////////////////////////
            bool GetWindowSize(int& widthPixels, int& heightPixels)
            {
                ANativeWindow* window = GetWindow();
                if (window)
                {
                    widthPixels = ANativeWindow_getWidth(window);
                    heightPixels = ANativeWindow_getHeight(window);

                    // should an error occur from the above functions a negative value will be returned
                    return (widthPixels > 0 && heightPixels > 0);
                }
                return false;
            }

            ////////////////////////////////////////////////////////////////
            void SetLoadFilesToMemory(const char* fileNames)
            {
                APKFileHandler::SetLoadFilesToMemory(fileNames);
            }
        }
    }
}
