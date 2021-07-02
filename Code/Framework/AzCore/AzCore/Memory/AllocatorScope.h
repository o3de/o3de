/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    // Class which ensures that a set of allocators are initialized, records which ones it was
    // responsible for, and tears them down in reverse order at Deactivate time
    template <class... Allocators>
    class AllocatorScope
    {
    public:
        void ActivateAllocators()
        {
            // Note the parameter pack expansion, this creates the equivalent of a fold expression
            // For each type, call InitAllocator<T>(), then put 0 in the initializer list
            std::initializer_list<int> init{(InitAllocator<Allocators>(), 0)...};
        }

        void DeactivateAllocators()
        {
            // tear down in reverse order of activations
            for (auto allocatorIt = m_teardownCallbacks.rbegin(); allocatorIt != m_teardownCallbacks.rend(); ++allocatorIt)
            {
                (*allocatorIt)();
            }
        }

    private:
        template <class Allocator>
        void InitAllocator()
        {
            if (!AZ::AllocatorInstance<Allocator>::IsReady())
            {
                AZ::AllocatorInstance<Allocator>::Create();
                // Record a typed call to Teardown if we are responsible for this allocator type
                m_teardownCallbacks.push_back(&AllocatorScope::TeardownAllocator<Allocator>);
            }
        }

        template <class Allocator>
        static void TeardownAllocator()
        {
            AZ::AllocatorInstance<Allocator>::Destroy();
        }
        
        // NOTE: this is intentionally not a function<void()>, as function can
        // allocate on the heap, which may not be safe to do
        AZStd::fixed_vector<void(*)(), sizeof...(Allocators)> m_teardownCallbacks;
    };
}
