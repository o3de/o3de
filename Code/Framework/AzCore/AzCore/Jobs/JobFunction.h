/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/function_traits.h>

namespace AZ
{
    /**
     * A job which uses a templated function (can be AZStd::function, AZStd::delegate, result of AZStd::bind, lambda, reference to a functor, regular function).
     * 
     * The function can either take the owning AZ::Job& as its lone parameter or no parameters at all
     */
    template<class Function>
    class JobFunction
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(JobFunction, ThreadPoolAllocator);

        typedef const typename AZStd::remove_cv<typename AZStd::remove_reference<Function>::type>::type& FunctionCRef;

        JobFunction(FunctionCRef processFunction, bool isAutoDelete, JobContext* context)
            : Job(isAutoDelete, context)
            , m_function(processFunction)
        {
        }

        void Process() override
        {
            // Use our template argument helper to invoke m_function with either no args or *this
            constexpr size_t ArgCount = AZStd::function_traits<Function>::arity;
            ArgHelper<Function, ArgCount>::Process(m_function, *this);
        }

    protected:
        JobFunction(const JobFunction&) = delete;
        JobFunction& operator=(const JobFunction&) = delete;

        template <class T>
        using FirstArgType = AZStd::function_traits_get_arg_t<T, 0>;

        // Specialized template helper to match 1 or 0 arg functors
        template<typename FunctionType, size_t ArgCount>
        struct ArgHelper
        {
            static void Process(FunctionType& function, AZ::Job& job)
            {
                static_assert((ArgCount == 1) && AZStd::is_same<FirstArgType<FunctionType>, AZ::Job&>::value,
                    "JobFunction functors must take either the owning AZ::Job& as their lone parameter or no parameters at all"
                );
                function(job);
            }
        };

        template<typename FunctionType>
        struct ArgHelper<FunctionType, 0>
        {
            static void Process(FunctionType& function, AZ::Job&)
            {
                function();
            }
        };

        Function m_function;
    };

    /// Convenience function to create (aznew JobFunction with any function signature). Delete the function with delete (or isAutoDelete set to true)
    template<class Function>
    inline JobFunction<Function>* CreateJobFunction(const Function& processFunction, bool isAutoDelete, JobContext* context = nullptr)
    {
        return aznew JobFunction<Function>(processFunction, isAutoDelete, context);
    }

    /// For delete symmetry
    inline void DeleteJobFunction(Job* jobFunction)
    {
        delete jobFunction;
    }
}
