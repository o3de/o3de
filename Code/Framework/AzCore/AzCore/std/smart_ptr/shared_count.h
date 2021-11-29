/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_SMART_PTR_SHARED_COUNT_H
#define AZSTD_SMART_PTR_SHARED_COUNT_H

//
//  detail/shared_count.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/typetraits/type_id.h>
#include <AzCore/std/smart_ptr/checked_delete.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    namespace Internal
    {
        typedef AZStd::type_id sp_typeinfo;

        class sp_counted_base
        {
        private:
            sp_counted_base(sp_counted_base const&);
            sp_counted_base& operator= (sp_counted_base const&);

            AZStd::atomic<long> use_count_;        // #shared
            AZStd::atomic<long> weak_count_;       // #weak + (#shared != 0)

        public:

            sp_counted_base()
                : use_count_(1)
                , weak_count_(1)
            {}

            virtual ~sp_counted_base() // nothrow
            {}

            // dispose() is called when use_count_ drops to zero, to release
            // the resources managed by *this.

            virtual void dispose() = 0; // nothrow

            // destroy() is called when weak_count_ drops to zero.

            virtual void destroy() = 0; // nothrow

            virtual void* get_deleter(sp_typeinfo const& ti) = 0;
            void add_ref_copy()
            {
                use_count_.fetch_add(1, AZStd::memory_order_acq_rel);
            }

            bool add_ref_lock() // true on success
            {
                long tmp = use_count_.load(AZStd::memory_order_acquire);
                for (;; )
                {
                    if (tmp == 0)
                    {
                        return false;
                    }
                    //weak because spurious failure is ok, we're retrying anyway
                    if (use_count_.compare_exchange_weak(tmp, tmp + 1, AZStd::memory_order_acq_rel, AZStd::memory_order_acquire))
                    {
                        return true;
                    }
                }
            }

            void release() // nothrow
            {
                if (use_count_.fetch_sub(1, AZStd::memory_order_acq_rel) == 1)
                {
                    dispose();
                    weak_release();
                }
            }

            void weak_add_ref() // nothrow
            {
                weak_count_.fetch_add(1, AZStd::memory_order_acq_rel);
            }

            void weak_release() // nothrow
            {
                if (weak_count_.fetch_sub(1, AZStd::memory_order_acq_rel) == 1)
                {
                    destroy();
                }
            }

            long use_count() const // nothrow
            {
                return use_count_.load(AZStd::memory_order_acquire);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // sp_counted_impl_pa
        template<class X, class A>
        class sp_counted_impl_pa
            : public sp_counted_base
        {
        private:
            X* px_;
            A a_; // copy constructor must not throw

            sp_counted_impl_pa(sp_counted_impl_pa const&);
            sp_counted_impl_pa& operator= (sp_counted_impl_pa const&);

            typedef sp_counted_impl_pa<X, A> this_type;
        public:
            explicit sp_counted_impl_pa(X* px, const A& a)
                : px_(px)
                , a_(a)
            {
            }

            void dispose() override // nothrow
            {
                AZStd::checked_delete(px_);
            }
            void destroy() override // nothrow
            {
                this->~this_type();
                a_.deallocate(this, sizeof(this_type), AZStd::alignment_of<this_type>::value);
            }
            void* get_deleter(Internal::sp_typeinfo const&) override
            {
                return 0;
            }
        };

        template<class P, class D, class A>
        class sp_counted_impl_pda
            : public sp_counted_base
        {
        private:

            P p_; // copy constructor must not throw
            D d_; // copy constructor must not throw
            A a_; // copy constructor must not throw

            sp_counted_impl_pda(sp_counted_impl_pda const&);
            sp_counted_impl_pda& operator= (sp_counted_impl_pda const&);

            typedef sp_counted_impl_pda<P, D, A> this_type;

        public:

            // pre: d( p ) must not throw

            sp_counted_impl_pda(P p, D& d, const A& a)
                : p_(p)
                , d_(d)
                , a_(a)
            {
            }

            sp_counted_impl_pda(P p, const A& a)
                : p_(p)
                , a_(a)
            {
            }

            void dispose() override // nothrow
            {
                d_(p_);
            }

            void destroy() override // nothrow
            {
                this->~this_type();
                a_.deallocate(this, sizeof(this_type), AZStd::alignment_of<this_type>::value);
            }

            void* get_deleter(Internal::sp_typeinfo const& ti) override
            {
                return ti == aztypeid(D) ? &reinterpret_cast<char&>(d_) : 0;
            }
        };
        //////////////////////////////////////////////////////////////////////////


        struct sp_nothrow_tag {};

        template< class D >
        struct sp_inplace_tag
        {
        };

        class weak_count;
        class shared_count
        {
        private:
            sp_counted_base* pi_;
            friend class weak_count;
        public:
            shared_count()
                : pi_(0)           // nothrow
            {}
            template<class Y, class A>
            explicit shared_count(Y* p, const A& a)
                : pi_(0)
            {
                typedef sp_counted_impl_pa<Y, A> impl_type;
                A a2(a);
                pi_ = reinterpret_cast<impl_type*>(a2.allocate(sizeof(impl_type), AZStd::alignment_of<impl_type>::value));
                if (pi_ != 0)
                {
                    new(static_cast<void*>(pi_))impl_type(p, a);
                }
                else
                {
                    AZStd::checked_delete(p);
                }
            }
            template<class P, class D, class A>
            shared_count(P p, D d, const A& a)
                : pi_(0)
            {
                typedef sp_counted_impl_pda<P, D, A> impl_type;
                A a2(a);
                pi_ = reinterpret_cast<impl_type*>(a2.allocate(sizeof(impl_type), AZStd::alignment_of<impl_type>::value));
                if (pi_ != 0)
                {
                    new(static_cast< void* >(pi_))impl_type(p, d, a);
                }
                else
                {
                    d(p);
                }
            }

            template< class P, class D, class A >
            shared_count(P p, sp_inplace_tag<D>, const A& a)
                : pi_(0)
            {
                typedef sp_counted_impl_pda<P, D, A> impl_type;
                A a2(a);
                pi_ = reinterpret_cast<impl_type*>(a2.allocate(sizeof(impl_type), AZStd::alignment_of<impl_type>::value));
                if (pi_ != 0)
                {
                    new(static_cast<void*>(pi_))impl_type(p, a);
                }
                else
                {
                    //d( p );
                }
            }

#ifndef AZ_NO_AUTO_PTR
            // auto_ptr<Y> is special cased to provide the strong guarantee
            template<class Y, class A>
            explicit shared_count(std::auto_ptr<Y>& r, const A& a)
                : pi_(0)
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(shared_count_id)
#endif
            {
                typedef sp_counted_impl_pa<Y, A> impl_type;
                A a2(a);
                pi_ = reinterpret_cast<impl_type*>(a2.allocate(sizeof(impl_type), AZStd::alignment_of<impl_type>::value));
                if (pi_ != 0)
                {
                    new(static_cast< void* >(pi_))impl_type(r.get(), a);
                }
                r.release();
            }
#endif // AZ_NO_AUTO_PTR

            ~shared_count() // nothrow
            {
                if (pi_ != 0)
                {
                    pi_->release();
                }
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                id_ = 0;
#endif
            }

            shared_count(shared_count const& r)
                : pi_(r.pi_)                                 // nothrow
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(shared_count_id)
#endif
            {
                if (pi_ != 0)
                {
                    pi_->add_ref_copy();
                }
            }
            shared_count(shared_count&& r)
                : pi_(r.pi_)                            // nothrow
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(shared_count_id)
#endif
            {
                r.pi_ = 0;
            }

            explicit shared_count(weak_count const& r);  // throws bad_weak_ptr when r.use_count() == 0
            shared_count(weak_count const& r, sp_nothrow_tag);    // constructs an empty *this when r.use_count() == 0
            shared_count& operator= (shared_count const& r)   // nothrow
            {
                sp_counted_base* tmp = r.pi_;

                if (tmp != pi_)
                {
                    if (tmp != 0)
                    {
                        tmp->add_ref_copy();
                    }
                    if (pi_ != 0)
                    {
                        pi_->release();
                    }
                    pi_ = tmp;
                }

                return *this;
            }

            void swap(shared_count& r)  // nothrow
            {
                sp_counted_base* tmp = r.pi_;
                r.pi_ = pi_;
                pi_ = tmp;
            }

            long use_count() const // nothrow
            {
                return pi_ != 0 ? pi_->use_count() : 0;
            }

            bool unique() const // nothrow
            {
                return use_count() == 1;
            }

            bool empty() const // nothrow
            {
                return pi_ == 0;
            }

            friend inline bool operator==(shared_count const& a, shared_count const& b)
            {
                return a.pi_ == b.pi_;
            }

            friend inline bool operator<(shared_count const& a, shared_count const& b)
            {
                //return AZStd::less<sp_counted_base *>()( a.pi_, b.pi_ );
                return (a.pi_ < b.pi_);
            }

            void* get_deleter(sp_typeinfo const& ti) const
            {
                return pi_ ? pi_->get_deleter(ti) : 0;
            }
        };

        class weak_count
        {
        private:
            sp_counted_base* pi_;
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            int id_;
#endif
            friend class shared_count;
        public:
            weak_count()
                : pi_(0)         // nothrow
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(weak_count_id)
#endif
            {}

            weak_count(shared_count const& r)
                : pi_(r.pi_)                               // nothrow
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(weak_count_id)
#endif
            {
                if (pi_ != 0)
                {
                    pi_->weak_add_ref();
                }
            }

            weak_count(weak_count const& r)
                : pi_(r.pi_)                             // nothrow
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(weak_count_id)
#endif
            {
                if (pi_ != 0)
                {
                    pi_->weak_add_ref();
                }
            }
            // Move support
            weak_count(weak_count&& r)
                : pi_(r.pi_)                        // nothrow
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                , id_(weak_count_id)
#endif
            {
                r.pi_ = 0;
            }
            ~weak_count() // nothrow
            {
                if (pi_ != 0)
                {
                    pi_->weak_release();
                }
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
                id_ = 0;
#endif
            }

            weak_count& operator= (shared_count const& r)   // nothrow
            {
                sp_counted_base* tmp = r.pi_;

                if (tmp != pi_)
                {
                    if (tmp != 0)
                    {
                        tmp->weak_add_ref();
                    }
                    if (pi_ != 0)
                    {
                        pi_->weak_release();
                    }
                    pi_ = tmp;
                }

                return *this;
            }

            weak_count& operator= (weak_count const& r)   // nothrow
            {
                sp_counted_base* tmp = r.pi_;

                if (tmp != pi_)
                {
                    if (tmp != 0)
                    {
                        tmp->weak_add_ref();
                    }
                    if (pi_ != 0)
                    {
                        pi_->weak_release();
                    }
                    pi_ = tmp;
                }

                return *this;
            }

            void swap(weak_count& r)  // nothrow
            {
                sp_counted_base* tmp = r.pi_;
                r.pi_ = pi_;
                pi_ = tmp;
            }

            long use_count() const // nothrow
            {
                return pi_ != 0 ? pi_->use_count() : 0;
            }

            bool empty() const // nothrow
            {
                return pi_ == 0;
            }

            friend inline bool operator==(weak_count const& a, weak_count const& b)
            {
                return a.pi_ == b.pi_;
            }

            friend inline bool operator<(weak_count const& a, weak_count const& b)
            {
                //return AZStd::less<sp_counted_base *>()(a.pi_, b.pi_);
                return (a.pi_ < b.pi_);
            }
        };

        inline shared_count::shared_count(weak_count const& r)
            : pi_(r.pi_)
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            , id_(shared_count_id)
#endif
        {
            if (pi_ == 0 || !pi_->add_ref_lock())
            {
                AZ_Assert(pi_ != 0, "Bad weak pointer");
            }
        }

        inline shared_count::shared_count(weak_count const& r, sp_nothrow_tag)
            : pi_(r.pi_)
#if defined(AZSTD_SP_ENABLE_DEBUG_HOOKS)
            , id_(shared_count_id)
#endif
        {
            if (pi_ != 0 && !pi_->add_ref_lock())
            {
                pi_ = 0;
            }
        }
    } // namespace Internal
} // namespace AZStd

#endif  // #ifndef AZSTD_SMART_PTR_SHARED_COUNT_H
#pragma once
