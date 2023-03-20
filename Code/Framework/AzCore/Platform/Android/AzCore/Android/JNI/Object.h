/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/utils.h>

#include <AzCore/Android/JNI/JNI.h>

#if defined(JNI_SIGNATURE_VALIDATION)
    #include <AzCore/Android/JNI/Signature.h>
    #include <AzCore/Outcome/Outcome.h>
#endif


namespace AZ { namespace Android
{
    namespace JNI
    {
        namespace Internal
        {
            //! Utility to allow easier managing of JNI reference when hosting Java classes and objects
            //! in native code.  Provides the same functionality that is available when manipulating JNI
            //! references directly with the raw JNIEnv pointer.
            //! \tparam Allocator The type of allocator used for both it self and all it's internal
            //!         allocations.  Defaults to AZ::SystemAllocator
            template<typename Allocator = AZ::SystemAllocator>
            class Object final
            {
            private:
                //! special case if we are using the SystemAllocator to use the default AZStd::allocator instead of wrapping
                //! the allocator in AZStdAlloc.  This way the known string types (AZStd::string and AZ::OSString) are correctly
                //! typedefed internally.
                typedef typename AZStd::conditional<AZStd::is_same<Allocator, AZ::SystemAllocator>::value, AZStd::allocator, AZStdAlloc<Allocator>>::type AZStdAllocator;


            public:
                typedef AZStd::basic_string<char, AZStd::char_traits<char>, AZStdAllocator> string_type;
                typedef AZStd::vector<JNINativeMethod, AZStdAllocator> vector_type;


                AZ_CLASS_ALLOCATOR(Object<Allocator>, Allocator);


                //! Creates a custom jni object wrapper from a java class path.  This JNI object
                //! will take owner ship of all global refs used internally
                //! \param classPath  The full java class path for the object to be loaded
                //! \param className  The name of the java class, mostly use for logging purposes
                explicit Object(const char* classPath, const char* className = nullptr);

                //! Creates a JNI object wrapper based on an existing global ref
                //! \param classRef  The global reference to the jclass for the object
                //! \param objectRef  The global reference to the instance object
                //! \param takeOwnership  [Optional] Tell the object to clean up the argument global refs when destroyed
                Object(jclass classRef, jobject objectRef, bool takeOwnership = false);

                //! Automatically cleans up any global JNI reference with the JVM
                ~Object();


                //! Register a non-static Java method with the associated Java object instance.  These methods can only
                //! be invoked with a valid jobject reference.
                //! \param methodName The exact name of the java method to register
                //! \param methodSignature The argument/return signature of the java method.
                //!        See http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp16432 for more details
                //! \return True if the method was registered successfully, False otherwise
                bool RegisterMethod(const char* methodName, const char* methodSignature);

                //! Register a static Java method with the associated Java class reference.  These methods
                //! can be invoked as long as the class reference is valid.
                //! \param methodName The exact name of the java method to register
                //! \param methodSignature The argument/return signature of the Java method.
                //!        See http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp16432 for more details
                //! \return True if the method was registered successfully, False otherwise
                bool RegisterStaticMethod(const char* methodName, const char* methodSignature);

                //! Register native callback with Java 'native' method.
                //! \param nativeMethods All the native methods to register with the Java class.
                //! \return True if all the methods were registered successfully, False otherwise
                bool RegisterNativeMethods(vector_type nativeMethods);

                //! Register a instance member field with the object.
                //! \param fieldName The exact name of the java field to register.
                //! \param fieldSignature The type signature of the Java field.
                //!        See http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp16432 for more details
                //! \return True if the field was registered successfully, False otherwise.
                bool RegisterField(const char* fieldName, const char* fieldSignature);

                //! Register a static member field with the object.
                //! \param fieldName The exact name of the java static field to register.
                //! \param fieldSignature The type signature of the Java static field.
                //!        See http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp16432 for more details
                //! \return True if the static field was registered successfully, False otherwise.
                bool RegisterStaticField(const char* fieldName, const char* fieldSignature);


