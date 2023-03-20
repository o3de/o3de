/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/config.h>
#include <AzCore/std/allocator.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/chrono/chrono.h>

#define AFFINITY_MASK_ALL          AZ_TRAIT_THREAD_AFFINITY_MASK_ALLTHREADS
#define AFFINITY_MASK_MAINTHREAD   AZ_TRAIT_THREAD_AFFINITY_MASK_MAINTHREAD
#define AFFINITY_MASK_RENDERTHREAD AZ_TRAIT_THREAD_AFFINITY_MASK_RENDERTHREAD
#define AFFINITY_MASK_USERTHREADS  AZ_TRAIT_THREAD_AFFINITY_MASK_WORKERTHREADS

namespace AZStd
{
    namespace Internal
    {
        template<typename T>
        struct thread_move_t
        {
            T& m_t;
            explicit thread_move_t(T& t)
                : m_t(t)   {}

            T& operator*() const  { return m_t; }
            T* operator->() const { return &m_t; }
        private:
            void operator=(thread_move_t&);
        };

        //      Once we start adding move semantics
        //      template<typename T>
        //      typename Utils::enable_if<AZStd::is_convertible<T&,Internal::thread_move_t<T> >, T >::type move(T& t)
        //      {
        //          return T(Internal::thread_move_t<T>(t));
        //      }
        //      template<typename T>
        //      Internal::thread_move_t<T> move(Internal::thread_move_t<T> t)
        //      {
        //          return t;
        //      }
    }
    // Extension

    struct thread_desc
    {
        //! Debug thread name. Limited to 16 characters on Linux.
        const char*     m_name{ "AZStd::thread" };

        //! Thread stack size. Default is -1, which means we will use the default stack size for each platform.
        int             m_stackSize{ -1 };

        //! Windows: One of the following values:
        //!     THREAD_PRIORITY_IDLE
        //!     THREAD_PRIORITY_LOWEST
        //!     THREAD_PRIORITY_BELOW_NORMAL
        //!     THREAD_PRIORITY_NORMAL  (This is the default)
        //!     THREAD_PRIORITY_ABOVE_NORMAL
        //!     THREAD_PRIORITY_TIME_CRITICAL
        //!
        //! UnixLike platforms inherit calling thread priority by default,
        //!     see platform specific implementations for more details
        int             m_priority{ -100000 };

        //! The CPU ids (as a bitfield) that this thread will be running on, see \ref AZStd::thread_desc::m_cpuId.
        //! Each bit maps directly to the core numbers [0-n], default is 0
        int             m_cpuId{ AFFINITY_MASK_ALL };

        //! If we can join the thread.
        bool            m_isJoinable{ true };
    };


    // 30.3.1
    class thread
    {
    public:
        // types:
        typedef thread_id id;
        typedef native_thread_handle_type native_handle_type;

        // construct/copy/destroy:
        thread();
        ~thread();

        thread(thread&& rhs)
            : m_thread(rhs.m_thread)
        {
            rhs.m_thread = AZStd::thread().m_thread; // set default value
        }
        thread& operator=(thread&& rhs)
        {
            AZ_Assert(joinable() == false, "You must call detach or join before you delete/move over the current thread!");
            m_thread = rhs.m_thread;
            rhs.m_thread = AZStd::thread().m_thread; // set default value
            return *this;
        }

        template<class F, class... Args, typename = AZStd::enable_if_t<!AZStd::is_convertible_v<AZStd::decay_t<F>, thread_desc>>>
        explicit thread(F&& f, Args&&... args);

        /**
         * \note thread_desc is AZStd extension.
         */
        template<class F, class... Args>
        thread(const thread_desc& desc, F&& f, Args&&... args);

        // Till we fully have RVALUES
        template <class F>
        explicit thread(Internal::thread_move_t<F> f);
        thread(Internal::thread_move_t<thread> rhs);
        inline operator Internal::thread_move_t<thread>()   {
            return Internal::thread_move_t<thread>(*this);
        }
        inline thread& operator=(Internal::thread_move_t<thread> rhs)
        {
            thread new_thread(rhs);
            swap(new_thread);
            if (new_thread.joinable())
            {
                new_thread.detach(); // we should/could terminate the thread, this is tricky on some platforms so we leave it to the user.
            }
            return *this;
        }
        inline void swap(thread& rhs) { AZStd::swap(m_thread, rhs.m_thread); }

