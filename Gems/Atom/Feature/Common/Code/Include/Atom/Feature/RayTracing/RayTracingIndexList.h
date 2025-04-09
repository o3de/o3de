/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    namespace Render
    {
        static const uint32_t InvalidIndex = aznumeric_cast<uint32_t>(-1);

        //! Manages an index list used by RayTracing, with an internal freelist chain.
        //! 
        //! Indices are stored in a flat array, and new indices are either added to the end
        //! or in the first available free index.
        //! 
        //! The freelist chain is stored inside array itself, with each entry in the chain pointing
        //! to the next free index, and terminated with InvalidIndex.  Free list entries have
        //! FreeListThreshold added to their value to indicate they are part of the freelist.
        template<uint32_t BlockSize>
        class RayTracingIndexList
        {
        public:

            RayTracingIndexList() = default;
            ~RayTracingIndexList() = default;

            // add a BlockSize set of entries to the index list
            uint32_t AddEntry(AZStd::array<uint32_t, BlockSize> entry);

            // add an entry, non-array version for use when BlockSize == 1
            uint32_t AddEntry(uint32_t entry);

            // set a BlockSize set of entries at the specified index
            void SetEntry(uint32_t index, AZStd::array<uint32_t, BlockSize> entry);

            // set an existing entry, non-array version for use when BlockSize == 1
            void SetEntry(uint32_t index, uint32_t entry);

            // remove BlockSize entries starting at the specified index
            void RemoveEntry(uint32_t index);

            // returns the index list
            using IndexVector = AZStd::vector<uint32_t>;
            const IndexVector& GetIndexList() const { return m_indices; }

            // returns true if the index is valid
            bool IsValidIndex(uint32_t index) const;

            // clears the index list and all associated state
            void Reset();

        private:           

            // freelist index entries are at or above this value
            static const uint32_t FreeListThreshold = 1000000000;

            // helper functions to encode/decode freelist chain indices
            uint32_t EncodeFreeListIndex(uint32_t index) const { return (index == InvalidIndex) ? InvalidIndex : (index + FreeListThreshold); }
            uint32_t DecodeFreeListIndex(uint32_t index) const { return (index == InvalidIndex) ? InvalidIndex : (index - FreeListThreshold); }

            // list of indices
            IndexVector m_indices;

            // starting index of the freelist chain
            uint32_t m_freeStartIndex = InvalidIndex;
        };

        template<uint32_t BlockSize>
        uint32_t RayTracingIndexList<BlockSize>::AddEntry(AZStd::array<uint32_t, BlockSize> entry)
        {
            uint32_t index = 0;

            if (m_freeStartIndex == InvalidIndex)
            {
                // no free entries, insert at the end of the index list
                index = aznumeric_cast<uint32_t>(m_indices.size());
                m_indices.insert(m_indices.end(), entry.begin(), entry.end());
            }
            else
            {
                // get the next free index from the list
                uint32_t nextFreeIndex = m_indices[m_freeStartIndex];

                // overwrite the indices list with the new entries at the free index
                index = m_freeStartIndex;
                AZStd::copy(entry.begin(), entry.end(), m_indices.begin() + index);

                // move the start of the free index chain to nextFreeIndex
                m_freeStartIndex = DecodeFreeListIndex(nextFreeIndex);
            }

            return index;
        }

        template<uint32_t BlockSize>
        uint32_t RayTracingIndexList<BlockSize>::AddEntry(uint32_t entry)
        {
            return AddEntry(AZStd::array<uint32_t, 1>{entry});
        }

        template<uint32_t BlockSize>
        void RayTracingIndexList<BlockSize>::SetEntry(uint32_t index, AZStd::array<uint32_t, BlockSize> entry)
        {
            AZ_Assert(index < m_indices.size(), "Index passed to SetEntry exceeds list size");
            AZ_Assert(index % BlockSize == 0, "Index passed must be a multiple of the BlockSize");

            AZStd::copy(entry.begin(), entry.end(), m_indices.begin() + index);
        }

        template<uint32_t BlockSize>
        void RayTracingIndexList<BlockSize>::SetEntry(uint32_t index, uint32_t entry)
        {
            SetEntry(index, AZStd::array<uint32_t, 1>{entry});
        }

        template<uint32_t BlockSize>
        void RayTracingIndexList<BlockSize>::RemoveEntry(uint32_t index)
        {
            if (m_freeStartIndex == InvalidIndex)
            {
                // no free entries, just set the start index to this entry
                m_freeStartIndex = index;
            }
            else
            {
                // walk the free index chain until we reach the end (marked by InvalidIndex)
                uint32_t currentIndex = m_freeStartIndex;
                uint32_t nextIndex = DecodeFreeListIndex(m_indices[currentIndex]);
                while (nextIndex != InvalidIndex)
                {
                    currentIndex = nextIndex;
                    nextIndex = DecodeFreeListIndex(m_indices[nextIndex]);
                }

                // store the index 
                m_indices[currentIndex] = EncodeFreeListIndex(index);
            }

            // terminate the free index chain by setting the last entry to InvalidIndex
            m_indices[index] = InvalidIndex;
        }

        template<uint32_t BlockSize>
        bool RayTracingIndexList<BlockSize>::IsValidIndex(uint32_t index) const
        {
            return (index < FreeListThreshold);
        }

        template<uint32_t BlockSize>
        void RayTracingIndexList<BlockSize>::Reset()
        {
            m_indices.clear();
            m_freeStartIndex = InvalidIndex;
        }
    }
}