                //! Creates a global reference to an java instance object
                //! \param constructorSignature  The function signature of the constructor desired for creating the object
                //!        See http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp16432 for more details
                //! \param parameters All the arguments required for the function call
                //! \return True if the global instance was created successfully, False otherwise
                template<typename... Args>
                bool CreateInstance(const char* constructorSignature, Args&&... parameters);

                //! Destroys the global instance of the java object only.  Static method calls can still be made, while instance methods will
                //! fail until a new instance is constructed through CreateInstance
                void DestroyInstance();


                //!@{
                //! All the Invoke<TYPE>Method functions are for calling registered instance methods on
                //! a java object where <TYPE> is the return type of the java method.
                //! \param methodName The exact name of the java method to call
                //! \param parameters All the arguments required for the function call
                template<typename... Args>
                void InvokeVoidMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jboolean InvokeBooleanMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jbyte InvokeByteMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jchar InvokeCharMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jshort InvokeShortMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jint InvokeIntMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jlong InvokeLongMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jfloat InvokeFloatMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jdouble InvokeDoubleMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                string_type InvokeStringMethod(const char* methodName, Args&&... parameters);
                //!@}

                //! Call a java instance method that returns a customs java object such as an String, Array
                //! or other java class type.  This function is restricted to types derived from _jobject.
                //! The return value will be a global reference and the caller is responsible for deleting
                //! through Jni::DeleteGlobalRef when no longer needed.
                template<typename ReturnType, typename... Args>
                typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
                InvokeObjectMethod(const char* methodName, Args&&... parameters);


                //!@{
                //! All the InvokeStatic<TYPE>Method functions are for calling registered static methods on
                //! a java object where <TYPE> is the return type of the java method.
                //! \param methodName The exact name of the java method to call
                //! \param parameters All the arguments required for the function call
                template<typename... Args>
                void InvokeStaticVoidMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jboolean InvokeStaticBooleanMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jbyte InvokeStaticByteMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jchar InvokeStaticCharMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jshort InvokeStaticShortMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jint InvokeStaticIntMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jlong InvokeStaticLongMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jfloat InvokeStaticFloatMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                jdouble InvokeStaticDoubleMethod(const char* methodName, Args&&... parameters);

                template<typename... Args>
                string_type InvokeStaticStringMethod(const char* methodName, Args&&... parameters);
                //!@}

                //! Call a java static method that returns a customs java object such as an String, Array or
                //! other java class type.  This function is restricted to types derived from _jobject.
                //! The return value will be a global reference and the caller is responsible for deleting
                //! through Jni::DeleteGlobalRef when no longer needed.
                template<typename ReturnType, typename... Args>
                typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
                InvokeStaticObjectMethod(const char* methodName, Args&&... parameters);


                //!@{
                //! All the Set<TYPE>Field functions are for setting a registered instance member on
                //! a java object where <TYPE> is the type of the instance member.
                //! \param fieldName The exact name of the java instance member to set.
                //! \param value The new value to set the instance member.
                void SetBooleanField(const char* fieldName, jboolean value);
                void SetByteField(const char* fieldName, jbyte value);
                void SetCharField(const char* fieldName, jchar value);
                void SetShortField(const char* fieldName, jshort value);
                void SetIntField(const char* fieldName, jint value);
                void SetLongField(const char* fieldName, jlong value);
                void SetFloatField(const char* fieldName, jfloat value);
                void SetDoubleField(const char* fieldName, jdouble value);
                void SetStringField(const char* fieldName, const string_type& value);
                //!@}

                //! Set a custom java object instance field.  Restricted to types derived from _jobject,
                //! such as jstring, jarray, etc.
                template<typename ValueType, typename... Args>
                typename AZStd::enable_if<AZStd::is_convertible<ValueType, jobject>::value>::type
                SetObjectField(const char* fieldName, ValueType value);

