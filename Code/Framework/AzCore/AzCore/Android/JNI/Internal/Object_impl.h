/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Android/JNI/Internal/ClassName.h>
#include <AzCore/Android/JNI/Internal/JStringUtils.h>

#include <AzCore/Debug/Trace.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/typetraits/is_same.h>


namespace AZ { namespace Android
{
    namespace JNI
    {
        namespace Internal
        {
            // ----
            // Object (public)
            // ----

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            Object<Allocator>::Object(const char* classPath, const char* className /* = nullptr */)
                : m_className()
                , m_stdAllocator()

                , m_classRef(nullptr)
                , m_objectRef(nullptr)

                , m_methods()
                , m_staticMethods()

                , m_fields()
                , m_staticFields()

                , m_ownsGlobalRefs(true)
                , m_instanceConstructed(false)
            {
                m_classRef = LoadClass(classPath);
                if (!className)
                {
                    string_type classPathStr = classPath;
                    size_t last = classPathStr.find_last_of('/');
                    m_className = classPathStr.substr(last + 1, (classPathStr.length() - last) - 1);
                }
                else
                {
                    m_className = className;
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            Object<Allocator>::Object(jclass classRef, jobject objectRef, bool takeOwnership /* = false */)
                : m_className()
                , m_stdAllocator()

                , m_classRef(classRef)
                , m_objectRef(objectRef)

                , m_methods()
                , m_staticMethods()

                , m_fields()
                , m_staticFields()

                , m_ownsGlobalRefs(takeOwnership)
                , m_instanceConstructed(true)
            {
                m_className = ClassName<string_type>::GetSimpleName(classRef);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            Object<Allocator>::~Object()
            {
                if (m_ownsGlobalRefs)
                {
                    JNIEnv* jniEnv = GetEnv();
                    if (jniEnv)
                    {
                        jniEnv->DeleteGlobalRef(m_classRef);
                        jniEnv->DeleteGlobalRef(m_objectRef);
                    }
                    else
                    {
                        AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread object deletion on class %s", m_className.c_str());
                    }
                }

                m_methods.clear();
                m_staticMethods.clear();

                m_fields.clear();
                m_staticFields.clear();
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            bool Object<Allocator>::RegisterMethod(const char* methodName, const char* methodSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register method %s on class %s", methodName, m_className.c_str());
                    return false;
                }

                jmethodID methodId = jniEnv->GetMethodID(m_classRef, methodName, methodSignature);
                if (!methodId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to find method %s with signature %s in class %s", methodName, methodSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JMethodCachePtr newMethod = AZStd::allocate_shared<JMethodCache>(m_stdAllocator);
                newMethod->m_methodId = methodId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetMethodSignature(newMethod, methodName, methodSignature);
            #endif

                m_methods.insert(AZStd::make_pair(methodName, newMethod));
                return true;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            bool Object<Allocator>::RegisterStaticMethod(const char* methodName, const char* methodSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register static method %s on class %s", methodName, m_className.c_str());
                    return false;
                }

                jmethodID methodId = jniEnv->GetStaticMethodID(m_classRef, methodName, methodSignature);
                if (!methodId || jniEnv->ExceptionCheck())
                {
                    AZ_Warning("JNI::Object", false, "Failed to find method %s with signature %s in class %s", methodName, methodSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JMethodCachePtr newMethod = AZStd::allocate_shared<JMethodCache>(m_stdAllocator);
                newMethod->m_methodId = methodId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetMethodSignature(newMethod, methodName, methodSignature);
            #endif

                m_staticMethods.insert(AZStd::make_pair(methodName, newMethod));
                return true;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            bool Object<Allocator>::RegisterNativeMethods(vector_type nativeMethods)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register native methods on class %s", m_className.c_str());
                    return false;
                }

                if (jniEnv->RegisterNatives(m_classRef, nativeMethods.data(), nativeMethods.size()) < 0)
                {
                    AZ_Error("JNI::Object", false, "Failed to register native methods with class %s", m_className.c_str());
                    return false;
                }
                return true;
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            bool Object<Allocator>::RegisterField(const char* fieldName, const char* fieldSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register field %s on class %s", fieldName, m_className.c_str());
                    return false;
                }

                jfieldID fieldId = jniEnv->GetFieldID(m_classRef, fieldName, fieldSignature);
                if (!fieldId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to find field %s with signature %s in class %s", fieldName, fieldSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JFieldCachePtr newField = AZStd::allocate_shared<JFieldCache>(m_stdAllocator);
                newField->m_fieldId = fieldId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetFieldSignature(newField, fieldName, fieldSignature);
            #endif

                m_fields.insert(AZStd::make_pair(fieldName, newField));
                return true;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            bool Object<Allocator>::RegisterStaticField(const char* fieldName, const char* fieldSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register static field %s on class %s", fieldName, m_className.c_str());
                    return false;
                }

                jfieldID fieldId = jniEnv->GetStaticFieldID(m_classRef, fieldName, fieldSignature);
                if (!fieldId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to find static field %s with signature %s in class %s", fieldName, fieldSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JFieldCachePtr newField = AZStd::allocate_shared<JFieldCache>(m_stdAllocator);
                newField->m_fieldId = fieldId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetFieldSignature(newField, fieldName, fieldSignature);
            #endif

                m_staticFields.insert(AZStd::make_pair(fieldName, newField));
                return true;
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            bool Object<Allocator>::CreateInstance(const char* constructorSignature, Args&&... parameters)
            {
                if (m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The Instance has already been created");
                    return false;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for constructor call on class %s", m_className.c_str());
                    return false;
                }

                // get the constructor method
                jmethodID constructorMethodId = jniEnv->GetMethodID(m_classRef, "<init>", constructorSignature);
                if (!constructorMethodId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get the constructor for class %s with signature %s", m_className.c_str(), constructorSignature);
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                {
                    JMethodCachePtr newMethod = AZStd::allocate_shared<JMethodCache>(m_stdAllocator);
                    SetMethodSignature(newMethod, "<inti>", constructorSignature);

                    SignatureOutcome outcome = ValidateSignature(newMethod->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                    if (!outcome.IsSuccess())
                    {
                        const string_type& errorMessage = outcome.GetError();
                        AZ_Error("JNI::Object", false, "Invalid constructor signature supplied for class %s. %s", m_className.c_str(), errorMessage.c_str());
                        HANDLE_JNI_EXCEPTION(jniEnv);
                        return false;
                    }
                }
            #endif

                jobject localObjectRef = jniEnv->NewObject(m_classRef, constructorMethodId, AZStd::forward<Args>(parameters)...);
                if (!localObjectRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to construct a local object of class %s", m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localObjectRef);
                    return false;
                }

                m_objectRef = jniEnv->NewGlobalRef(localObjectRef);
                if (!m_objectRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to construct a global object of class %s", m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localObjectRef);
                    return false;
                }

                jniEnv->DeleteLocalRef(localObjectRef);
                m_instanceConstructed = true;

                return true;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::DestroyInstance()
            {
                if (!m_ownsGlobalRefs)
                {
                    AZ_Warning("JNI::Object", false, "This object does not have ownership of the global references.  Instance not destroyed");
                    return;
                }

                if (m_instanceConstructed)
                {
                    DeleteRef(m_objectRef);
                    m_objectRef = nullptr;

                    m_instanceConstructed = false;
                }
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            void Object<Allocator>::InvokeVoidMethod(const char* methodName, Args&&... parameters)
            {
                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance method call to %s failed for class %s", methodName, m_className.c_str());
                    return;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for method call %s on class %s", methodName, m_className.c_str());
                    return;
                }

                JMethodCachePtr methodCache = GetMethod(methodName);
                if (!methodCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome outcome = ValidateSignature(methodCache->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                if (!outcome.IsSuccess())
                {
                    const string_type& errorMessage = outcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return;
                }

                if (methodCache->m_returnSignature.compare("V") != 0)
                {
                    AZ_Error("JNI::Object", false, "Invalid return signature supplied for method call %s on class %s.\nExpecting: %s\nActual: V",
                            methodCache->m_methodName.c_str(), m_className.c_str(), methodCache->m_returnSignature.c_str());
                    return;
                }
            #endif

                jniEnv->CallVoidMethod(m_objectRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jboolean Object<Allocator>::InvokeBooleanMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jboolean>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallBooleanMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jbyte Object<Allocator>::InvokeByteMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jbyte>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallByteMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jchar Object<Allocator>::InvokeCharMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jchar>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallCharMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jshort Object<Allocator>::InvokeShortMethod(const char *methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jshort>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallShortMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename... Args>
            jint Object<Allocator>::InvokeIntMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jint>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallIntMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jlong Object<Allocator>::InvokeLongMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jlong>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallLongMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jfloat Object<Allocator>::InvokeFloatMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jfloat>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallFloatMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);        }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jdouble Object<Allocator>::InvokeDoubleMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jdouble>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallDoubleMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            typename Object<Allocator>::string_type Object<Allocator>::InvokeStringMethod(const char* methodName, Args&&... parameters)
            {
                jstring rawStringValue = InvokeObjectMethod<jstring>(methodName, AZStd::forward<Args>(parameters)...);
                if (!rawStringValue)
                {
                    return "";
                }
                string_type returnValue = Internal::ConvertJstringToStringImpl<string_type>(rawStringValue);
                DeleteRef(rawStringValue);
                return returnValue;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object<Allocator>::InvokeObjectMethod(const char* methodName, Args&&... parameters)
            {
                ReturnType defaultValue = nullptr;

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The Java object instance has not been created yet.  Instance method call to %s failed for class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(methodCache->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                SignatureOutcome returnOutcome = ValidateSignaturePartial(methodCache->m_returnSignature, defaultValue);
                if (!returnOutcome.IsSuccess())
                {
                    const string_type& errorMessage = returnOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid return signature supplied for method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                jobject localRef = jniEnv->CallObjectMethod(m_objectRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (!localRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject globalRef = jniEnv->NewGlobalRef(localRef);
                if (!globalRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a return global ref to method call %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localRef);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localRef);
                return static_cast<ReturnType>(globalRef);
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            void Object<Allocator>::InvokeStaticVoidMethod(const char* methodName, Args&&... parameters)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static method call %s on class %s", methodName, m_className.c_str());
                    return;
                }

                JMethodCachePtr methodCache = GetStaticMethod(methodName);
                if (!methodCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome outcome = ValidateSignature(methodCache->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                if (!outcome.IsSuccess())
                {
                    const string_type& errorMessage = outcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return;
                }

                if (methodCache->m_returnSignature.compare("V") != 0)
                {
                    AZ_Error("JNI::Object", false, "Invalid return signature supplied for static method call %s on class %s.\nExpecting: %s\nActual: V",
                            methodCache->m_methodName.c_str(), m_className.c_str(), methodCache->m_returnSignature.c_str());
                    return;
                }
            #endif

                jniEnv->CallStaticVoidMethod(m_classRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke static method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jboolean Object<Allocator>::InvokeStaticBooleanMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jboolean>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticBooleanMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jbyte Object<Allocator>::InvokeStaticByteMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jbyte>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticByteMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jchar Object<Allocator>::InvokeStaticCharMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jchar>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticCharMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jshort Object<Allocator>::InvokeStaticShortMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jshort>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticShortMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename... Args>
            jint Object<Allocator>::InvokeStaticIntMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jint>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticIntMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jlong Object<Allocator>::InvokeStaticLongMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jlong>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticLongMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jfloat Object<Allocator>::InvokeStaticFloatMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jfloat>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticFloatMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            jdouble Object<Allocator>::InvokeStaticDoubleMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jdouble>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticDoubleMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ...Args>
            typename Object<Allocator>::string_type Object<Allocator>::InvokeStaticStringMethod(const char* methodName, Args&&... parameters)
            {
                jstring rawStringValue = InvokeStaticObjectMethod<jstring>(methodName, AZStd::forward<Args>(parameters)...);
                string_type returnValue = Internal::ConvertJstringToStringImpl<string_type>(rawStringValue);
                DeleteRef(rawStringValue);
                return returnValue;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object<Allocator>::InvokeStaticObjectMethod(const char* methodName, Args&&... parameters)
            {
                const ReturnType defaultValue = nullptr;

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetStaticMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(methodCache->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                SignatureOutcome returnOutcome = ValidateSignaturePartial(methodCache->m_returnSignature, defaultValue);
                if (!returnOutcome.IsSuccess())
                {
                    const string_type& errorMessage = returnOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid return signature supplied for static method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                jobject localRef = jniEnv->CallStaticObjectMethod(m_classRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (!localRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke static method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject globalRef = jniEnv->NewGlobalRef(localRef);
                if (!globalRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a return global ref to method call %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localRef);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localRef);
                return static_cast<ReturnType>(globalRef);
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetBooleanField(const char* fieldName, jboolean value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetBooleanField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetByteField(const char* fieldName, jbyte value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetByteField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetCharField(const char* fieldName, jchar value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetCharField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetShortField(const char* fieldName, jshort value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetShortField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetIntField(const char* fieldName, jint value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetIntField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetLongField(const char* fieldName, jlong value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetLongField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetFloatField(const char* fieldName, jfloat value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetFloatField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetDoubleField(const char* fieldName, jdouble value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetDoubleField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStringField(const char* fieldName, const string_type& value)
            {
                jstring argString = Internal::ConvertStringToJstringImpl(value);
                if (!argString)
                {
                    AZ_Error("JNI::Object", false, "Failed to set string field %s in class %s due to string conversion failure.", fieldName, m_className.c_str());
                    return;
                }

                SetObjectField<jstring>(fieldName, argString);

                DeleteRef(argString);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ValueType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ValueType, jobject>::value>::type
            Object<Allocator>::SetObjectField(const char* fieldName, ValueType value)
            {
                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The Java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(fieldCache->m_signature, value);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return;
                }
            #endif

                jniEnv->SetObjectField(m_objectRef, fieldCache->m_fieldId, value);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jboolean Object<Allocator>::GetBooleanField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jboolean>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetBooleanField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jbyte Object<Allocator>::GetByteField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jbyte>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetByteField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jchar Object<Allocator>::GetCharField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jchar>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetCharField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jshort Object<Allocator>::GetShortField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jshort>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetShortField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jint Object<Allocator>::GetIntField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jint>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetIntField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jlong Object<Allocator>::GetLongField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jlong>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetLongField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jfloat Object<Allocator>::GetFloatField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jfloat>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetFloatField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jdouble Object<Allocator>::GetDoubleField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jdouble>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetDoubleField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            typename Object<Allocator>::string_type Object<Allocator>::GetStringField(const char* fieldName)
            {
                jstring rawString = GetObjectField<jstring>(fieldName);
                string_type returnValue = Internal::ConvertJstringToStringImpl<string_type>(rawString);
                DeleteRef(rawString);
                return returnValue;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object<Allocator>::GetObjectField(const char* fieldName)
            {
                const ReturnType defaultValue(nullptr);

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignaturePartial(fieldCache->m_signature, defaultValue);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                jobject localValue = jniEnv->GetObjectField(m_objectRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject returnValue = jniEnv->NewGlobalRef(localValue);
                if (!returnValue || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a global ref from field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localValue);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localValue);

                return static_cast<ReturnType>(returnValue);
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticBooleanField(const char* fieldName, jboolean value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticBooleanField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticByteField(const char* fieldName, jbyte value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticByteField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticCharField(const char* fieldName, jchar value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticCharField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticShortField(const char* fieldName, jshort value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticShortField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticIntField(const char* fieldName, jint value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticIntField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticLongField(const char* fieldName, jlong value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetLongField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticFloatField(const char* fieldName, jfloat value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetFloatField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticDoubleField(const char* fieldName, jdouble value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticDoubleField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetStaticStringField(const char* fieldName, const string_type& value)
            {
                jstring argString = Internal::ConvertStringToJstringImpl(value);
                if (!argString)
                {
                    AZ_Error("JNI::Object", false, "Failed to set string field %s in class %s due to string conversion failure.", fieldName, m_className.c_str());
                    return;
                }

                SetStaticObjectField<jstring>(fieldName, argString);

                DeleteRef(argString);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ValueType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ValueType, jobject>::value>::type
            Object<Allocator>::SetStaticObjectField(const char* fieldName, ValueType value)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(fieldCache->m_signature, value);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return;
                }
            #endif

                jniEnv->SetObjectField(m_objectRef, fieldCache->m_fieldId, value);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jboolean Object<Allocator>::GetStaticBooleanField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jboolean>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticBooleanField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jbyte Object<Allocator>::GetStaticByteField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jbyte>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticByteField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jchar Object<Allocator>::GetStaticCharField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jchar>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticCharField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jshort Object<Allocator>::GetStaticShortField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jshort>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticShortField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jint Object<Allocator>::GetStaticIntField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jint>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticIntField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jlong Object<Allocator>::GetStaticLongField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jlong>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticLongField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jfloat Object<Allocator>::GetStaticFloatField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jfloat>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticFloatField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            jdouble Object<Allocator>::GetStaticDoubleField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jdouble>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticDoubleField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            typename Object<Allocator>::string_type Object<Allocator>::GetStaticStringField(const char* fieldName)
            {
                jstring rawString = GetStaticObjectField<jstring>(fieldName);
                string_type returnValue = Internal::ConvertJstringToStringImpl<string_type>(rawString);
                DeleteRef(rawString);
                return returnValue;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object<Allocator>::GetStaticObjectField(const char* fieldName)
            {
                const ReturnType defaultValue(nullptr);

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignaturePartial(fieldCache->m_signature, defaultValue);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                jobject localValue = jniEnv->GetStaticObjectField(m_classRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject returnValue = jniEnv->NewGlobalRef(localValue);
                if (!returnValue || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a global ref from static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localValue);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localValue);

                return static_cast<ReturnType>(returnValue);
            }


            // ----
            // Object (private)
            // ----


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            typename Object<Allocator>::JMethodCachePtr Object<Allocator>::GetMethod(const string_type& methodName) const
            {
                typename JMethodMap::const_iterator methodIter = m_methods.find(methodName);
                if (methodIter == m_methods.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered method %s in class %s", methodName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return methodIter->second;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            typename Object<Allocator>::JMethodCachePtr Object<Allocator>::GetStaticMethod(const string_type& methodName) const
            {
                typename JMethodMap::const_iterator methodIter = m_staticMethods.find(methodName);
                if (methodIter == m_methods.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered static method %s in class %s", methodName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return methodIter->second;
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            typename Object<Allocator>::JFieldCachePtr Object<Allocator>::GetField(const string_type& fieldName) const
            {
                typename JFieldMap::const_iterator fieldIter = m_fields.find(fieldName);
                if (fieldIter == m_fields.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered field %s in class %s", fieldName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return fieldIter->second;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            typename Object<Allocator>::JFieldCachePtr Object<Allocator>::GetStaticField(const string_type& fieldName) const
            {
                typename JFieldMap::const_iterator fieldIter = m_staticFields.find(fieldName);
                if (fieldIter == m_staticFields.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered static method %s in class %s", fieldName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return fieldIter->second;
            }


        #if defined(JNI_SIGNATURE_VALIDATION)
            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetMethodSignature(JMethodCachePtr methodCache, const char* methodName, const char* methodSignature)
            {
                methodCache->m_methodName = methodName;

                string_type signature = methodSignature;
                size_t first = signature.find('(');
                size_t last = signature.find_last_of(')');

                if ((last - first) > 1)
                {
                    methodCache->m_argumentSignature = signature.substr(first + 1, last - first - 1);
                }

                methodCache->m_returnSignature = signature.substr(last + 1, signature.size() - last - 1);
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            void Object<Allocator>::SetFieldSignature(JFieldCachePtr fieldCache, const char* fieldName, const char* signature)
            {
                fieldCache->m_fieldName = fieldName;
                fieldCache->m_signature = signature;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename... Args>
            typename Object<Allocator>::SignatureOutcome Object<Allocator>::ValidateSignature(const string_type& baseSignature, Args&&... parameters)
            {
                bool validSignature = SigUtil::Validate(baseSignature, AZStd::forward<Args>(parameters)...);
                if (!validSignature)
                {
                    string_type paramSignature = SigUtil::Generate(AZStd::forward<Args>(parameters)...);
                    return AZ::Failure<string_type>(string_type::format("Expecting: %s\nActual: %s", baseSignature.c_str(), paramSignature.c_str()));
                }
                return AZ::Success();
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename Type>
            typename Object<Allocator>::SignatureOutcome Object<Allocator>::ValidateSignaturePartial(const string_type& baseSignature, Type param)
            {
                bool validSignature;
                bool isParamValid;
                if (AZStd::is_same<Type, jobject>::value)
                {
                    isParamValid = (param != nullptr);
                    validSignature = (baseSignature.compare(0, 1, "L") == 0);
                }
                else if (AZStd::is_same<Type, jobjectArray>::value)
                {
                    isParamValid = (param != nullptr);
                    validSignature = (baseSignature.compare(0, 2, "[L") == 0);
                }
                else
                {
                    isParamValid = true;
                    validSignature = SigUtil::Validate(baseSignature, param);
                }

                if (!validSignature)
                {
                    string_type paramSignature = (isParamValid ? SigUtil::Generate(param).c_str() : "Unknown");
                    return AZ::Failure<string_type>(string_type::format("Expecting: %s\nActual: %s", baseSignature.c_str(), paramSignature.c_str()));
                }
                return AZ::Success();
            }
        #endif // defined(JNI_SIGNATURE_VALIDATION)


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType, typename... Args>
            ReturnType Object<Allocator>::InvokePrimitiveTypeMethodInternal(const char* methodName, JniMethodCallback<ReturnType> jniCallback, Args&&... parameters)
            {
                const ReturnType defaultValue(0);

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance method call to %s failed for class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(methodCache->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                SignatureOutcome returnOutcome = ValidateSignature(methodCache->m_returnSignature, defaultValue);
                if (!returnOutcome.IsSuccess())
                {
                    const string_type& errorMessage = returnOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid return signature supplied for method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_objectRef, methodCache->m_methodId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType, typename... Args>
            ReturnType Object<Allocator>::InvokePrimitiveTypeStaticMethodInternal(const char* methodName, JniStaticMethodCallback<ReturnType> jniCallback, Args&&... parameters)
            {
                const ReturnType defaultValue(0);

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetStaticMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(methodCache->m_argumentSignature, AZStd::forward<Args>(parameters)...);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                SignatureOutcome returnOutcome = ValidateSignature(methodCache->m_returnSignature, defaultValue);
                if (!returnOutcome.IsSuccess())
                {
                    const string_type& errorMessage = returnOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid return signature supplied for static method call %s on class %s. %s",
                            methodCache->m_methodName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_classRef, methodCache->m_methodId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke static method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ValueType>
            void Object<Allocator>::SetPrimitiveTypeFieldInternal(const char* fieldName, JniFieldCallback<void> jniCallback, ValueType value)
            {
                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(fieldCache->m_signature, value);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return;
                }
            #endif

                jniCallback(jniEnv, m_objectRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType>
            ReturnType Object<Allocator>::GetPrimitiveTypeFieldInternal(const char* fieldName, JniFieldCallback<ReturnType> jniCallback)
            {
                const ReturnType defaultValue(0);

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(fieldCache->m_signature, defaultValue);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_objectRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }


            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ValueType>
            void Object<Allocator>::SetPrimitiveTypeStaticFieldInternal(const char* fieldName, JniStaticFieldCallback<void> jniCallback, ValueType value)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(fieldCache->m_signature, value);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return;
                }
            #endif

                jniCallback(jniEnv, m_classRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename Allocator>
            template<typename ReturnType>
            ReturnType Object<Allocator>::GetPrimitiveTypeStaticFieldInternal(const char* fieldName, JniStaticFieldCallback<ReturnType> jniCallback)
            {
                const ReturnType defaultValue(0);

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                SignatureOutcome argsOutcome = ValidateSignature(fieldCache->m_signature, defaultValue);
                if (!argsOutcome.IsSuccess())
                {
                    const string_type& errorMessage = argsOutcome.GetError();
                    AZ_Error("JNI::Object", false, "Invalid argument signature supplied for static field %s on class %s. %s",
                            fieldCache->m_fieldName.c_str(), m_className.c_str(), errorMessage.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_classRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }
        } // namespace Internal
    } // namespace JNI
} // namespace Android
} // namespace AZ
