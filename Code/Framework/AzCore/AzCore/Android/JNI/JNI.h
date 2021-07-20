/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include <jni.h>
#include <android/asset_manager.h>


#define HANDLE_JNI_EXCEPTION(jniEnv) \
    jniEnv->ExceptionDescribe(); \
    jniEnv->ExceptionClear();


#if defined(AZ_DEBUG_BUILD)
    #define JNI_SIGNATURE_VALIDATION
#endif


// redefine the JNI_FALSE and JNI_TRUE macros to ensure their correct types are represented when using them
#if defined(JNI_FALSE)
    #undef JNI_FALSE
    #define JNI_FALSE jboolean(0)
#endif // defined(JNI_FALSE)

#if defined(JNI_TRUE)
    #undef JNI_TRUE
    #define JNI_TRUE jboolean(1)
#endif // defined(JNI_TRUE)


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            //! Request a thread specific JNIEnv pointer from the Android environment.
            //! \return A pointer to the JNIEnv on the current thread.
            JNIEnv* GetEnv();

            //! Loads a Java class as opposed to attempting to find a loaded class from the call stack.
            //! \param classPath The fully qualified forward slash separated Java class path.
            //! \return A global reference to the desired jclass.  Caller is responsible for making a
            //!         call do DeleteGlobalJniRef when the jclass is no longer needed.
            jclass LoadClass(const char* classPath);

            //! Get the fully qualified forward slash separated Java class path of Java class ref.
            //! e.g. android.app.NativeActivity ==> android/app/NativeActivity
            //! \param classRef A valid reference to a java class
            //! \return A copy of the class name
            AZStd::string GetClassName(jclass classRef);

            //! Get just the name of the Java class from a Java class ref.
            //! e.g. android.app.NativeActivity ==> NativeActivity
            //! \param classRef A valid reference to a java class
            //! \return A copy of the class name
            AZStd::string GetSimpleClassName(jclass classRef);

            //! Converts a jstring to a AZStd::string
            //! \param stringValue A local or global reference to a jstring object
            //! \return A copy of the converted string
            AZStd::string ConvertJstringToString(jstring stringValue);

            //! Converts a string to a jstring
            //! \param stringValue The native string value to be converted
            //! \return A global reference to the converted jstring.  The caller is responsible for
            //!         deleting it when no longer needed
            jstring ConvertStringToJstring(const AZStd::string& stringValue);

            //! Gets the reference type of the Java object.  Can be Local, Global or Weak Global.
            //! \param javaRef Raw Java object reference, can be null.
            //! \return The result of GetObjectRefType as long as the object is valid,
            //!         otherwise JNIInvalidRefType.
            int GetRefType(jobject javaRef);

            //! Deletes a JNI object/class reference.  Will handle local, global and weak global references.
            //! \param javaRef Raw java object reference.
            void DeleteRef(jobject javaRef);
        }
    }
}