                //!@{
                //! All the Get<TYPE>Field functions are for getting a registered instance member on
                //! a java object where <TYPE> is the type of the instance member.
                //! \param fieldName The exact name of the java instance member to get.
                jboolean    GetBooleanField(const char* fieldName);
                jbyte       GetByteField(const char* fieldName);
                jchar       GetCharField(const char* fieldName);
                jshort      GetShortField(const char* fieldName);
                jint        GetIntField(const char* fieldName);
                jlong       GetLongField(const char* fieldName);
                jfloat      GetFloatField(const char* fieldName);
                jdouble     GetDoubleField(const char* fieldName);
                string_type GetStringField(const char* fieldName);
                //!@}

                //! Get a custom java object instance field.  Restricted to types derived from _jobject,
                //! such as jstring, jarray, etc.  A global refernece will be returned and the caller is
                //! responsible for deleting through Jni::DeleteGlobalRef when no longer needed.
                template<typename ReturnType, typename... Args>
                typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
                GetObjectField(const char* fieldName);


                //!@{
                //! All the Set<TYPE>Field functions are for setting a registered instance member on
                //! a java object where <TYPE> is the type of the instance member.
                //! \param fieldName The exact name of the java instance member to set.
                //! \param value The new value to set the static member.
                void SetStaticBooleanField(const char* fieldName, jboolean value);
                void SetStaticByteField(const char* fieldName, jbyte value);
                void SetStaticCharField(const char* fieldName, jchar value);
                void SetStaticShortField(const char* fieldName, jshort value);
                void SetStaticIntField(const char* fieldName, jint value);
                void SetStaticLongField(const char* fieldName, jlong value);
                void SetStaticFloatField(const char* fieldName, jfloat value);
                void SetStaticDoubleField(const char* fieldName, jdouble value);
                void SetStaticStringField(const char* fieldName, const string_type& value);
                //!@}

                //! Set a custom java object static field.  Restricted to types derived from _jobject,
                //! such as jstring, jarray, etc.
                template<typename ValueType, typename... Args>
                typename AZStd::enable_if<AZStd::is_convertible<ValueType, jobject>::value>::type
                SetStaticObjectField(const char* fieldName, ValueType value);


                //!@{
                //! All the Get<TYPE>Field functions are for getting a registered static member on
                //! a java object where <TYPE> is the type of the static member.
                //! \param fieldName The exact name of the java instance member to get.
                jboolean    GetStaticBooleanField(const char* fieldName);
                jbyte       GetStaticByteField(const char* fieldName);
                jchar       GetStaticCharField(const char* fieldName);
                jshort      GetStaticShortField(const char* fieldName);
                jint        GetStaticIntField(const char* fieldName);
                jlong       GetStaticLongField(const char* fieldName);
                jfloat      GetStaticFloatField(const char* fieldName);
                jdouble     GetStaticDoubleField(const char* fieldName);
                string_type GetStaticStringField(const char* fieldName);
                //!@}

                //! Get a custom java object static field.  Restricted to types derived from _jobject,
                //! such as jstring, jarray, etc.  A global reference will be returned and the caller is
                //! responsible for deleting through Jni::DeleteGlobalRef when no longer needed.
                template<typename ReturnType, typename... Args>
                typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
                GetStaticObjectField(const char* fieldName);


            private:
                //! Simple structure containing core information about a registered Java method
                struct JMethodCache
                {
                    jmethodID m_methodId; //!< Pointer to the method reference on the JVM

                #if defined(JNI_SIGNATURE_VALIDATION)
                    string_type m_methodName; //!< Name of the Java method, mostly used for debug logs
                    string_type m_argumentSignature; //!< Java type signature of all method arguments e.g. (int, String) => ILjava/lang/String;
                    string_type m_returnSignature; //!< Java type signature of the return value
                #endif
                };

