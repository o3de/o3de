/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <AtomCore/Instance/Instance.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Render
    {
        //! A shadowmap atlas is a single image array which contains shadowmaps
        //! with different sizes.  This ShadowmapAtlas class determines the location
        //! of each shadowmap.
        //! This class also offers ShadowmapIndexTable, by which a compute shader can
        //! determine the corresponding shadowmap index from a coordinate (x, y, z)
        //! in the image array resource.
        class ShadowmapAtlas final
        {
        public:
            ShadowmapAtlas() = default;
            ~ShadowmapAtlas() = default;

            struct Origin
            {
                uint16_t m_arraySlice = 0;
                AZStd::array<uint32_t, 2> m_originInSlice{ {0, 0} };
            };

            //! ShadowmapIndexTree is used to determine shadowmap index from 
            //! a coordinate in the shadowmap atlas resource.  It enables a single dispatch
            //! to compute for all shadowmaps in the atlas, instead of multiple dispatches
            //! for each shadowmap.
            //! ShadowmapIndexTree is represented by an array of ShadowmapIndexNode.
            //! For the structure of ShadowmapIndexTree, refere the comment on
            //! CreateShadowmapIndexTableBuffer() below.
            struct ShadowmapIndexNode
            {
                // If a Location (see below) is shared by several sub-shadowmaps in a subtable,
                // an m_nextTableOffset offers the offset to the subtable for the sub-shadowmaps.
                // If a Location is  occupied by a shadowmap (so it is not shared by sub-shadowmaps),
                // then m_nextTableOffset == 0, which works as the terminator for seaching
                // a shadowmap index in a compute shader.
                uint32_t m_nextTableOffset = 0;
                uint32_t m_shadowmapIndex = ~0; // invalid index
            };

            //! This initializes the packing of shadowmap sizes.
            void Initialize();

            //! This sets shadowmap size.
            //! @param index light index of shadowmap.
            //! @param size shadowmap size (width/height).
            void SetShadowmapSize(size_t index, ShadowmapSize size);

            //! This decides shadowmap locations in the atlas.
            //! As we describe below (above of definition of Location), a location is
            //! encoded in a finite sequence of non-negative integers.
            //! This determines the locations from the largest shadowmaps to smaller ones,
            //! and the locations of the same size are occupied in the lexicographical order,
            //! e.g., [0,1,1] is occupied before [0,1,2].
            void Finalize();

            //! This returns array slice count of the atlas.
            //! This has to be called after execution of Finalize().
            //! @return array slice count of the atlas.
            uint16_t GetArraySliceCount() const;

            //! This returns image size of the atlas.
            //! This has to be called after execution of Finalize().
            //! @return image size of the atlas.
            ShadowmapSize GetBaseShadowmapSize() const;

            //! This returns shadowmap origin in the atlas.
            //! This has to be called after execution of Finalize().
            //! @param index shadowmap index.
            //! @return shadowmap origin in the atlas.
            Origin GetOrigin(size_t index) const;

            //! This returns a buffer by which a shader finds the shadowmap index in the atlas.
            //! @param bufferName name of the buffer
            //! @return buffer which a computer shader look up for the shadowmap index.
            //!
            //! The buffer contains a table ShadowmapIndexTable which consists of "subtables."
            //! A subtable is an array of ShadowmapIndexNodes, and corresponds to 
            //! a shadowmap Location.  A ShadowmapIndexNode offers either the offset to 
            //! another subtable or ths shadowmap index.  So a subtable offers the offsets
            //! of the sub-subtable and shadowmap indices for direct children Location
            //! of the corresponding Location.
            //! We call the subtable for Locaion [] (the empty array) by "root subtable."
            //! The size of the root subtable equals to the array slice count of the image resource
            //! which the atlas occupies.
            //! The size of a non-root subtable (i.e., subtable for non-empty Location) is
            //! LocationIndexNum, i.e., 4.
            //! If m_nextTableOffset != 0, then m_nextTableOffset means the corresponding
            //! subtable's offset from the beginning of the buffer.
            //! If m_nextTableOffset == 0, then m_shadowmapIndex is the required shadowmap index.
            //!
            //! For example, we assume baseShadowmapSize = 2048 
            //! light #0 uses Location [0], #1 does [1,0], and #2 does [1,1].
            //! Then the resulting array "table" of T has size 6 and would be like below:
            //!
            //! (root subtable, i.e., subtable for Location [])
            //! table[0] : m_nextTableOffset == 0, and m_shadowmapIndex == 0
            //!            the Location of the shadowmap is [0].
            //! table[1] : m_nextTableOffset == 2 which indicate slice #1 is shared
            //!             by multiple shadowmaps and the next subtable begin with offset 2.
            //!
            //! (subtalble for Location [1])
            //! table[2 + 0] : m_nextTableOffset == 0 and m_shadowmapIndex == 1
            //!                the Location of the shadowmap is [1, 0].
            //! table[2 + 1] : m_nextTableOffset == 0 and m_shadowmapIndex == 2
            //!                the Location of the shadowmap is [1, 1].
            //! table[4], table[5] : not used.
            //! For the way to extract shadowmap specific parameter, refer ShadowmapAtlasLib.azsli.
            Data::Instance<RPI::Buffer> CreateShadowmapIndexTableBuffer(const AZStd::string& bufferName) const;

            const AZStd::vector<ShadowmapIndexNode>& GetShadowmapIndexTable() const;

        private:
            //! Location indicates the position of a shadowmap in the image array resource,
            //! the array slice in which it is placed and which sub-square it occupies.
            //! (Remember that a shadowmap's width and height are the same)
            //! Location is encoded by a finite sequence of non-negative integers.
            //! The length of the sequence indicates shadowmap size, as
            //!    the shadowmap size = baseShadowmapSize * (1 / 2^{len-1})
            //!    where len is the length of the sequence.
            //!
            //! The 0-th value of a Location indicates the array slice index in the image.
            //! For k>=0, the (k+1)-th value (0-3) of a Location indicates the sub-square
            //! in the square located by 0-th, ..., k-th value of the Location (see the figure below).
            //! So the square indicated by 0-th,..., (k+1)-th value has the half sized width
            //! of the square by 0-th,..., k-th.
            //!
            //!     +---+---+  -> X
            //!     | 0 | 1 |
            //!     +---+---+
            //!     | 2 | 3 |
            //!     +---+---+
            //!     |
            //!     v Y
            //!
            //!  For example, when baseShadowmapSize = 2048,
            //!  [0] indicates (0, 0)-(0+2047, 0+2047) of slice:0 (width 2048),
            //!  [1,1] indicates (1024, 0)-(1024+1023, 0+1023) of slice:1 (width 1024), and
            //!  [2,2,2] indicates (0, 1024+512)-(0+511, 1024+512+511) of slice:2 (width 512).
            using Location = AZStd::vector<uint8_t>;
            static constexpr uint8_t LocationIndexNum = 4;
            static constexpr size_t InvalidIndex = ~0;

            struct LocationHasher
            {
                using argument_type = Location;
                using result_type = size_t;

                result_type operator()(const argument_type& value) const
                {
                    const char* buffer = reinterpret_cast<const char*>(value.data());
                    return AZStd::hash_range(buffer, buffer + value.size() * sizeof(uint8_t));
                }
            };

            //! m_shadowmapIndexNodeTree is a tree represeting parent-child relation of Location,
            //! and the tree will be flatten into a table of ShadowmapIndexNode
            //! by BuildIndexTableData() below.
            //! The table consists of the subtables which correspond to
            //! the Locations in this atlas.
            //! A ShadowmapIndicesInNode has a correspondence to a subtable,
            //! and holds indices of shadowmaps directly contained in the subtable.
            using ShadowmapIndicesInNode = AZStd::vector<size_t>;

            //! This finds the "next" location of the given location in the mean of
            //! the lexicographic order.
            void SucceedLocation(Location& location);

            //! This sets a shadowmap index to the node of a given location
            //! in TableTree of the atlas.
            void SetShadowmapIndexInTree(const Location& location, size_t index);

            //! This returns the reference of the shadowmap indices in m_shadowmapIndexNodeTree
            //! for a given location.
            ShadowmapIndicesInNode& GetNodeOfTree(const Location& location);

            //! This flattens m_shadowmapIndexNodeTree to an array of ShadowmapIndexNode.
            void BuildIndexTableData();

            bool m_requireFinalize = true;
            ShadowmapSize m_baseShadowmapSize = MaxShadowmapImageSize;
            uint8_t m_maxArraySlice = 0;
            AZStd::unordered_map<ShadowmapSize, AZStd::list<size_t>> m_indicesForSize;
            AZStd::vector<ShadowmapIndexNode> m_indexTableData;

            AZStd::unordered_map<size_t, Location> m_locations;
            AZStd::unordered_map<Location, ShadowmapIndicesInNode, LocationHasher> m_shadowmapIndexNodeTree;
        };

    } // namespace Render
} // namespace AZ
