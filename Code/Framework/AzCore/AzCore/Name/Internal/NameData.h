/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace AZ
{
    class Name;
    class NameDictionary;

    namespace Internal
    {
        class NameData final
        {
            friend NameDictionary;
            friend Name;
        public:
            AZ_CLASS_ALLOCATOR(NameData, AZ::SystemAllocator);

            using Hash = uint32_t; // We use a 32 bit hash especially for efficient transport over a network.

            //! Returns the string part of the name data.
            AZStd::string_view GetName() const;

            //! Returns the hash part of the name data.
            Hash GetHash() const;

        private:
            NameData(AZStd::string&& name, Hash hash);

            void add_ref();
            void release();

            template <typename T>
            friend struct AZStd::IntrusivePtrCountPolicy;

            AZStd::atomic_int m_useCount{ 0 };
            AZStd::string m_name;
            Hash m_hash;

            // TODO: We should be able to change this to a normal bool after introducing name dictionary garbage collection
            AZStd::atomic<bool> m_hashCollision{ false }; // Tracks whether the hash has been involved in a collision
            //! Stores a pointer to the name dictionary that created the NameData
            //! if the the name dictionary is destroyed, set back to nullptr
            NameDictionary* m_nameDictionary{};
        };
    }
}