                //! Simple structure containing core information about a registered Java field
                struct JFieldCache
                {
                    jfieldID m_fieldId; //!< Pointer to the field reference on the JVM

                #if defined(JNI_SIGNATURE_VALIDATION)
                    string_type m_fieldName; //!< Name of the Java field, mostly used for debug logs
                    string_type m_signature; //!< Java type signature of the field
                #endif
                };


                typedef AZStd::shared_ptr<JMethodCache> JMethodCachePtr;
                typedef AZStd::shared_ptr<JFieldCache> JFieldCachePtr;

                template<typename ValueType>
                using CacheMap = AZStd::unordered_map<string_type, ValueType, AZStd::hash<string_type>, AZStd::equal_to<string_type>, AZStdAllocator>;

                typedef CacheMap<JMethodCachePtr> JMethodMap;
                typedef CacheMap<JFieldCachePtr> JFieldMap;


                template<typename ReturnType>
                using JniMethodCallback = AZStd::function<ReturnType(JNIEnv*, jobject, jmethodID)>;

                template<typename ReturnType>
                using JniStaticMethodCallback = AZStd::function<ReturnType(JNIEnv*, jclass, jmethodID)>;

                template<typename ReturnType>
                using JniFieldCallback = AZStd::function<ReturnType(JNIEnv*, jobject, jfieldID)>;

                template<typename ReturnType>
                using JniStaticFieldCallback = AZStd::function<ReturnType(JNIEnv*, jclass, jfieldID)>;


            #if defined(JNI_SIGNATURE_VALIDATION)
                using SignatureOutcome = AZ::Outcome<void, string_type>;

                typedef Signature<string_type> SigUtil;
            #endif


                // ----

                //! Helper to find a register instance method
                JMethodCachePtr GetMethod(const string_type& methodName) const;

                //! Helper to find a register static method
                JMethodCachePtr GetStaticMethod(const string_type& methodName) const;

                //! Helper to find a register instance field
                JFieldCachePtr GetField(const string_type& fieldName) const;

                //! Helper to find a register static field
                JFieldCachePtr GetStaticField(const string_type& fieldName) const;


            #if defined(JNI_SIGNATURE_VALIDATION)
                //! Helper to extract the argument and return signatures from a complete method signature
                //! and set the respective properties within the specified JMethodCache pointer.
                //! \param methodCache JMethodCache pointer to set
                //! \param methodName The exact name of the java method
                //! \param methodSignature The full method signature
                void SetMethodSignature(JMethodCachePtr methodCache, const char* methodName, const char* methodSignature);

                //! Helper to extract the argument and return signatures from a complete method signature
                //! and set the respective properties within the specified JMethodCache pointer.
                //! \param fieldCache JFieldCache pointer to set
                //! \param fieldName The exact name of the java field
                //! \param signature The signature, or type, of the java field
                void SetFieldSignature(JFieldCachePtr fieldCache, const char* fieldName, const char* signature);

                //! Performs a full signature validation for all types in Args
                //! \param baseSignature The base signature used to register the JNI method or field
                //! \param parameters Pending arguments to a JNI call needing validation
                //! \return \ref AZ::Success if all parameters pass validation, \ref AZ::Failure<string_type> containing the
                //!         error message otherwise
                template<typename... Args>
                SignatureOutcome ValidateSignature(const string_type& baseSignature, Args&&... parameters);

                //! Performs a partial signature validation when \p Type is jobject or jobjectArray, and a full signature validation
                //! for other types.  Primarily used for basic validation of return type values when the requested type
                //! can't be deduced without making the JNI call first.
                //! \param baseSignature The base signature used to register the JNI method or field
                //! \param param Pending type to a JNI call needing validation
                //! \return \ref AZ::Success if all parameters pass validation, \ref AZ::Failure<string_type> containing the
                //!         error message otherwise
                template<typename Type>
                SignatureOutcome ValidateSignaturePartial(const string_type& baseSignature, Type param);
            #endif // defined(JNI_SIGNATURE_VALIDATION)


