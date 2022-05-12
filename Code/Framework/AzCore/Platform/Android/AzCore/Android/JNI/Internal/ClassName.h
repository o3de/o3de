/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/JNI/Internal/JStringUtils.h>


namespace AZ { namespace Android { namespace JNI { namespace Internal
{
    //! \brief Utility for getting the Java class names
    //! \tparam StringType The type of string that should be return during generation.  Defaults to AZStd::string
    template<typename StringType = AZStd::string>
    class ClassName
    {
    public:
        //! Get the fully qualified forward slash separated Java class path of Java class ref.
        //! e.g. android.app.NativeActivity ==> android/app/NativeActivity
        //! \param classRef A valid reference to a java class
        //! \return A copy of the class name
        static StringType GetName(jclass classRef)
        {
            StringType className;

            AndroidEnv* androidEnv = AndroidEnv::Get();
            AZ_Assert(androidEnv, "Attempting to use the AndroidEnv before it's created");

            if (androidEnv)
            {
                className = GetNameImpl(classRef, androidEnv->m_getClassNameMethod);
            }
            return className;
        }

        //! Get just the name of the Java class from a Java class ref.
        //! e.g. android.app.NativeActivity ==> NativeActivity
        //! \param classRef A valid reference to a java class
        //! \return A copy of the class name
        static StringType GetSimpleName(jclass classRef)
        {
            StringType className;

            AndroidEnv* androidEnv = AndroidEnv::Get();
            AZ_Assert(androidEnv, "Attempting to use the AndroidEnv before it's created");

            if (androidEnv)
            {
                className = GetNameImpl(classRef, androidEnv->m_getSimpleClassNameMethod);
            }
            return className;
        }


    private:
        static StringType GetNameImpl(jclass classRef, jmethodID methodId)
        {
            JNIEnv* jniEnv = GetEnv();
            if (!jniEnv)
            {
                AZ_Error("JNI::ClassName", false, "Failed to get JNIEnv* on thread on call to GetClassNameImpl");
                return StringType();
            }

            jstring rawStringValue = static_cast<jstring>(jniEnv->CallObjectMethod(classRef, methodId));
            if (!rawStringValue || jniEnv->ExceptionCheck())
            {
                AZ_Error("JNI::ClassName", false, "Failed to invoke a GetName variant method on class Unknown");
                HANDLE_JNI_EXCEPTION(jniEnv);
                return StringType();
            }

            StringType className = ConvertJstringToStringImpl<StringType>(rawStringValue);
            jniEnv->DeleteLocalRef(rawStringValue);

            return className;
        }
    };
} // namespace Internal
} // namespace JNI
} // namespace Android
} // namespace AZ
