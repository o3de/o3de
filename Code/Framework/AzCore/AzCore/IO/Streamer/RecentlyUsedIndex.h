/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/is_integral.h>

namespace AZ::IO
{
    //! Uses an optimized double linked list to keep track of the most and least recently used index.
    template<typename T>
    class RecentlyUsedIndex
    {
        static_assert(
            AZStd::is_integral_v<T> && AZStd::is_unsigned_v<T>,
            "Recently Used Index only supports unsigned integers as they need to represent an index.");

    public:
        static constexpr T InvalidIndex = AZStd::numeric_limits<T>::max();

        RecentlyUsedIndex() = default;
        explicit RecentlyUsedIndex(T size);

        //! Pushes the index to end of the list, making it the most recently used.
        void Touch(T index);
        //! Pushes the index to the start of the list, making it the least recently used.
        void Flush(T index);

        //! Flush all indices, returning the cache to it's initial state.
        void FlushAll();

        //! Touches the least recently used index. This has better performance than using Touch with
        //! the least recently used index.
        T TouchLeastRecentlyUsed();

        T GetLeastRecentlyUsed() const;
        T GetMostRecentlyUsed() const;

        void GetIndicesInOrder(const AZStd::function<void(T)>& callback) const;

    private:
        template<bool CheckFront, bool CheckBack>
        void Remove(T index);

        AZStd::unique_ptr<T[]> m_indicesPrevious;
        AZStd::unique_ptr<T[]> m_indicesNext;
        T m_size{ 0 };
        T m_front{ InvalidIndex };
        T m_back{ InvalidIndex };
    };
} // namespace AZ::IO

#include <AzCore/IO/Streamer/RecentlyUsedIndex.inl>