                //! Helper for invoking a primitive type instance method on the JNI object
                //! \param methodName The name of the instance method to invoke
                //! \param jniCallback Lambda wrapper to the actual JNIEnv::Call<type>Method
                //! \param parameters Java method call arguments list
                //! \return The primitive return value from the Java method
                template<typename ReturnType, typename... Args>
                ReturnType InvokePrimitiveTypeMethodInternal(const char* methodName, JniMethodCallback<ReturnType> jniCallback, Args&&... parameters);

                //! Helper for invoking a primitive type instance method on the JNI object
                //! \param methodName The name of the instance method to invoke
                //! \param jniCallback Lambda wrapper to the actual JNIEnv::CallStatic<type>Method
                //! \param parameters Java method call arguments list
                //! \return The primitive return value from the Java method
                template<typename ReturnType, typename... Args>
                ReturnType InvokePrimitiveTypeStaticMethodInternal(const char* methodName, JniStaticMethodCallback<ReturnType> jniCallback, Args&&... parameters);


                //! Helper for setting a primitive type instance field
                //! \param fieldName The name of the instance field to set
                //! \param jniCallback Lambda wrapper to the actual JNIEnv::Set<type>Field
                //! \param value New value of the instance field
                template<typename ValueType>
                void SetPrimitiveTypeFieldInternal(const char* fieldName, JniFieldCallback<void> jniCallback, ValueType value);

                //! Helper for getting a primitive type instance field
                //! \param fieldName The name of the instance field to set
                //! \param jniCallback Lambda wrapper to the actual JNIEnv::Get<type>Field
                //! \return The current value of the primitive instance field
                template<typename ReturnType>
                ReturnType GetPrimitiveTypeFieldInternal(const char* fieldName, JniFieldCallback<ReturnType> jniCallback);


                //! Helper for setting a primitive type static field
                //! \param fieldName The name of the static field to set
                //! \param jniCallback Lambda wrapper to the actual JNIEnv::SetStatic<type>Field
                //! \param value New value of the static field
                template<typename ValueType>
                void SetPrimitiveTypeStaticFieldInternal(const char* fieldName, JniStaticFieldCallback<void> jniCallback, ValueType value);

                //! Helper for getting a primitive type static field
                //! \param fieldName The name of the static field to set
                //! \param jniCallback Lambda wrapper to the actual JNIEnv::GetStatic<type>Field
                //! \return The current value of the primitive static field
                template<typename ReturnType>
                ReturnType GetPrimitiveTypeStaticFieldInternal(const char* fieldName, JniStaticFieldCallback<ReturnType> jniCallback);


                // ----

                string_type m_className; //!< The simple name of the Java class, used for debugging

                AZStdAllocator m_stdAllocator; //!< Allocator instance used for allocating the JMethodCachePtr/JFieldCachePtr shared pointers

                jclass m_classRef; //!< A global reference to the java class, used for method/filed extraction, static method invocation
                jobject m_objectRef; //!< A global reference to the java object instance, used for instance method invocation, field access

                JMethodMap m_methods; //!< Container of all the instance methods currently registered for the java class
                JMethodMap m_staticMethods; //!< Container of all the static methods currently registered for the java class

                JFieldMap m_fields; //!< Container of all the instance fields currently registered for the java class
                JFieldMap m_staticFields; //!< Container of all the static fields currently registered for the java class

                bool m_ownsGlobalRefs; //!< Should the global references be destroyed automatically or manually
                bool m_instanceConstructed; //!< Can we invoke instance methods, has CreateInstance ben called
            };
        } // namespace Internal

        //! \brief The default \ref AZ::Android::JNI::Internal::Object type which uses the SystemAllocator
        typedef Internal::Object<AZ::SystemAllocator> Object;
    } // namespace JNI
} // namespace Android
} // namespace AZ


#include <AzCore/Android/JNI/Internal/Object_impl.h>
