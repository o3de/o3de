/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/ShadowmapAtlas.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace Render
    {
        void ShadowmapAtlas::Initialize()
        {
            m_requireFinalize = true;
            m_indicesForSize.clear();
            m_locations.clear();
            m_baseShadowmapSize = ShadowmapSize::None;
            m_maxArraySlice = 0;
            m_shadowmapIndexNodeTree.clear();
            m_indexTableData.clear();
        }

        void ShadowmapAtlas::SetShadowmapSize(size_t index, ShadowmapSize size)
        {
            AZ_Assert(m_requireFinalize, "Initialize before set shadowmap size");
            m_requireFinalize = true;
            m_indicesForSize[size].push_back(index);
            m_baseShadowmapSize = AZStd::GetMax(size, m_baseShadowmapSize);
        }

        void ShadowmapAtlas::Finalize()
        {
            AZ_Assert(m_requireFinalize, "Initialize before finalization.");

            // Determine shadowmap location in the atlas from the larger shadowmaps.
            Location currentLocation;
            for (ShadowmapSize size = m_baseShadowmapSize;
                size >= MinShadowmapImageSize;
                size = static_cast<ShadowmapSize>(static_cast<uint32_t>(size) / 2))
            {
                // The length of the location indicates the width of shadowmap.
                // When we make the width half, we extend the sequence by 1.
                currentLocation.resize(currentLocation.size() + 1);
                if (m_indicesForSize.find(size) != m_indicesForSize.end())
                {
                    for (const size_t index : m_indicesForSize[size])
                    {
                        m_locations[index] = currentLocation;
                        SetShadowmapIndexInTree(currentLocation, index);
                        m_maxArraySlice = AZStd::GetMax(m_maxArraySlice, currentLocation[0]);
                        SucceedLocation(currentLocation);
                    }
                }
            }
            m_requireFinalize = false;

            AZ_Assert(m_shadowmapIndexNodeTree.empty() || GetNodeOfTree(Location{}).size() == GetArraySliceCount(),
                "The atlas has a shadowmap, but the root subtable does not have size of the array slice count.");
            BuildIndexTableData();
        }

        uint16_t ShadowmapAtlas::GetArraySliceCount() const
        {
            AZ_Assert(!m_requireFinalize, "Finalization is required.");
            // If no shadowmap is added, it returns 1
            // since an image resource have to be created for such a case.
            return static_cast<uint16_t>(m_maxArraySlice) + 1;
        }

        ShadowmapSize ShadowmapAtlas::GetBaseShadowmapSize() const
        {
            AZ_Assert(!m_requireFinalize, "Finalization is required.");
            return m_baseShadowmapSize;
        }

        ShadowmapAtlas::Origin ShadowmapAtlas::GetOrigin(size_t index) const
        {
            AZ_Assert(!m_requireFinalize, "Finalization is required.");
            Origin origin;

            if (m_locations.find(index) == m_locations.end())
            {
                return origin; // disabled shadowmap for this light
            }
            const Location location = m_locations.at(index);
            if (location.empty())
            {
                return origin; // disabled shadowmap for this light
            }

            origin.m_arraySlice = static_cast<uint16_t>(location[0]);
            uint32_t sizeDiff = static_cast<uint32_t>(m_baseShadowmapSize);
            for (size_t digitIndex = 1; digitIndex < location.size(); ++digitIndex)
            {
                sizeDiff /= 2;
                AZ_Assert(location[digitIndex] <= LocationIndexNum, "Digit in atlas location is illegal.");
                origin.m_originInSlice[0] += (location[digitIndex] & 1) ? sizeDiff : 0;
                origin.m_originInSlice[1] += (location[digitIndex] & 2) ? sizeDiff : 0;
            }
            return origin;
        }

        void ShadowmapAtlas::SucceedLocation(Location& location)
        {
            constexpr uint8_t LocationIndexMax = LocationIndexNum - 1;
            // A location is a finite sequence of non-negative integers.
            // We consider it as a number of base LocationIndexMax (base 4)  except 0-th digit.
            for (size_t digitIndex = location.size() - 1; digitIndex > 0; --digitIndex)
            {
                if (location[digitIndex] < LocationIndexMax)
                {
                    // If the figure of the current digit is < LocationIndexMax(=3),
                    // just succeed the figure and finish.
                    // (e.g., [1,0,_0_] -> [1,0,_1_])
                    ++location[digitIndex];
                    return;
                }
                else
                {
                    // If the figure of the current digit is already max,
                    // carry over and consider the next digit.
                    // (e.g., [1,0,_3_] -> [1,_0_,0] and next execution of the loop it will be [1,_1_,0])
                    location[digitIndex] = 0; // carry over
                }
            }

            // The first index indicates array slice index,
            // so just add a new slice and carry over.
            // This case happens when the all following figures are already LocationIndexMax(=3).
            // (e.g., [1,3,3,3] input -> [2,0,0,0] output)
            ++location[0];
            AZ_Assert(location[0] != 0, "Array slice index is overflowed.");
        }

        void ShadowmapAtlas::SetShadowmapIndexInTree(const Location& location, size_t index)
        {
            const Location parentLocation(location.begin(), location.end() - 1);
            ShadowmapIndicesInNode& parentNode = GetNodeOfTree(parentLocation);
            if (parentLocation.empty())
            {
                // parentLocation indicates the root location.
                // The root subtable's size is not determined at this point,
                // so just reserve the required size.
                AZ_Assert(parentNode.size() == location.back(), "Root node should grow its size gradually.");
                parentNode.push_back(index);
            }
            else
            {
                // parentLocation is not root.  Then its size does not change.
                parentNode[location.back()] = index;
            }
        } 

        ShadowmapAtlas::ShadowmapIndicesInNode& ShadowmapAtlas::GetNodeOfTree(const Location& location)
        {
            auto it = m_shadowmapIndexNodeTree.find(location);
            if (it != m_shadowmapIndexNodeTree.end())
            {
                return it->second;
            }
            
            if (location.empty())
            {
                // The root subtable has size the array slice count of the atlas image resource,
                // it is not determined at this point.  (It is detemined during Finalize().)
                // So we create it as an empty ShadowmapIndicesInNode here.
                m_shadowmapIndexNodeTree.emplace(location, ShadowmapIndicesInNode(0));
            }
            else
            {
                // It reserves parent subtable node placeholder.
                const Location parentLocation(location.begin(), location.end() - 1);
                ShadowmapIndicesInNode& parentNode = GetNodeOfTree(parentLocation);
                if (parentLocation.empty() && location.back() >= parentNode.size())
                {
                    // parentLocation indicates the root location.
                    // The root subtable's size is not determined at this point,
                    // so just reserve the required size.
                    // The adding location is shared by multiple shadowmaps,
                    // so its value is InvalidIndex.
                    AZ_Assert(parentNode.size() == location.back(), "Root node should grow its size gradually.");
                    parentNode.push_back(InvalidIndex);
                }

                // A non root subtable has size LocationIndexNum.
                m_shadowmapIndexNodeTree.emplace(location, ShadowmapIndicesInNode(LocationIndexNum, InvalidIndex));
            }
            return m_shadowmapIndexNodeTree.at(location);
        }

        Data::Instance<RPI::Buffer> ShadowmapAtlas::CreateShadowmapIndexTableBuffer(const AZStd::string& bufferName) const
        {
            AZ_Assert(!m_requireFinalize, "Finalization is required.");
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
            desc.m_bufferName = bufferName;
            desc.m_elementSize = sizeof(ShadowmapIndexNode);
            desc.m_elementFormat = RHI::Format::R32G32_UINT;
            desc.m_byteCount = m_indexTableData.size() * sizeof(ShadowmapIndexNode);
            desc.m_bufferData = m_indexTableData.data();

            return RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        const AZStd::vector<ShadowmapAtlas::ShadowmapIndexNode> &ShadowmapAtlas::GetShadowmapIndexTable() const
        {
            AZ_Assert(!m_requireFinalize, "Finalization is required.");
            return m_indexTableData;
        }

        void ShadowmapAtlas::BuildIndexTableData()
        {
            AZ_Assert(!m_requireFinalize, "Finalization is required.");

            const size_t subtableCount = m_shadowmapIndexNodeTree.size();
            const uint32_t rootSubtableSize = GetArraySliceCount();

            if (subtableCount == 0)
            {
                m_indexTableData.resize(rootSubtableSize);
                return;
            }
            const size_t nonRootSubtableCount = subtableCount - 1;

            // The subtable of root (at location []) has the size of array slice count,
            // and the other subtabls has the size LocationIndexNum.
            m_indexTableData.resize(rootSubtableSize + nonRootSubtableCount * LocationIndexNum);

            // Determine indices of subtables, which enables calculation
            // of offset of each subtable in the table.
            AZStd::vector<Location> nodes;
            nodes.reserve(nonRootSubtableCount);
            AZStd::unordered_map<Location, uint32_t, LocationHasher> indices;
            { // scope to avoid variable name pollution.
                for (const auto& it : m_shadowmapIndexNodeTree)
                {
                    const Location& location = it.first;
                    if (location.empty())
                    {
                        continue; // skip entry in the root subtable
                    }
                    nodes.push_back(location);
                }
                AZStd::sort(
                    nodes.begin(), nodes.end(),
                    [](const Location& lhs, const Location& rhs) -> bool
                    {
                        if (lhs.size() == rhs.size())
                        {
                            for (size_t index = 0; index < lhs.size(); ++index)
                            {
                                if (lhs[index] != rhs[index])
                                {
                                    return lhs[index] < rhs[index];
                                }
                            }
                        }
                        return lhs.size() < rhs.size();
                    });
                AZ_Assert(nodes.size() == nonRootSubtableCount, "subtable count has an unexpected value.");

                for (uint32_t index = 0; index < nodes.size(); ++index)
                {
                    indices[nodes[index]] = index;
                }
            }

            // Store the next table offsets and shadowmap indices into the table.
            for (const auto& it : m_shadowmapIndexNodeTree)
            {
                const Location& location = it.first;
                const ShadowmapIndicesInNode& indicesInNode = it.second;

                uint8_t digitCount = LocationIndexNum;
                uint32_t indexInTableBase = 0;
                if (location.empty())
                {
                    digitCount = m_maxArraySlice + 1;
                }
                else
                {
                    const uint32_t subtableIndex = indices.at(location);
                    indexInTableBase = rootSubtableSize + subtableIndex * LocationIndexNum;
                }
                for (uint8_t digit = 0; digit < digitCount; ++digit)
                {
                    const uint32_t indexInTable = indexInTableBase + digit;
                    if (indicesInNode[digit] == InvalidIndex)
                    {
                        // This atlas does not have shadowmap in this location.
                        // In such a case, look for the subtable,
                        // and put its offset to m_nextTableOffset if it exists.
                        Location childLocation = location;
                        childLocation.push_back(digit);
                        const auto childIt = indices.find(childLocation);
                        if (childIt != indices.end())
                        {
                            const uint32_t offset = rootSubtableSize + childIt->second * LocationIndexNum;
                            m_indexTableData[indexInTable].m_nextTableOffset = offset;
                        }
                    }
                    else
                    {
                        // This atlas has a shadowmap of index indicesInNode[digit] in this location.
                        // In such a case, put its auxiliary data and
                        // set m_nextTableOffset to 0.
                        m_indexTableData[indexInTable].m_shadowmapIndex = aznumeric_cast<uint32_t>(indicesInNode[digit]);
                        m_indexTableData[indexInTable].m_nextTableOffset = 0;
                    }
                }
            }
        }

    } // namespace Render
} // namespace AZ
