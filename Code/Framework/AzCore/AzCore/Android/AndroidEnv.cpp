/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/JNI/Object.h>

#include <android/configuration.h>


namespace AZ
{
    namespace Android
    {
        static const char* s_loadClassMethodName = "loadClass";

        pthread_key_t AndroidEnv::s_jniEnvKey;
        AZ::EnvironmentVariable<AndroidEnv*> AndroidEnv::s_instance;


        // ----
        // AndroidEnv (public)
        // ----

        ////////////////////////////////////////////////////////////////
        // static
        AndroidEnv* AndroidEnv::Get()
        {
            if (!s_instance)
            {
                s_instance = AZ::Environment::FindVariable<AndroidEnv*>(AZ::AzTypeInfo<AndroidEnv>::Name());
                AZ_Assert(s_instance, "The Android environment is NOT ready for use! Call Create first!");
            }
            return *s_instance;
        }

        ////////////////////////////////////////////////////////////////
        // static
        bool AndroidEnv::Create(const Descriptor& descriptor)
        {
            if (!s_instance)
            {
                s_instance = AZ::Environment::CreateVariable<AndroidEnv*>(AZ::AzTypeInfo<AndroidEnv>::Name());
                (*s_instance) = aznew AndroidEnv();
            }

            if ((*s_instance)->IsReady()) // already created in a different module
            {
                return true;
            }

            return (*s_instance)->Initialize(descriptor);
        }

        ////////////////////////////////////////////////////////////////
        // static
        void AndroidEnv::Destroy()
        {
            if (s_instance)
            {
                if (s_instance.IsOwner())
                {
                    (*s_instance)->Cleanup();
                    delete (*s_instance);
                }
                s_instance.Reset();
            }
            else
            {
                AZ_Assert(false, "The Android environment is NOT ready for use! Call Create first!");
            }
        }


        // ----

        ////////////////////////////////////////////////////////////////
        JNIEnv* AndroidEnv::GetJniEnv() const
        {
            JNIEnv* jniEnv = static_cast<JNIEnv*>(pthread_getspecific(s_jniEnvKey));
            if (!jniEnv)
            {
                jint status = m_jvm->GetEnv((void **) &jniEnv, JNI_VERSION_1_6);
                if (status == JNI_EDETACHED)
                {
                    AZ_TracePrintf("AndroidEnv", "JNI Env not attached to the VM");
                    if (m_jvm->AttachCurrentThread(&jniEnv, NULL) != JNI_OK)
                    {
                        AZ_Assert(false, "Failed to attach tread to the JVM");
                        return nullptr;
                    }
                }
                pthread_setspecific(s_jniEnvKey, jniEnv);
            }
            return jniEnv;
        }

        ////////////////////////////////////////////////////////////////
        const char* AndroidEnv::GetObbFileName(bool mainFile) const
        {
            return (mainFile ? m_mainObbFileName.c_str() : m_patchObbFileName.c_str());
        }

        ////////////////////////////////////////////////////////////////
        void AndroidEnv::UpdateConfiguration()
        {
            if (m_ownsConfiguration)
            {
                AConfiguration_fromAssetManager(m_configuration, m_assetManager);
            }
        }

        ////////////////////////////////////////////////////////////////
        jclass AndroidEnv::LoadClass(const char *classPath)
        {
            JNIEnv* jniEnv = GetJniEnv();
            if (!jniEnv)
            {
                return nullptr;
            }

            jstring classString = jniEnv->NewStringUTF(classPath);
            if (!classString || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to convert cstring %s to jstring", classPath);
                jniEnv->ExceptionDescribe();
                return nullptr;
            }

            jclass returnClass = m_classLoader->InvokeObjectMethod<jclass>(s_loadClassMethodName, classString);
            jniEnv->DeleteLocalRef(classString);

            return returnClass;
        }


        // ----
        // AndroidEnv (private)
        // ----

        ////////////////////////////////////////////////////////////////
        // static
        void AndroidEnv::DestroyJniEnv(void *threadData)
        {
            JNIEnv* jniEnv = static_cast<JNIEnv*>(threadData);
            if (jniEnv)
            {
                JavaVM *javaVm = nullptr;
                jniEnv->GetJavaVM(&javaVm);
                javaVm->DetachCurrentThread();
                pthread_setspecific(s_jniEnvKey, nullptr);
            }
        }


        // ----

        ////////////////////////////////////////////////////////////////
        AndroidEnv::AndroidEnv()
            : m_jvm(nullptr)

            , m_activityRef(nullptr)
            , m_activityClass(nullptr)

