/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Internal/JStringUtils.h>
#include <AzCore/Android/JNI/Internal/ClassName.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            ////////////////////////////////////////////////////////////////
            JNIEnv* GetEnv()
            {
                return AndroidEnv::Get()->GetJniEnv();
            }

            ////////////////////////////////////////////////////////////////
            jclass LoadClass(const char* classPath)
            {
                return AndroidEnv::Get()->LoadClass(classPath);
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string GetClassName(jclass classRef)
            {
                return Internal::ClassName<AZStd::string>::GetName(classRef);
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string GetSimpleClassName(jclass classRef)
            {
                return Internal::ClassName<AZStd::string>::GetSimpleName(classRef);
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string ConvertJstringToString(jstring stringValue)
            {
                return Internal::ConvertJstringToStringImpl<AZStd::string>(stringValue);
            }

            ////////////////////////////////////////////////////////////////
            jstring ConvertStringToJstring(const AZStd::string& stringValue)
            {
                return Internal::ConvertStringToJstringImpl(stringValue);
            }

            ////////////////////////////////////////////////////////////////
            int GetRefType(jobject javaRef)
            {
                int refType = JNIInvalidRefType;

                if (javaRef)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Unable to get JNIEnv pointer to determine JNI reference type.");

                    if (jniEnv)
                    {
                        refType = jniEnv->GetObjectRefType(javaRef);
                    }
                }

                return refType;
            }

            ////////////////////////////////////////////////////////////////
            void DeleteRef(jobject javaRef)
            {
                if (javaRef)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Unable to get JNIEnv pointer to free JNI reference.");

                    if (jniEnv)
                    {
                        jobjectRefType refType = jniEnv->GetObjectRefType(javaRef);
                        switch (refType)
                        {
                            case JNIGlobalRefType:
                                jniEnv->DeleteGlobalRef(javaRef);
                                break;
                            case JNILocalRefType:
                                jniEnv->DeleteLocalRef(javaRef);
                                break;
                            case JNIWeakGlobalRefType:
                                jniEnv->DeleteWeakGlobalRef(javaRef);
                                break;
                            default:
                                AZ_Error("AZ::Android::JNI", false, "Unknown or invalid reference type detected.");
                                break;
                        }
                    }
                }
            }
        }
    }
}
