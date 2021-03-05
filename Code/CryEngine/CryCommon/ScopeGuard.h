/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <type_traits>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SCOPEGUARD_H_SECTION_1 1
#define SCOPEGUARD_H_SECTION_2 2
#endif

/**
This is from the c++17 working draft paper N3949 http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3949.pdf
It is a new library addition targeted for c++17. I didn't feel like waiting. Only modification is as required to get the
code compiling in the yet to be c++11 compliant visual studio. It is similar to boost scope exit, but in a modern and
more feature rich form. scope_guard just executes a lambda when it goes out of scope. unique_resource is a more complete
RAII wrapper. Its stands in for a resource (i.e. overloads cast operator to the wrapped resource) and frees it when it
goes out of scope. see the paper for more examples and a better description. Get rid of this once c++17 is available.
*/
namespace std17
{
    template <typename D>
    struct scope_guard_t
    {
        // construction
        explicit scope_guard_t(D&& f)
            : deleter(std::move(f))
            , execute_on_destruction(true)
        {
        }
        // move
        scope_guard_t(scope_guard_t&& rhs)
            : deleter(std::move(rhs.deleter))
            , execute_on_destruction(rhs.execute_on_destruction)
        {
            rhs.release();
        }
        // release
        ~scope_guard_t()
        {
            if (execute_on_destruction)
            {
                deleter();
            }
        }
        void release() { execute_on_destruction = false; }
    private:
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SCOPEGUARD_H_SECTION_1
    #include AZ_RESTRICTED_FILE(ScopeGuard_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        scope_guard_t(scope_guard_t const&) = delete;
        void operator=(scope_guard_t const&) = delete;
        scope_guard_t& operator=(scope_guard_t&&) = delete;
#endif
        D deleter;
        bool execute_on_destruction;
        // exposition only
    };

    template <typename D>
    scope_guard_t<D> scope_guard(D&& deleter)
    {
        return scope_guard_t<D>(std::move(deleter));
        // fails with curlies
    }

    enum class invoke_it
    {
        once, again
    };
    template<typename R, typename D>
    class unique_resource_t
    {
        R resource;
        D deleter;
        bool execute_on_destruction;
        // exposition only
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SCOPEGUARD_H_SECTION_2
    #include AZ_RESTRICTED_FILE(ScopeGuard_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        unique_resource_t& operator=(unique_resource_t const&) = delete;
        unique_resource_t(unique_resource_t const&) = delete;
#endif
        // no copies!
    public:
        // construction
        explicit unique_resource_t(R&& _resource, D&& _deleter, bool _shouldrun = true)
            : resource(std::move(_resource))
            , deleter(std::move(_deleter))
            , execute_on_destruction(_shouldrun)
        {
        }
        // move
        unique_resource_t(unique_resource_t&& other)
            : resource(std::move(other.resource))
            , deleter(std::move(other.deleter))
            , execute_on_destruction(other.execute_on_destruction)
        {
            other.release();
        }
        unique_resource_t& operator=(unique_resource_t&& other)
        {
            this->invoke(invoke_it::once);
            deleter = std::move(other.deleter);
            resource = std::move(other.resource);
            execute_on_destruction = other.execute_on_destruction;
            other.release();
            return *this;
        }
        // resource release
        ~unique_resource_t()
        {
            this->invoke(invoke_it::once);
        }
        void invoke(invoke_it const strategy = invoke_it::once)
        {
            if (execute_on_destruction)
            {
                get_deleter()(resource);
            }
            execute_on_destruction = strategy == invoke_it::again;
        }
        R const& release()
        {
            execute_on_destruction = false;
            return this->get();
        }
        void reset(R&& newresource)
        {
            invoke(invoke_it::again);
            resource = std::move(newresource);
        }
        // resource access
        R const& get() const
        {
            return resource;
        }
        operator R const& () const
        {
            return resource;
        }
        R operator->() const
        {
            return resource;
        }
        typename std::add_lvalue_reference<typename std::remove_pointer<R>::type>::type operator*() const
        {
            return *resource;
        }
        // deleter access
        const D& get_deleter() const
        {
            return deleter;
        }
    };

    template<typename R, typename D>
    unique_resource_t<R, D> unique_resource(R&& r, D t)
    {
        return unique_resource_t<R, D>(std::move(r), std::move(t), true);
    }

    template<typename R, typename D>
    unique_resource_t<R, D> unique_resource_checked(R r, R invalid, D t)
    {
        bool shouldrun = (r != invalid);
        return unique_resource_t<R, D>(std::move(r), std::move(t), shouldrun);
    }
}