        bool joinable() const;
        void join();
        void detach();
        id get_id() const;
        native_handle_type native_handle();
        static unsigned hardware_concurrency();

        // Extensions
        //thread(AZStd::delegate<void ()> d,const thread_desc* desc = 0);

    private:
        thread(const thread&) = delete;
        thread& operator=(const thread&) = delete;

        native_thread_data_type     m_thread;
    };

    // 30.3
    class thread;
    inline void swap(thread& x, thread& y)      { x.swap(y); }
    namespace this_thread {
        thread::id get_id();
        void yield();
        ///extension, spins for the specified number of loops, yielding correctly on hyper threaded processors
        void pause(int numLoops);
        //template <class Clock, class Duration>
        //void sleep_until(const chrono::time_point<Clock, Duration>& abs_time);
        template <class Rep, class Period>
        void sleep_for(const chrono::duration<Rep, Period>& rel_time);
    }

    namespace Internal
    {
        /**
        * Thread info interface.
        */
        class thread_info
        {
        public:
            virtual ~thread_info() {}
            virtual void execute() = 0;
            thread_info()
                : m_name(nullptr) {}
            const char* m_name;
        };

        /**
        * Wrapper for the thread execute function.
        */
        template<typename F>
        class thread_info_impl
            : public thread_info
        {
        public:
            thread_info_impl(const F& f)
                : m_f(f) {}
            thread_info_impl(F&& f)
                : m_f(AZStd::move(f)) {}
            thread_info_impl(Internal::thread_move_t<F> f)
                : m_f(f) {}
            void execute() override { m_f(); }
        private:
            F m_f;

            void operator=(thread_info_impl&);
            thread_info_impl(thread_info_impl&);
        };

        template<typename F>
        class thread_info_impl<AZStd::reference_wrapper<F> >
            : public thread_info
        {
        public:
            thread_info_impl(AZStd::reference_wrapper<F> f)
                :  m_f(f)  {}
            virtual void execute() { m_f();  }
        private:
            void operator=(thread_info_impl&);
            thread_info_impl(thread_info_impl&);
            F& m_f;
        };

        template<typename F>
        class thread_info_impl<const AZStd::reference_wrapper<F> >
            : public thread_info
        {
        public:
            thread_info_impl(const AZStd::reference_wrapper<F> f)
                : m_f(f)  {}
            virtual void execute() { m_f(); }
        private:
            F& m_f;
            void operator=(thread_info_impl&);
            thread_info_impl(thread_info_impl&);
        };

        template<typename F>
        static AZ_INLINE thread_info* create_thread_info(F&& f)
        {
            using FunctorType = AZStd::decay_t<F>;
            AZStd::allocator a;
            return new (a.allocate(sizeof(thread_info_impl<FunctorType>), AZStd::alignment_of< thread_info_impl<FunctorType> >::value))thread_info_impl<FunctorType>(AZStd::forward<F>(f));
        }

        template<typename F>
        static AZ_INLINE thread_info* create_thread_info(thread_move_t<F> f)
        {
            AZStd::allocator a;
            return new (a.allocate(sizeof(thread_info_impl<F>), AZStd::alignment_of< thread_info_impl<F> >::value))thread_info_impl<F>(f);
        }

        static AZ_INLINE void destroy_thread_info(thread_info*& ti)
        {
            if (ti)
            {
                ti->~thread_info();
                AZStd::allocator a;
                a.deallocate(ti, 0, 0);
                ti = 0;
            }
        }
    }

    template <>
    struct hash<thread_id>
    {
        size_t operator()(const thread_id& value) const
        {
            static_assert(sizeof(thread_id) <= sizeof(size_t), "thread_id should less than size_t");
            size_t hash{};
            *reinterpret_cast<thread_id*>(&hash) = value;
            return hash;
        }
    };

}

#include <AzCore/std/parallel/internal/thread_Platform.h>

