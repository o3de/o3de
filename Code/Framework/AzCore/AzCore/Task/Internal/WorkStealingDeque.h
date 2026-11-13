/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// The `WorkStealingDeque` class is copied from Google Filament licensed under
// the Apache License 2.0.
// Original code:
// https://github.com/google/filament/blob/main/libs/utils/include/utils/WorkStealingDequeue.h
// Modifications:
// - Make the work-stealing deque variable-sized.
//
// The `Array` class is copied from Taskflow licensed under the MIT License.
// Original code:
// https://github.com/taskflow/taskflow/blob/master/taskflow/core/tsq.hpp

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Internal
{
    class Task;

    class WorkStealingDeque
    {
        template<typename T>
        //requires std::is_pointer_v<T>
        class Array
        {
        public:
            explicit Array(const int capacity)
                : m_capacity{ capacity }
                , m_mask{ capacity - 1 }
                , m_buffer{ new std::atomic<T>[capacity] }
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

    public:
        explicit WorkStealingDeque(const int capacity = 128)
            : m_top{ 0 }
            , m_bottom{ 0 }
            , m_array{ new Array<Task*>{ capacity } }
        {
            AZ_Assert(capacity && (capacity & capacity - 1) == 0, "Capacity must be a power of 2");
            m_garbage.reserve(64);
        }

        WorkStealingDeque(const WorkStealingDeque&) = delete;
        WorkStealingDeque(WorkStealingDeque&&) = delete;
        WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;
        WorkStealingDeque& operator=(WorkStealingDeque&&) = delete;

        ~WorkStealingDeque() noexcept
        {
            for (auto* array : m_garbage)
            {
                delete array;
            }
            delete m_array.load();
        }

        void Enqueue(Task* item)
        {
            // std::memory_order_relaxed is sufficient because this load doesn't acquire
            // anything from another thread. m_bottom is only written in pop() which
            // cannot be concurrent with push().
            const auto bottom = m_bottom.load(AZStd::memory_order_relaxed);
            const auto top = m_top.load(AZStd::memory_order_acquire);
            auto* array = m_array.load(AZStd::memory_order_relaxed);
            if (array->Capacity() - 1 < bottom - top)
            {
                array = Resize(array, bottom, top);
            }
            array->Put(bottom, item);
            // std::memory_order_release is used because we release the item we just
            // pushed to other threads which are calling steal().
            m_bottom.store(bottom + 1, AZStd::memory_order_release);
        }

        [[nodiscard]] Task* TryDequeue()
        {
            // std::memory_order_seq_cst is needed to guarantee ordering in steal() Note
            // however that this is not a typical acquire/release operation:
            // - Not acquire because m_bottom is only written in push() which is not
            // concurrent.
            // - Not release because we're not publishing anything to steal() here.
            // QUESTION: does this prevent m_top load below to be reordered before the
            // "store" part of fetch_sub()? Hopefully it does. If not we'd need a full
            // memory barrier.
            const auto bottom = m_bottom.fetch_sub(1, AZStd::memory_order_seq_cst) - 1;
            // bottom could be -1 if we tried to pop() from an empty queue. This will be
            // corrected below.
            AZ_Assert(bottom >= -1, "Pop from an empty queue");
            auto* array = m_array.load(AZStd::memory_order_relaxed);
            // std::memory_order_seq_cst is needed to guarantee ordering in steal().
            // Note however that this is not a typical acquire operation (i.e. other
            // thread's writes of m_top don't publish data).
            auto top = m_top.load(AZStd::memory_order_seq_cst);
            if (top < bottom)
            {
                // The queue isn't empty, and it's not the last item, just return it, this
                // is the common case.
                return array->Get(bottom);
            }
            Task* item{ nullptr };
            if (top == bottom)
            {
                // We just took the last item.
                item = array->Get(bottom);
                // Because we know we took the last item, we could be racing with steal()
                // -- the last item being both at the top and bottom of the queue. We
                // resolve this potential race by also stealing that item from ourselves.
                if (m_top.compare_exchange_strong(top, top + 1, AZStd::memory_order_seq_cst, AZStd::memory_order_relaxed))
                {
                    // Success: we stole our last item from ourselves, meaning that a
                    // concurrent steal() would have failed. m_top now equals top + 1, we
                    // adjust top to make the queue empty.
                    ++top;
                }
                else
                {
                    // Failure: m_top was not equal to top, which means the item was stolen
                    // under our feet. top now equals to m_top. Simply discard the item we
                    // just popped. The queue is now empty.
                    item = nullptr;
                }
            }
            else
            {
                // We could be here if the item was stolen just before we read m_top, we'll
                // adjust m_bottom below.
                AZ_Assert(top - bottom == 1, "Item was stolen just before we read m_top");
            }
            // AZStd::memory_order_relaxed used because we're not publishing any data. No
            // concurrent writes to m_bottom possible, it's always safe to write m_bottom.
            m_bottom.store(top, AZStd::memory_order_relaxed);
            return item;
        }

        [[nodiscard]] Task* TrySteal()
        {
            // Note: A key component of this algorithm is that m_top is read before
            // m_bottom here (and observed as such in other threads).

            // std::memory_order_seq_cst is needed to guarantee ordering in pop(). Note
            // however that this is not a typical acquire operation (i.e. other thread's
            // writes of m_top don't publish data).
            auto top = m_top.load(AZStd::memory_order_seq_cst);

            // std::memory_order_acquire is needed because we're acquiring items
            // published in push(). std::memory_order_seq_cst is needed to guarantee
            // ordering in pop().
            if (const auto bottom = m_bottom.load(AZStd::memory_order_seq_cst); top >= bottom)
            {
                // The queue is empty.
                return nullptr;
            }

            // The queue isn't empty.
            auto* array = m_array.load(AZStd::memory_order_acquire);
            const auto item = array->Get(top);
            if (!m_top.compare_exchange_strong(top, top + 1, AZStd::memory_order_seq_cst, AZStd::memory_order_relaxed))
            {
                // Failure: the item we just tried to steal was pop()'ed under our feet,
                // simply discard it; nothing to do -- it's okay to try again.
                return nullptr;
            }
            // Success: we stole an item, just return it.
            return item;
        }

    private:
        [[nodiscard]] Array<Task*>* Resize(Array<Task*>* array, const size_t bottom, const size_t top)
        {
            auto* tmp = array->Resize(bottom, top);
            m_garbage.push_back(array);
            AZStd::swap(array, tmp);
            m_array.store(array, AZStd::memory_order_release);
            return array;
        }

#ifdef __cpp_lib_hardware_interference_size
        alignas(std::hardware_destructive_interference_size)
#endif
        AZStd::atomic<int> m_top, m_bottom;

        AZStd::atomic<Array<Task*>*> m_array;
        AZStd::vector<Array<Task*>*> m_garbage;
    };
} // namespace AZ::Internal

