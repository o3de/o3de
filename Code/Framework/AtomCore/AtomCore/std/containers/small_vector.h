/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>

namespace AZStd
{
    // Class that servers a similar purpose as boosts small_vector
    //  https://www.boost.org/doc/libs/1_86_0/doc/html/container/non_standard_containers.html#container.non_standard_containers.small_vector
    // If the there are less than `FixedSize` elements in the vector, the data is stored in a fixed_vector
    // If there are more, an AZstd::vector allocates memory on the heap
    //
    // This is a pretty simple implementation that does not expose all vector functions.
    //  E.g. iterators are only accessible through the span() function
    template<class T, size_t FixedSize>
    class small_vector
    {
    public:
        small_vector() = default;
        small_vector(const small_vector<T, FixedSize>&) = default;
        small_vector(small_vector<T, FixedSize>&&) = default;
        small_vector& operator=(const small_vector<T, FixedSize>&) = default;
        small_vector& operator=(small_vector<T, FixedSize>&&) = default;

        small_vector(size_t newSize, const T& value)
        {
            resize(newSize, value);
        }

        void push_back(const T& value)
        {
            if (AZStd::holds_alternative<FixedVectorT>(m_data))
            {
                if (span().size() >= FixedSize)
                {
                    ConvertToHeapVector();
                    AZStd::get<HeapVectorT>(m_data).push_back(value);
                }
                else
                {
                    AZStd::get<FixedVectorT>(m_data).push_back(value);
                }
            }
            else if (AZStd::holds_alternative<HeapVectorT>(m_data))
            {
                AZStd::get<HeapVectorT>(m_data).push_back(value);
            }
            else
            {
                AZ_Assert(false, "small_vector::push_back: Empty variant");
            }
        }

        template<typename... Args, typename = AZStd::enable_if_t<AZStd::is_constructible_v<T, Args...>>>
        T& emplace_back(Args&&... args) noexcept
        {
            if (AZStd::holds_alternative<FixedVectorT>(m_data))
            {
                if (span().size() >= FixedSize)
                {
                    ConvertToHeapVector();
                    return AZStd::get<HeapVectorT>(m_data).emplace_back(AZStd::forward<Args>(args)...);
                }
                else
                {
                    return AZStd::get<FixedVectorT>(m_data).emplace_back(AZStd::forward<Args>(args)...);
                }
            }
            else if (AZStd::holds_alternative<HeapVectorT>(m_data))
            {
                return AZStd::get<HeapVectorT>(m_data).emplace_back(AZStd::forward<Args>(args)...);
            }
            else
            {
                AZ_Assert(false, "small_vector::emplace_back: Empty variant");
                return span().front();
            }
        }

        void erase(size_t position)
        {
            if (AZStd::holds_alternative<FixedVectorT>(m_data))
            {
                auto& fixed = AZStd::get<FixedVectorT>(m_data);
                fixed.erase(fixed.begin() + position);
            }
            else if (AZStd::holds_alternative<HeapVectorT>(m_data))
            {
                auto& heap = AZStd::get<HeapVectorT>(m_data);
                heap.erase(heap.begin() + position);
            }
            else
            {
                AZ_Assert(false, "small_vector::erase: Empty variant");
            }
        }

        void resize(size_t newSize, const T& value)
        {
            if (AZStd::holds_alternative<FixedVectorT>(m_data))
            {
                if (newSize > FixedSize)
                {
                    ConvertToHeapVector();
                    AZStd::get<HeapVectorT>(m_data).resize(newSize, value);
                }
                else
                {
                    AZStd::get<FixedVectorT>(m_data).resize(newSize, value);
                }
            }
            else if (AZStd::holds_alternative<HeapVectorT>(m_data))
            {
                AZStd::get<HeapVectorT>(m_data).resize(newSize, value);
            }
            else
            {
                AZ_Assert(false, "small_vector::resize: Empty variant");
            }
        }

        void resize(size_t newSize)
        {
            resize(newSize, {});
        }

        void reserve(size_t newCapacity)
        {
            if (AZStd::holds_alternative<FixedVectorT>(m_data))
            {
                if (newCapacity > FixedSize)
                {
                    ConvertToHeapVector();
                    AZStd::get<HeapVectorT>(m_data).reserve(newCapacity);
                }
                else
                {
                    AZStd::get<FixedVectorT>(m_data).reserve(newCapacity);
                }
            }
            else if (AZStd::holds_alternative<HeapVectorT>(m_data))
            {
                AZStd::get<HeapVectorT>(m_data).reserve(newCapacity);
            }
            else
            {
                AZ_Assert(false, "small_vector::reserve: Empty variant");
            }
        }

        size_t size()
        {
            return AZStd::visit(
                [](auto& vector)
                {
                    return vector.size();
                },
                m_data);
        }

        AZStd::span<T> span()
        {
            return AZStd::visit(
                [](auto& vector) -> AZStd::span<T>
                {
                    return { vector };
                },
                m_data);
        }

        AZStd::span<const T> span() const
        {
            return AZStd::visit(
                [](auto& vector) -> AZStd::span<const T>
                {
                    return { vector };
                },
                m_data);
        }

        T& operator[](size_t pos)
        {
            return AZStd::visit(
                [&](auto& vector) -> T&
                {
                    return vector[pos];
                },
                m_data);
        }

        const T& operator[](size_t pos) const
        {
            return AZStd::visit(
                [&](auto& vector) -> const T&
                {
                    return vector[pos];
                },
                m_data);
        }

        void clear()
        {
            AZStd::visit(
                [&](auto& vector)
                {
                    vector.clear();
                },
                m_data);
        }

        bool empty() const
        {
            return span().empty();
        }

        size_t size() const
        {
            return span().size();
        }

    private:
        void ConvertToHeapVector()
        {
            if (AZStd::holds_alternative<FixedVectorT>(m_data))
            {
                auto data = span();
                auto newData = HeapVectorT(data.begin(), data.end());
                m_data = AZStd::move(newData);
            }
        }

        using FixedVectorT = AZStd::fixed_vector<T, FixedSize>;
        using HeapVectorT = AZStd::vector<T>;

        AZStd::variant<FixedVectorT, HeapVectorT> m_data;
    };
} // namespace AZStd
