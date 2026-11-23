/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*/

// Original code:
// https://github.com/taskflow/taskflow/blob/master/taskflow/core/wsq.hpp

#pragma once

#include <AzCore/std/parallel/atomic.h>

namespace AZ
{
    namespace Internal
    {
        template<typename T>
        //requires std::is_pointer_v<T>
        class Array
        {
        public:
            explicit Array(const int capacity)
                : m_capacity{ capacity }
                , m_mask{ capacity - 1 }
                , m_buffer{ new AZStd::atomic<T>[capacity] }
            {
            }

            Array(const Array&) = delete;
            Array(Array&&) = delete;
            Array& operator=(const Array&) = delete;
            Array& operator=(Array&&) = delete;
            ~Array() noexcept
            {
                delete[] m_buffer;
            }

            void Put(const size_t index, T item) noexcept
            {
                m_buffer[index & m_mask].store(item, AZStd::memory_order_relaxed);
            }

            [[nodiscard]] T Get(const size_t index) noexcept
            {
                return m_buffer[index & m_mask].load(AZStd::memory_order_relaxed);
            }

            [[nodiscard]] Array* Resize(const size_t bottom, const size_t top)
            {
                auto* array = new Array{ 2 * m_capacity };
                for (auto i = top; i != bottom; ++i)
                {
                    array->Put(i, Get(i));
                }
                return array;
            }

            [[nodiscard]] int Capacity() const
            {
                return m_capacity;
            }

        private:
            const int m_capacity;
            const int m_mask;
            AZStd::atomic<T>* m_buffer;
        };
    }
}