            , m_classLoader()

            , m_getClassNameMethod(nullptr)
            , m_getSimpleClassNameMethod(nullptr)

            , m_assetManager(nullptr)
            , m_configuration(nullptr)
            , m_window(nullptr)

            , m_appPrivateStoragePath()
            , m_appPublicStoragePath()
            , m_obbStoragePath()

            , m_mainObbFileName()
            , m_patchObbFileName()

            , m_packageName()
            , m_appVersionCode(0)

            , m_ownsActivityRef(false)
            , m_ownsConfiguration(false)
            , m_isReady(false)

            , m_isRunning(false)
        {
        }

        ////////////////////////////////////////////////////////////////
        AndroidEnv::~AndroidEnv()
        {
            if (s_instance)
            {
                AZ_Assert(s_instance.IsOwner(), "The Android Environment instance is being destroyed by someone other than the owner.");
            }
        }

        ////////////////////////////////////////////////////////////////
        bool AndroidEnv::Initialize(const Descriptor& descriptor)
        {
            m_jvm = descriptor.m_jvm;
            m_assetManager = descriptor.m_assetManager;
            m_configuration = descriptor.m_configuration;
            m_appPrivateStoragePath = descriptor.m_appPrivateStoragePath;
            m_appPublicStoragePath = descriptor.m_appPublicStoragePath;
            m_obbStoragePath = descriptor.m_obbStoragePath;

            if (!m_configuration)
            {
                m_configuration = AConfiguration_new();
                AConfiguration_fromAssetManager(m_configuration, m_assetManager);
                m_ownsConfiguration = true;
            }

            int result = pthread_key_create(&s_jniEnvKey, DestroyJniEnv);
            if (result)
            {
                AZ_Assert(false, "Something went wrong calling pthread_key_create... Error code: %d", result);
                return false;
            }

            JNIEnv* jniEnv = GetJniEnv();
            if (!jniEnv)
            {
                AZ_Error("AndroidEnv", false, "Failed to get JNIEnv* on thread to initialize the AndroidEnv instance");
                return false;
            }

            if (!LoadClassNameMethods(jniEnv))
            {
                return false;
            }

            jobjectRefType refType = jniEnv->GetObjectRefType(descriptor.m_activityRef);
            if (refType == JNIGlobalRefType)
            {
                m_activityRef = descriptor.m_activityRef;
            }
            else if (refType == JNILocalRefType)
            {
                m_activityRef = static_cast<jclass>(jniEnv->NewGlobalRef(descriptor.m_activityRef));
                if (!m_activityRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("AndroidEnv", false, "Failed to construct a global reference to the activity instance");
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }
                m_ownsActivityRef = true;
            }
            else
            {
                AZ_Error("AndroidEnv", false, "Unable to use 'activityRef' argument for global ref construction");
                return false;
            }

            jclass activityClass = jniEnv->GetObjectClass(m_activityRef);

            m_activityClass = static_cast<jclass>(jniEnv->NewGlobalRef(activityClass));
            if (!m_activityClass || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to construct a global reference to the activity class");
                HANDLE_JNI_EXCEPTION(jniEnv);
                jniEnv->DeleteLocalRef(activityClass);
                return false;
            }

            jniEnv->DeleteLocalRef(activityClass);

            if (!CacheActivityData(jniEnv))
            {
                return false;
            }

            if (m_obbStoragePath.empty())
            {
                AZ::OSString relPath = AZ::OSString::format("/data/%s/files", m_packageName.c_str());
                AZ_Assert(m_appPublicStoragePath.find(relPath) != AZ::OSString::npos,
                          "Public application storage path appears to be invalid.  The OBB path may be incorrect and lead to unexpected results.");

                AZ::OSString publicAndroidRoot = m_appPublicStoragePath.substr(0, m_appPublicStoragePath.length() - relPath.length());
                m_obbStoragePath = AZ::OSString::format("%s/obb/%s", publicAndroidRoot.c_str(), m_packageName.c_str());
            }

            m_mainObbFileName = AZ::OSString::format("main.%d.%s.obb", m_appVersionCode, m_packageName.c_str());
            m_patchObbFileName = AZ::OSString::format("patch.%d.%s.obb", m_appVersionCode, m_packageName.c_str());

            AZ_TracePrintf("AndroidEnv", "Application private storage path   = %s", m_appPrivateStoragePath.c_str());
            AZ_TracePrintf("AndroidEnv", "Application public storage path    = %s", m_appPublicStoragePath.c_str());
            AZ_TracePrintf("AndroidEnv", "Application OBB path               = %s", m_obbStoragePath.c_str());

            AZ_TracePrintf("AndroidEnv", "Main OBB file name                 = %s", m_mainObbFileName.c_str());
            AZ_TracePrintf("AndroidEnv", "Patch OBB file name                = %s", m_patchObbFileName.c_str());

            if (!APKFileHandler::Create())
            {
                AZ_Error("AndroidEnv", false, "Failed to construct the global APK file handler");
                return false;
            }

            m_isReady = true;
            return true;
        }

