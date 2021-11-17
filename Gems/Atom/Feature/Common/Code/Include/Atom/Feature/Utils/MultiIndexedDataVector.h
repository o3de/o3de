/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AtomCore/std/containers/array_view.h>

namespace AZ
{
    namespace Render
    {
        //! MultiIndexedDataVector is similar to IndexedDataVector but adds support for multiple different data vectors each containing different types
        //! i.e. structure of (N) arrays
        //! See MultiIndexedDataVectorTests.cpp for examples of use
        template<typename ... Ts>
        class MultiIndexedDataVector
        {
        public:

            using IndexType = uint16_t;

            MultiIndexedDataVector()
            {
                m_dataToIndices.reserve(InitialReservedCount);
                m_indices.reserve(InitialReservedCount);

                AZStd::apply(ReserveFunctor(InitialReservedCount), m_data);
            }
            ~MultiIndexedDataVector() = default;

            static constexpr IndexType NoFreeSlot = std::numeric_limits<IndexType>::max();
            IndexType m_firstFreeSlot = NoFreeSlot;

            void Clear()
            {
                m_dataToIndices.clear();
                m_indices.clear();

                AZStd::apply(static_cast<Fn>(ClearFunction), m_data);
                m_firstFreeSlot = NoFreeSlot;
            }

            IndexType GetFreeSlotIndex()
            {
                IndexType freeSlotIndex = static_cast<IndexType>(m_indices.size());

                if (freeSlotIndex == NoFreeSlot)
                {
                    // the vector is full
                    return NoFreeSlot;
                }

                if (m_firstFreeSlot == NoFreeSlot)
                {
                    // If there's no free slot, add on to the end.
                    m_indices.push_back(freeSlotIndex);
                }
                else
                {
                    // Fill the free slot. m_indices uses it's empty slots to store a linked list (via indices) to other empty slots.
                    freeSlotIndex = m_firstFreeSlot;
                    m_firstFreeSlot = m_indices.at(m_firstFreeSlot);
                    m_indices.at(freeSlotIndex) = static_cast<IndexType>(AZStd::get<0>(m_data).size());
                }

                // The data itself is always packed and m_indices points at it, so push a new entry to the back.
                AZStd::apply(static_cast<Fn>(PushBackFunction), m_data);

                m_dataToIndices.push_back(freeSlotIndex);

                return freeSlotIndex;
            }

            template <class... Args>
            static void PushBackFunction(Args&... args)
            {
                (args.emplace_back(), ...);
            }

            template <class... Args>
            static void PopBackFunction(Args&... args)
            {
                (args.pop_back(), ...);
            }

            template <class... Args>
            static void ClearFunction(Args&... args)
            {
                (args.clear(), ...);
            }

            template<typename IndexType>
            struct CopyBackElementFunctor
            {
                CopyBackElementFunctor(IndexType dataIndex)
                    : m_dataIndex(dataIndex)
                {
                }

                void operator()() const
                {
                }

                template <class FirstArg, class ... OtherArgs>
                void operator()(FirstArg& firstArg, OtherArgs&... otherArgs) const
                {
                    firstArg[m_dataIndex] = firstArg.back();
                    this->operator()(otherArgs ...);
                }

                IndexType m_dataIndex;
            };

            struct ReserveFunctor
            {
                ReserveFunctor(size_t reserveAmount)
                    : m_reserveAmount(reserveAmount)
                {
                }

                void operator()() const
                {
                }

                template <class FirstArg, class ... OtherArgs>
                void operator()(FirstArg& firstArg, OtherArgs&... otherArgs) const
                {
                    firstArg.reserve(m_reserveAmount);
                    this->operator()(otherArgs ...);
                }

                size_t m_reserveAmount;
            };

            // Removes data at the provided index
            // If data is moved into the vacated slot, returns the new index corresponding to that data
            // Else returns NoFreeSlot
            IndexType RemoveIndex(IndexType index)
            {
                IndexType dataIndex = m_indices.at(index);

                // Copy the back element on top of this one.
                AZStd::apply(CopyBackElementFunctor<IndexType>(dataIndex), m_data);

                m_dataToIndices.at(dataIndex) = m_dataToIndices.back();

                // Update the index of the moved light
                m_indices.at(m_dataToIndices.at(dataIndex)) = dataIndex;

                // Pop the back
                AZStd::apply(static_cast<Fn>(PopBackFunction), m_data);

                m_dataToIndices.pop_back();

                // Use free slot to link to next free slot
                m_indices.at(index) = m_firstFreeSlot;
                m_firstFreeSlot = index;

                if (dataIndex < m_dataToIndices.size())
                {
                    return m_dataToIndices.at(dataIndex);
                }
                else
                {
                    return NoFreeSlot;
                }
            }

            template<size_t ArrayIndex>
            const auto& GetData(size_t elemIndex)const
            {
                return AZStd::get<ArrayIndex>(m_data).at(m_indices[elemIndex]);
            }

            template<size_t ArrayIndex>
            auto& GetData(size_t elemIndex)
            {
                return AZStd::get<ArrayIndex>(m_data).at(m_indices[elemIndex]);
            }

            size_t GetDataCount() const
            {
                return AZStd::get<0>(m_data).size();
            }

            template<size_t Index>
            const auto& GetDataVector() const
            {
                return AZStd::get<Index>(m_data);
            }

            IndexType GetRawIndex(IndexType index) const
            {
                return m_indices.at(index);
            }
            
            template<size_t Index, typename DataType>
            IndexType GetIndexForData(const DataType* data) const
            {
                if (data >= &AZStd::get<Index>(m_data).front() && data <= &AZStd::get<Index>(m_data).back())
                {
                    return m_dataToIndices.at(data - &AZStd::get<Index>(m_data).front());
                }
                return NoFreeSlot;
            }

            template<size_t Index, typename LambdaType>
            void ForEach(LambdaType lambda) const
            {
                for (auto& item : AZStd::get<Index>(m_data))
                {
                    if (!lambda(item))
                    {
                        break;
                    }
                }
            }

        private:
            using Fn = void(&)(AZStd::vector<Ts>& ...);

            static constexpr size_t InitialReservedCount = 128;

            // stores the index of data vector for respective elements, it also include a linked list to flag the free slots
            AZStd::vector<IndexType> m_indices;
            // stores the index of index vector for respective element
            AZStd::vector<IndexType> m_dataToIndices;

            // Actual data fields. If we defined this as MultiIndexedDataVector<int, string, bool>
            // then m_data will contain
            AZStd::tuple<AZStd::vector<Ts>...> m_data;
        };
    }
}
