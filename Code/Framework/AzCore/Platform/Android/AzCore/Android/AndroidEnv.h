/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/osstring.h>

#include <jni.h>
#include <pthread.h>


struct AAssetManager;
struct ANativeWindow;
struct AConfiguration;


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                template<typename Allocator>
                class Object;

                template<typename StringType>
                class ClassName;
            } // namespace Internal
        } // namespace JNI


        class AndroidEnv
        {
        public:
            AZ_TYPE_INFO(AndroidEnv, "{E51A8876-7A26-4CB1-BA88-394A128728C7}")
            AZ_CLASS_ALLOCATOR(AndroidEnv, AZ::OSAllocator);


            //! Creation POD for the AndroidEnv
            struct Descriptor
            {
                Descriptor()
                    : m_jvm(nullptr)
                    , m_activityRef(nullptr)
                    , m_assetManager(nullptr)
                    , m_configuration(nullptr)
                    , m_appPrivateStoragePath()
                    , m_appPublicStoragePath()
                    , m_obbStoragePath()
                {
                }

                JavaVM* m_jvm; //!< Global pointer to the Java virtual machine
                jobject m_activityRef; //!< Local or global reference to the activity instance
                AAssetManager* m_assetManager; //!< Global pointer to the Android asset manager, used for APK file i/o
                AConfiguration* m_configuration; //!< Global pointer to the configuration of the device, e.g. orientation, screen density, locale, etc.
                AZ::OSString m_appPrivateStoragePath; //!< Access restricted location. E.G. /data/data/<package_name>/files
                AZ::OSString m_appPublicStoragePath; //!< Public storage specifically for the application. E.G. <public_storage>/Android/data/<package_name>/files
                AZ::OSString m_obbStoragePath; //!< Public storage specifically for the application's obb files. E.G. <public_storage>/Android/obb/<package_name>/files
            };

            //! Public accessor to the global AndroidEnv instance
            static AndroidEnv* Get();

            //! The preferred entry point for the construction of the global AndroidEnv instance
            static bool Create(const Descriptor& descriptor);

            //! Public accessor to destroy the AndroidEnv global instance
            static void Destroy();


            // ----

            //! Request a thread specific JNIEnv pointer from the JVM.
            //! \return A pointer to the JNIEnv on the current thread.
            JNIEnv* GetJniEnv() const;

            //! Request the global reference to the activity class
            jclass GetActivityClassRef() const { return m_activityClass; }

            //! Request the global reference to the activity instance
            jobject GetActivityRef() const { return m_activityRef; }

            //! Get the global pointer to the Android asset manager, which is used for APK file i/o.
            AAssetManager* GetAssetManager() const { return m_assetManager; }

            //! Get the global pointer to the device/application configuration,
            AConfiguration* GetConfiguration() const { return m_configuration; }

            //! Set the global pointer to the Android window surface.
            void SetWindow(ANativeWindow* window) { m_window = window; }

            //! Get the global pointer to the Android window surface
            ANativeWindow* GetWindow() const { return m_window; }

            //! Get the hidden internal storage, typically this is where the application is installed on the device.
            //! e.g. /data/data/<package_name/files
            const char* GetAppPrivateStoragePath() const { return m_appPrivateStoragePath.c_str(); }

            //! Get the application specific directory for public storage.
            //! e.g. <public_storage>/Android/data/<package_name/files
            const char* GetAppPublicStoragePath() const { return m_appPublicStoragePath.c_str(); }

            //! Get the application specific directory for obb files.
            //! e.g. <public_storage>/Android/obb/<package_name/files
            const char* GetObbStoragePath() const { return m_obbStoragePath.c_str(); }

            //! Get the dot separated package name for the current application.
            //! e.g. org.o3de.samples for SamplesProject
            const char* GetPackageName() const { return m_packageName.c_str(); }

            //! Get the app version code (android:versionCode in the manifest).
            int GetAppVersionCode() const { return m_appVersionCode; }

            //! Get the filename of the obb. This doesn't include the path to the obb folder.
            const char* GetObbFileName(bool mainFile) const;

            //! Check if the AndroidEnv has been initialized
            bool IsReady() const { return m_isReady; }

            //! Set wheather or not the application should be running
            void SetIsRunning(bool isRunning) { m_isRunning = isRunning; }

            //! Check if the application has been backgrounded (false) or not (true)
            bool IsRunning() const { return m_isRunning; }

            //! If the AndroidEnv owns the native configuration, it will be updated with the latest configuration
            //! information, otherwise nothing will happen.
            void UpdateConfiguration();

            //! Loads a Java class as opposed to attempting to find a loaded class from the call stack.
            //! \param classPath The fully qualified forward slash separated Java class path.
            //! \return A global reference to the desired jclass.  Caller is responsible for making a
            //!         call to DeleteGlobalJniRef when the jclass is no longer needed.
            jclass LoadClass(const char* classPath);
        private:
            template<typename StringType>
            friend class JNI::Internal::ClassName;

            typedef JNI::Internal::Object<OSAllocator> JniObject; //!< Internal usage of \ref AZ::Android::JNI::Internal::Object that uses the OSAllocator


            //! Callback for when a thread exists to detach the jni env from the thread
            //! \param threadData Expected to be the JNIEnv pointer
            static void DestroyJniEnv(void* threadData);


            // ----

            AndroidEnv();
            ~AndroidEnv();

            AZ_DISABLE_COPY_MOVE(AndroidEnv);

            //! Public global accessor to the android application environment
            //! \param descriptor
            bool Initialize(const Descriptor& descriptor);

            //! Handle the deletion of the global jni references
            void Cleanup();

            //! Finds the java/lang/Class jclass to get the method IDs to getName and getSimpleName
            //! \return True if successfully, False otherwise
            bool LoadClassNameMethods(JNIEnv* jniEnv);

            //! Calls some java methods on the activity instance and constructs the class loader
            //! \return True if successfully, False otherwise
            bool CacheActivityData(JNIEnv* jniEnv);


            // ----

            static pthread_key_t s_jniEnvKey; //!< Thread key for accessing the thread specific jni env pointers
            static AZ::EnvironmentVariable<AndroidEnv*> s_instance; //!< Reference to the global object, created in the main function (AndroidLauncher)


            JavaVM* m_jvm; //!< Mostly used for [de/a]ttaching JNIEnv pointers to threads

            jobject m_activityRef; //!< Reference to the global instance of the current activity object, used for instance method invocation, field access
            jclass m_activityClass; //!< Reference to the global instance of the current activity class, used for method / field extraction, static method invocation

            AZStd::unique_ptr<JniObject> m_classLoader; //!< Class loader instance, used for finding Java classes on any thread

            jmethodID m_getClassNameMethod; //!< Method ID for getName from java/lang/Class which returns a fully qualified dot separated Java class path
            jmethodID m_getSimpleClassNameMethod; //!< Method ID for getSimpleName from java/lang/Class which returns just the class name from a Java class path

            AAssetManager* m_assetManager; //!< Global pointer to the Android asset manager, used for APK file i/o
            AConfiguration* m_configuration; //!< Global pointer to the configuration of the device, e.g. orientation, screen density, locale, etc.
            ANativeWindow* m_window; //!< Global pointer to the window surface created by Android, used for creating GL contexts

            AZ::OSString m_appPrivateStoragePath; //!< Access restricted location. E.G. /data/data/<package_name>/files
            AZ::OSString m_appPublicStoragePath; //!< Public storage specifically for the application. E.G. <public_storage>/Android/data/<package_name>/files
            AZ::OSString m_obbStoragePath; //!< Public storage specifically for the application's obb files. E.G. <public_storage>/Android/obb/<package_name>/files

            AZ::OSString m_mainObbFileName; //!< File name for the main OBB
            AZ::OSString m_patchObbFileName; //!< File name for the patch OBB

            AZ::OSString m_packageName; //!< The dot separated package id of the application
            int m_appVersionCode; //!< The version code of the app (android:versionCode in the AndroidManifest.xml)

            bool m_ownsActivityRef; //!< For when a local activity ref is passed into the construction and needs to be cleaned up
            bool m_ownsConfiguration; //!< For when no configuration is passed into the construction and needs to be cleaned up
            bool m_isReady; //!< Set only once the object has been successfully constructed

            bool m_isRunning; //!< Internal flag indicating if the application is running, mainly used to determine if we shoudl be blocking on the event pump while paused
        };
    } // namespace Android
} // namespace AZ
