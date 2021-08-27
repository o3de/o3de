/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/typetraits/typetraits.h>

#include <AzCore/Android/JNI/JNI.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            //! A scoped_ref works in the same way a AZStd::scoped_ptr except it's specificially
            //! designed for the opaque pointer JNI types (e.g. jobject, jarray, etc.).  Guarantees
            //! the java object is released from the JNI environment when the scoped_ref falls
            //! out of scope.
            template<typename JniType>
            class scoped_ref
            {
                static_assert(AZStd::is_convertible<JniType, jobject>::value, "Specified type is not convertible to jobject.");

                typedef scoped_ref<JniType> ThisType;

            public:
                typedef JniType ThisType::* UnspecifiedBoolType;


                // ---

                //! Only explicit scoped_refs are allowed to be constructed
                //! \param javaObject Raw pointer to the java object.  Currently only supports Local and
                //!         Global reference types.  Weak Global reference are NOT supported.
                explicit scoped_ref(JniType javaObject = nullptr)
                    : m_javaObject(javaObject)
                {
                #if defined(AZ_ENABLE_TRACING)
                    if (m_javaObject)
                    {
                        int refType = GetRefType(m_javaObject);
                        AZ_Error("JNI::scoped_ref",
                                refType == JNIGlobalRefType || refType == JNILocalRefType,
                                "Unsupported JNI reference type (%d) used in JNI::scoped_ref.  "
                                "Supported reference types are JNIGlobalRefType and JNILocalRefType.  "
                                "This may lead to unexpected behaviour.", refType);
                    }
                #endif // defined(AZ_ENABLE_TRACING)
                }

                //! Automatically release the reference with the JNI environment when the object
                //! goes out of scope
                ~scoped_ref()
                {
                    DeleteRef(m_javaObject);
                }

                //! Compatibilty with the 'not' operator for validity checks.  Only checkes for raw
                //! pointer validity, NOT if it's pointing to a null reference (weak global ref).
                bool operator !() const
                {
                    return (m_javaObject == nullptr);
                }

                //! Operator for implicit bool conversions
                //! \return 'True' if internal reference is valid, False otherwise
                operator UnspecifiedBoolType() const
                {
                    return (m_javaObject == nullptr ? nullptr : &ThisType::m_javaObject);
                }

                //! Explicit accessor of the raw pointer to the java reference.
                //! \return The raw pointer to the java object reference
                JniType get() const
                {
                    return m_javaObject;
                }

                //! Swap the internal reference with another scoped_ref of the same type
                //! \param lhs The scoped_ref (of same type) to be swaped with
                void swap(scoped_ref& lhs)
                {
                    AZStd::swap(m_javaObject, lhs.m_javaObject);
                }

                //! Reset the internal reference with a new pointer
                //! \param javaObject Raw pointer to the java object.  Must be of same type.
                void reset(JniType javaObject = nullptr)
                {
                    // Pointer level self reset.  Triggering this assert will cause a crash when either this
                    // reference is used, or when the this scoped ref is cleaned up (double/invalid delete).
                    AZ_Assert(javaObject == nullptr || javaObject != m_javaObject, "JNI::scoped_ref pointer level self reset!");

                    // JNI reference level "self" reset.  The references themselves are different so this is a
                    // valid reset, however the underlining java object the references are pointing to
                    // is the same in this case.  As far as the JNI environment is concerned this is ok
                    // but we should still make note of these occurrences.
                    // NOTE: This warning will also trigger in the event the pointers are the same.
                    AZ_Warning("JNI::scoped_ref", GetEnv()->IsSameObject(m_javaObject, javaObject) == JNI_FALSE, "JNI::scoped_ref JNI reference level self reset.");

                    ThisType(javaObject).swap(*this);
                }


            private:
                //! Disable copy/move
                ///@{
                AZ_DISABLE_COPY_MOVE(scoped_ref);
                ///@}

                //! Disable direct comparisons of other scoped_refs
                ///@{
                void operator==(scoped_ref const&) const;
                void operator!=(scoped_ref const&) const;
                ///@}


                // ----

                JniType m_javaObject; //!< Raw pointer of the java object reference (e.g. jobject, jarray, etc.)
            };
        }
    }
}