        ////////////////////////////////////////////////////////////////
        void AndroidEnv::Cleanup()
        {
            if (m_ownsActivityRef)
            {
                JNI::DeleteRef(m_activityRef);
            }
            JNI::DeleteRef(m_activityClass);

            if (m_ownsConfiguration)
            {
                AConfiguration_delete(m_configuration);
            }

            APKFileHandler::Destroy();
        }

        ////////////////////////////////////////////////////////////////
        bool AndroidEnv::LoadClassNameMethods(JNIEnv* jniEnv)
        {
            const char* javaClassPath = "java/lang/Class";
            const char* getNameMethodName = "getName";
            const char* getSimpleNameMethodName = "getSimpleName";
            const char* getNameMethodSignature = "()Ljava/lang/String;";

            // since we are requesting a system class, it should be safe to use FindClass instead
            // of the ClassLoader.
            jclass javaClass = jniEnv->FindClass(javaClassPath);
            if (!javaClass || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to find class %s from the JNI environment", javaClassPath);
                HANDLE_JNI_EXCEPTION(jniEnv);
                return false;
            }

            m_getClassNameMethod = jniEnv->GetMethodID(javaClass, getNameMethodName, getNameMethodSignature);
            if (!m_getClassNameMethod || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to find method %s with signature %s in class %s", getNameMethodName, getNameMethodSignature, javaClassPath);
                HANDLE_JNI_EXCEPTION(jniEnv);
                jniEnv->DeleteLocalRef(javaClass);
                return false;
            }

            m_getSimpleClassNameMethod = jniEnv->GetMethodID(javaClass, getSimpleNameMethodName, getNameMethodSignature);
            if (!m_getSimpleClassNameMethod || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to find method %s with signature %s in class %s", getSimpleNameMethodName, getNameMethodSignature, javaClassPath);
                HANDLE_JNI_EXCEPTION(jniEnv);
                jniEnv->DeleteLocalRef(javaClass);
                return false;
            }

            jniEnv->DeleteLocalRef(javaClass);
            return true;
        }

        ////////////////////////////////////////////////////////////////
        bool AndroidEnv::CacheActivityData(JNIEnv* jniEnv)
        {
            JniObject activityObject(m_activityClass, m_activityRef);

            activityObject.RegisterMethod("GetPackageName", "()Ljava/lang/String;");
            activityObject.RegisterMethod("GetAppVersionCode", "()I");
            activityObject.RegisterMethod("getClassLoader", "()Ljava/lang/ClassLoader;");

            m_packageName = activityObject.InvokeStringMethod("GetPackageName");
            m_appVersionCode = activityObject.InvokeIntMethod("GetAppVersionCode");

            // construct the global class loader object
            jobject classLoaderRef = activityObject.InvokeObjectMethod<jobject>("getClassLoader");
            if (!classLoaderRef)
            {
                AZ_Error("AndroidEnv", false, "Failed to retrieve the class loader from the activity");
                HANDLE_JNI_EXCEPTION(jniEnv);
                return false;
            }

            jclass localClassLoaderClass = jniEnv->GetObjectClass(classLoaderRef);
            if (!localClassLoaderClass || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to get jclass from ClassLoader");
                HANDLE_JNI_EXCEPTION(jniEnv);
                return false;
            }

            jclass classLoaderClass = static_cast<jclass>(jniEnv->NewGlobalRef(localClassLoaderClass));
            if (!classLoaderClass || jniEnv->ExceptionCheck())
            {
                AZ_Error("AndroidEnv", false, "Failed to create a global reference to the class loader");
                HANDLE_JNI_EXCEPTION(jniEnv);
                jniEnv->DeleteLocalRef(localClassLoaderClass);
                return false;
            }

            jniEnv->DeleteLocalRef(localClassLoaderClass);

            m_classLoader.reset(aznew JniObject(classLoaderClass, classLoaderRef, true));
            m_classLoader->RegisterMethod(s_loadClassMethodName, "(Ljava/lang/String;)Ljava/lang/Class;");

            return true;
        }
    }
}
