/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>
#include <AzCore/Android/JNI/JNI.h>


namespace AZ { namespace Android { namespace JNI { namespace Internal
{
    //! Converts a jstring to a string type
    //! \param stringValue A local or global reference to a jstring object
    //! \return A copy of the converted string
    template<typename StringType>
    StringType ConvertJstringToStringImpl(jstring stringValue)
    {
        JNIEnv* jniEnv = GetEnv();
        if (!jniEnv)
        {
            AZ_Error("AZ::Android::JNI", false, "Failed to get JNIEnv* on thread for jstring conversion");
            return StringType();
        }

        const char* convertedStringValue = jniEnv->GetStringUTFChars(stringValue, nullptr);
        if (!convertedStringValue || jniEnv->ExceptionCheck())
        {
            AZ_Error("AZ::Android::JNI", false, "Failed to convert a jstring to cstring");
            HANDLE_JNI_EXCEPTION(jniEnv);
            return StringType();
        }

        StringType localCopy(convertedStringValue);
        jniEnv->ReleaseStringUTFChars(stringValue, convertedStringValue);

        return localCopy;
    }

    //! Converts a string to a jstring
    //! \param stringValue The native string value to be converted
    //! \return A global reference to the converted jstring.  The caller is responsible for
    //!         deleting it when no longer needed
    template<typename StringType>
    AZ_INLINE jstring ConvertStringToJstringImpl(const StringType& stringValue)
    {
        JNIEnv* jniEnv = GetEnv();
        if (!jniEnv)
        {
            AZ_Error("AZ::Android::JNI", false, "Failed to get JNIEnv* on thread for jstring conversion");
            return nullptr;
        }

        jstring localRef = jniEnv->NewStringUTF(stringValue.c_str());
        if (!localRef || jniEnv->ExceptionCheck())
        {
            AZ_Error("AZ::Android::JNI", false, "Failed to convert the cstring to jstring");
            HANDLE_JNI_EXCEPTION(jniEnv);
            return nullptr;
        }

        jstring globalRef = static_cast<jstring>(jniEnv->NewGlobalRef(localRef));
        if (!globalRef || jniEnv->ExceptionCheck())
        {
            AZ_Error("AZ::Android::JNI", false, "Failed to create a global reference to the return jstring");
            HANDLE_JNI_EXCEPTION(jniEnv);
            jniEnv->DeleteLocalRef(localRef);
            return nullptr;
        }

        jniEnv->DeleteLocalRef(localRef);

        return globalRef;
    }
} // namespace Internal
} // namespace JNI
} // namespace Android
} // namespace AZ
