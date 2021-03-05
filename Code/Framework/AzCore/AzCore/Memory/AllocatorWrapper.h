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

#include <AzCore/base.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    /**
    * Safe wrapper for an instance of an allocator.
    *
    * Generally it is preferred to use AllocatorInstance<> for allocator singletons over this wrapper,
    * but you may want to use this wrapper in particular when scoping a custom allocator to the lifetime 
    * of some other object.
    */
    template<class Allocator>
    class AllocatorWrapper
    {
    public:
        using Descriptor = typename Allocator::Descriptor;

        AllocatorWrapper()
        {
        }

        ~AllocatorWrapper()
        {
            Destroy();
        }

        AllocatorWrapper(const Allocator&) = delete;

        /// Creates the wrapped allocator. You may pass any custom arguments to the allocator's constructor.
        template<typename... Args>
        void Create(const Descriptor& desc, Args&&... args)
        {
            Destroy();

            m_allocator = new (&m_storage) Allocator(AZStd::forward<Args>(args)...);
            m_allocator->Create(desc);
            m_allocator->PostCreate();
        }

        /// Destroys the wrapped allocator.
        void Destroy()
        {
            if (m_allocator)
            {
                m_allocator->PreDestroy();
                m_allocator->Destroy();
                m_allocator->~Allocator();
            }

            m_allocator = nullptr;
        }

        /// Provides access to the wrapped allocator.
        Allocator* Get() const
        {
            return m_allocator;
        }

        /// Provides access to the wrapped allocator.
        Allocator& operator*() const
        {
            return *m_allocator;
        }

        /// Provides access to the wrapped allocator.
        Allocator* operator->() const
        {
            return m_allocator;
        }

        /// Support for conversion to a boolean, returns true if the allocator has been created.
        operator bool() const
        {
            return m_allocator != nullptr;
        }

    private:
        Allocator* m_allocator = nullptr;
        typename AZStd::aligned_storage<sizeof(Allocator), AZStd::alignment_of<Allocator>::value>::type m_storage;
    };
}
