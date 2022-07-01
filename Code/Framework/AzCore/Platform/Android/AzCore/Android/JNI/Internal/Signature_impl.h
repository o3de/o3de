/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>
#include <AzCore/Android/JNI/Internal/ClassName.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                template<typename StringType>
                StringType GetTypeSignature(jobject value)
                {
                    StringType signature("");

                    AZ_Error("JNI::Signature", value, "Call to GetTypeSignature with null jobject");
                    if (value)
                    {
                        JNIEnv* jniEnv = GetEnv();
                        AZ_Assert(jniEnv, "Failed to get JNIEnv* on thread for get signature call");

                        if (jniEnv)
                        {
                            jclass objectClass = jniEnv->GetObjectClass(value);

                            StringType typeSig = ClassName<StringType>::GetName(objectClass);
                            signature.reserve(typeSig.size() + 3);

                            signature.append("L");
                            signature.append(typeSig);
                            signature.append(";");

                            jniEnv->DeleteLocalRef(objectClass);

                            AZStd::replace(signature.begin(), signature.end(), '.', '/');
                        }
                    }
                    return signature;
                }

                template<typename StringType>
                StringType GetTypeSignature(jobjectArray value)
                {
                    StringType signature("");

                    AZ_Error("JNI::Signature", value, "Call to GetTypeSignature with null jobjectArray");
                    if (value)
                    {
                        JNIEnv* jniEnv = GetEnv();
                        AZ_Assert(jniEnv, "Failed to get JNIEnv* on thread for get signature call");

                        if (jniEnv)
                        {
                            jobject element = jniEnv->GetObjectArrayElement(value, 0);
                            if (!element || jniEnv->ExceptionCheck())
                            {
                                AZ_Error("JNI::Signature", false, "Unable to determine jobject array type");
                                HANDLE_JNI_EXCEPTION(jniEnv);
                            }
                            else
                            {
                                signature.append("[");
                                signature.append(GetTypeSignature<StringType>(element));

                                jniEnv->DeleteLocalRef(element);
                            }
                        }
                    }
                    return signature;
                }


                template<typename StringType, typename Type>
                bool CompareTypeSignature(const StringType& baseSignature, Type param)
                {
                    return (baseSignature.compare(GetTypeSignature(param)) == 0);
                }

                template<typename StringType>
                bool CompareTypeSignature(const StringType& baseSignature, jobject param)
                {
                    bool result = (!baseSignature.empty() && param);
                    if (result)
                    {
                        // check if the baseSignature is malformed e.g. doesn't start with 'L' or end with ';'
                        if (baseSignature[0] != 'L' || baseSignature[baseSignature.length() - 1] != ';')
                        {
                            return false;
                        }

                        // strip the preceding 'L' and trailing ';' from the class path
                        StringType classPath = baseSignature.substr(1, baseSignature.length() - 2);

                        if (JNIEnv* jniEnv = GetEnv())
                        {
                            // since it's valid to pass a derived java class through JNI we will need
                            // to check if the argument is an instance of the specified signature to
                            // accurately validate the signature
                            jclass signatureClass = LoadClass(classPath.c_str());
                            if (!signatureClass)
                            {
                                AZ_Assert(false, "Unable to load class in signature %s", classPath.c_str());
                                return false;
                            }
                            result = (jniEnv->IsInstanceOf(param, signatureClass) == JNI_TRUE);
                            DeleteRef(signatureClass);
                        }
                        else
                        {
                            result = false;
                        }
                    }
                    return result;
                }

                template<typename StringType>
                bool CompareTypeSignature(const StringType& baseSignature, jobjectArray param)
                {
                    bool result = (!baseSignature.empty() && param);
                    if (result)
                    {
                        // check if the baseSignature is malformed e.g. doesn't start with '['
                        if (baseSignature[0] != '[')
                        {
                            return false;
                        }

                        // strip the preceding '['
                        StringType typeSignature = baseSignature.substr(1, baseSignature.length() - 1);

                        if (JNIEnv* jniEnv = GetEnv())
                        {
                            jobject javaObject = jniEnv->GetObjectArrayElement(static_cast<jobjectArray>(param), 0);
                            result = CompareTypeSignature(typeSignature, javaObject);
                            DeleteRef(javaObject);
                        }
                        else
                        {
                            result = false;
                        }
                    }
                    return result;
                }
            } // namespace Internal


            template<typename StringType>
            template<typename Type, typename... Args>
            bool Signature<StringType>::ValidateImpl(Type firstParam, Args&&... parameters)
            {
                const char* signature = m_signature.c_str();
                const char* currentSignature = &(signature[m_currentIndex]);

                int paramLength;

                // extract the fully qualified class path for java objects
                if (currentSignature[0] == 'L' || (strncmp(currentSignature, "[L", 2) == 0))
                {
                    int endIndex = m_signature.find(';', m_currentIndex);
                    if (endIndex == StringType::npos)
                    {
                        AZ_Assert(false, "The base signature supplied (%s) for validation is malformed", m_signature.c_str());
                        return false;
                    }

                    paramLength = (endIndex - m_currentIndex) + 1; // +1 to include the trailing semicolon
                }
                // otherwise just extract the primitive type char(s)
                else
                {
                    paramLength = ((currentSignature[0] == '[') ? 2 : 1); // primitive types are 1 character signatures, arrays are 2
                }

                // extract the parameter signature and compare the value
                StringType paramSignature = m_signature.substr(m_currentIndex, paramLength);
                if (!Internal::CompareTypeSignature(paramSignature, firstParam))
                {
                    return false;
                }

                m_currentIndex = m_currentIndex + paramLength;
                if (m_currentIndex >= m_signatureLength)
                {
                    return false;
                }

                return ValidateImpl(AZStd::forward<Args>(parameters)...);
            }
        } // namespace JNI
    } // namespace Android
} // namespace AZ
