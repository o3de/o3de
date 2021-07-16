/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/smart_ptr/shared_count.h>

#include <AzCore/Android/JNI/JNI.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                using sp_counted_base = AZStd::Internal::sp_counted_base;
                using sp_typeinfo = AZStd::type_id;


                //! Similar to the AZStd::Internal::sp_counted_impl_pa in that accepts the data type and
                //! custom allocator type, however the data type is restricted to types that inherit from
                //! jobject. See AzCore/std/smartptr/shared_count.h for more details
                template<typename JniType, typename AllocatorType>
                class sr_counted_impl
                    : public AZStd::Internal::sp_counted_base
                {
                    static_assert(AZStd::is_convertible<JniType, jobject>::value, "Specified type is not convertible to jobject.");

                public:
                    //! Only explicit contruction of the shared_count private impl for shared_refs
                    //! \param javaObject The raw JNI pointer
                    //! \param allocator Custom allocator, used in the deallocation of this particular object,
                    //!                  NOT the JNI pointer
                    sr_counted_impl(JniType javaObject, const AllocatorType& allocator)
                        : m_javaObject(javaObject)
                        , m_allocator(allocator)
                    {
                    }

                    //! Called when the use count drops to zero. DOES release the JNI referene.
                    void dispose() override
                    {
                        DeleteRef(m_javaObject);
                    }

                    //! Called when the weak count drops to zero. Does NOT release the JNI referene.
                    void destroy() override
                    {
                        this->~ThisType();
                        m_allocator.deallocate(this, sizeof(ThisType), AZStd::alignment_of<ThisType>::value);
                    }

                    //! Throwaway pure-virtual.  Custom deleters are not supported for shared_refs since
                    //! they have to be released from the JNI environment
                    void* get_deleter(sp_typeinfo const&) override
                    {
                        return nullptr;
                    }


                private:
                    typedef sr_counted_impl<JniType, AllocatorType> ThisType;

                    //! Disable copy/move
                    ///@{
                    AZ_DISABLE_COPY_MOVE(sr_counted_impl);
                    ///@}


                    // ----

                    JniType m_javaObject; //!< The raw JNI pointer from the JVM
                    AllocatorType m_allocator; //!< Custom alloctor used for the [de]allocation of this object
                };

                //! Similar to the AZStd::Internal::shared_count, however the data type is restricted to
                //! types that inherit from jobject. See AzCore/std/smartptr/shared_count.h for more details
                class shared_count
                {
                public:
                    //! Default contruction, no private impl will be created e.i. the count is not valid
                    shared_count()
                        : m_impl(nullptr)
                    {
                    }

                    //! Explicit construction of the shared_count requiring the raw JNI pointer to manage
                    //! and a custom allocator to handle the private impl count [de]allocations
                    //! \param javaObject Raw JNI pointer from the JVM
                    //! \param allocator Custom allocator used only for the private impl count [de]allocations
                    template<typename JniType, typename Allocator>
                    shared_count(JniType javaObject, const Allocator& allocator)
                        : m_impl(nullptr)
                    {
                        static_assert(AZStd::is_convertible<JniType, jobject>::value, "Specified type is not convertible to jobject.");

                        typedef sr_counted_impl<JniType, Allocator> impl_type;

                        Allocator a2(allocator);
                        m_impl = reinterpret_cast<sp_counted_base*>(a2.allocate(sizeof(impl_type), AZStd::alignment_of<impl_type>::value));
                        if (m_impl)
                        {
                            new(m_impl)impl_type(javaObject, allocator);
                        }
                        else
                        {
                            AZ_Assert(false, "Failed to allocate the shared count for JNI::shared_ref.  Releasing reference from JVM.");
                            DeleteRef(javaObject);
                        }
                    }

                    //! Copy the shared count, increase the count if valid
                    shared_count(shared_count const& rhs)
                        : m_impl(rhs.m_impl)
                    {
                        if (m_impl)
                        {
                            m_impl->add_ref_copy();
                        }
                    }

                    //! Move the shared count, will invalidate the private impl of of the moved shared_count
                    shared_count(shared_count&& rhs)
                        : m_impl(rhs.m_impl)
                    {
                        rhs.m_impl = nullptr;
                    }

                    //! Decrease the shared count, if valid, on deletion
                    ~shared_count()
                    {
                        if (m_impl)
                        {
                            m_impl->release();
                        }
                    }

                    //! Copy the shared count. Increases the new count, if valid; decreases the old count, if valid
                    shared_count& operator =(shared_count const& rhs)
                    {
                        sp_counted_base* tmp = rhs.m_impl;

                        if (tmp != m_impl)
                        {
                            if (tmp)
                            {
                                tmp->add_ref_copy();
                            }

                            if (m_impl)
                            {
                                m_impl->release();
                            }

                            m_impl = tmp;
                        }

                        return *this;
                    }

                    //! Check to see if two shared_counts are managing the same private impl pointer
                    bool operator ==(shared_count const& rhs)
                    {
                        return (m_impl == rhs.m_impl);
                    }

                    //! Swap the private impl pointers between two shared_counts
                    void swap(shared_count& rhs)
                    {
                        AZStd::swap(m_impl, rhs.m_impl);
                    }

                    //! Get the number of reference held by the shared count, if valid
                    long use_count() const
                    {
                        return (m_impl != nullptr ? m_impl->use_count() : 0);
                    }

                    //! Check to see if the shared_count is the only one holding on to the private
                    //! impl pointer
                    bool unique() const
                    {
                        return (use_count() == 1);
                    }


                private:
                    //! The private impl of the shared_count.  This object is the one responsible for
                    //! releasing the JNI reference with the JVM.
                    sp_counted_base* m_impl;
                };
            }

            //! A shared_ref works in the same way a AZStd::shared_ptr except it's specificially
            //! designed for the opaque pointer JNI types (e.g. jobject, jarray, etc.).  Guarantees
            //! the java object is released from the JNI environment once the last shared_ref pointing
            //! to is released.
            template<typename JniType>
            class shared_ref
            {
                static_assert(AZStd::is_convertible<JniType, jobject>::value, "Specified type is not convertible to jobject.");

                typedef shared_ref<JniType> ThisType;

            public:
                typedef JniType ThisType::* UnspecifiedBoolType;


                // ---

                //! Construct a default shared_ref with a null raw JNI pointer
                shared_ref()
                    : m_javaObject(nullptr)
                    , m_count()
                {
                }

                //! Explicit construction of shared_ref with a null raw JNI pointer
                shared_ref(AZStd::nullptr_t)
                    : shared_ref()
                {
                }

                //! Only allow explicit construction from the raw pointer to the java object reference.
                //! Will use the AZ::SytemAllocator for the shared count allocations
                //! \param javaObject Raw pointer to the java object.  Currently only supports Local and
                //!         Global reference types.  Weak Global reference are NOT supported.
                explicit shared_ref(JniType javaObject)
                    : m_javaObject(javaObject)
                    , m_count(javaObject, AZStd::allocator())
                {
                #if defined(AZ_ENABLE_TRACING)
                    if (m_javaObject)
                    {
                        int refType = GetRefType(m_javaObject);
                        AZ_Error("JNI::shared_ref",
                                refType == JNIGlobalRefType || refType == JNILocalRefType,
                                "Unsupported JNI reference type (%d) used in JNI::shared_ref.  "
                                 "Supported reference types are JNIGlobalRefType and JNILocalRefType.  "
                                 "This may lead to unexpected behaviour.", refType);
                    }
                #endif // defined(AZ_ENABLE_TRACING)
                }

                //! Create a shared_ref with a custom allocator.
                //! NOTE: The custom allocator is only used for allocating the shared_count
                //! \param javaObject Raw pointer to the java object.  Currently only supports Local and
                //!         Global reference types.  Weak Global reference are NOT supported.
                //! \param allocator Custom allocator for usage within the shared count.
                template<typename Allocator>
                shared_ref(JniType javaObject, const Allocator& allocator)
                    : m_javaObject(javaObject)
                    , m_count(javaObject, allocator)
                {
                #if defined(AZ_ENABLE_TRACING)
                    if (m_javaObject)
                    {
                        int refType = GetRefType(m_javaObject);
                        AZ_Error("JNI::shared_ref",
                                refType == JNIGlobalRefType || refType == JNILocalRefType,
                                "Unsupported JNI reference type (%d) used in JNI::shared_ref.  "
                                "Supported reference types are JNIGlobalRefType and JNILocalRefType.  "
                                "This may lead to unexpected behaviour.", refType);
                    }
                #endif // defined(AZ_ENABLE_TRACING)
                }

                //! Make a copy of the shared_ref, increase the reference count
                //! \param rhs The shared_ref to copy
                explicit shared_ref(const shared_ref& rhs)
                    : m_javaObject(rhs.m_javaObject)
                    , m_count(rhs.m_count)
                {
                }

                //! Polymorphic copy of a shared_ref
                //! \param rhs The shared_ref of a derived JNI pointer type to be copied
                template<typename Y>
                shared_ref(const shared_ref<Y>& rhs, typename AZStd::enable_if<AZStd::is_convertible<Y, JniType>::value, Y>::type = AZStd::nullptr_t())
                    : m_javaObject(rhs.m_javaObject)
                    , m_count(rhs.m_count)
                {
                }

                //! Move the shared_ref from one shared_ref to another, Ctor
                //! \param rhs The shared_ref to be moved
                shared_ref(shared_ref&& rhs)
                    : m_javaObject(rhs.m_javaObject)
                    , m_count()
                {
                    m_count.swap(rhs.m_count);
                    rhs.m_javaObject = nullptr;
                }

                //! Polymorphic move the shared_ref from one shared_ref to another, Ctor
                //! \param rhs The shared_ref to be moved
                template<typename Y>
                shared_ref(shared_ref<Y>&& rhs)
                    : m_javaObject(rhs.m_javaObject)
                    , m_count()
                {
                    m_count.swap(rhs.m_count);
                    rhs.m_javaObject = nullptr;
                }

                //! Make a copy of the shared_ref, increase the reference count
                //! \param rhs The shared_ref to copy
                shared_ref& operator=(const shared_ref& rhs)     // never throws
                {
                    ThisType(rhs).swap(*this);
                    return *this;
                }

                //! Make a polymorphic copy of the shared_ref, increase the reference count
                //! \param rhs The shared_ref to copy
                template<typename Y>
                shared_ref& operator=(const shared_ref<Y>& rhs)
                {
                    ThisType(rhs).swap(*this);
                    return *this;
                }

                //! Move the shared_ref from one shared_ref to another
                //! \param rhs The shared_ref to be moved
                shared_ref& operator=(shared_ref&& rhs)
                {
                    ThisType(rhs).swap(*this);
                    return *this;
                }

                //! Polymorphic move of a shared_ref from one shared_ref to another
                //! \param rhs The shared_ref to be moved
                template<typename Y>
                shared_ref& operator=(shared_ref<Y>&& rhs)
                {
                    ThisType(rhs).swap(*this);
                    return *this;
                }

                //! Determine if two shared_refs are the same
                //! \param rhs The shared_ref to compare against, may be of another JNI pointer type
                //! \return True if the raw pointers are the same, False otherwise
                template<typename Y>
                bool operator==(const shared_ref<Y>& rhs) const
                {
                    return this->get() == rhs.get();
                }

                //! Determine if two shared_refs are not the same
                //! \param rhs The shared_ref to compare against, may be of another JNI pointer type
                //! \return True if the raw pointers are the same, False otherwise
                template<typename Y>
                bool operator!=(const shared_ref<Y>& rhs) const
                {
                    return this->get() != rhs.get();
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

                //! Check to see if the shared_ref is the only one holding on to the raw JNI pointer
                //! \return True if the only refernece, False othewise
                bool unique() const
                {
                    return m_count.unique();
                }

                //! Get the number of reference held on the raw JNI pointer
                long use_count() const
                {
                    return m_count.use_count();
                }

                //! Swap the internal reference with another shared_ref of the same type
                //! \param lhs The shared_ref (of same type) to be swaped with
                void swap(shared_ref& lhs)
                {
                    AZStd::swap(m_javaObject, lhs.m_javaObject);
                    m_count.swap(lhs.m_count);
                }

                //! Default reset of the internal reference to nullptr
                void reset()
                {
                    ThisType().swap(*this);
                }

                //! Reset the internal reference with a new pointer
                //! \param javaObject Raw pointer to the java object.  Must be of same type.
                void reset(JniType javaObject)
                {
                    // Pointer level self reset.  Triggering this assert will cause a crash when either this
                    // reference is used, or when the this scoped ref is cleaned up (double/invalid delete).
                    AZ_Assert(javaObject == nullptr || javaObject != m_javaObject, "JNI::shared_ref pointer level self reset!");

                    // JNI reference level "self" reset.  The references themselves are different so this is a
                    // valid reset, however the underlining java object the references are pointing to
                    // is the same in this case.  As far as the JNI environment is concerned this is ok
                    // but we should still make note of these occurrences.
                    // NOTE: This warning will also trigger in the event the pointers are the same.
                    AZ_Warning("JNI::shared_ref", GetEnv()->IsSameObject(m_javaObject, javaObject) == JNI_FALSE, "JNI::shared_ref JNI reference level self reset.");

                    ThisType(javaObject).swap(*this);
                }


            private:
                template<class Y> friend class shared_ref;


                // ----

                JniType m_javaObject; //!< Raw pointer of the java object reference (e.g. jobject, jarray, etc.)
                Internal::shared_count m_count; //!< Shared reference count, responsible for releaseing the JNI reference from the JVM
            };
        }
    }
}
