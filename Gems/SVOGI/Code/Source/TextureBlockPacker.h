/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Aabb.h>


/*
  These classes are used to map brick data into 3d textures so that we can do cone tracing on the gpu.
  The blocks represent the texture regions and associate meta data.
  The block packer is responsible for mapping blocks to free areas of the texture.
  Currently the blocks and texture regions are mapped in a 1:1 fashion but as a future optimizaiton
  we should utilize compression to map multiple blocks into a texture region.

*/
namespace SVOGI
{
    struct TextureBlock3D
    {
        const static AZ::u8 s_freeBlock;

        TextureBlock3D()
        {
            memset(this, 0, sizeof(*this));
            MarkFree();
        }

        AZ::Aabb    m_worldBox;
        AZ::Aabb    m_textureBox;
        AZ::u32     m_atlasOffset;
        AZ::u8      m_minX;       // 0xff        if free, inclusive
        AZ::u8      m_minY;       // not defined if free, inclusive
        AZ::u8      m_minZ;       // not defined if free, inclusive
        AZ::u8      m_maxX;       // not defined if free, non inclusive
        AZ::u8      m_maxY;       // not defined if free, non inclusive
        AZ::u8      m_maxZ;       // not defined if free, not inclusive
        AZ::u32     m_lastUpdatedFrame;
        bool        m_staticDirty;
        bool        m_dynamicDirty;

        bool IsFree() const
        {
            return m_minX == s_freeBlock;
        }

        void MarkFree()
        {
            m_minX = s_freeBlock;
        }
    };

    class TextureBlockPacker3D
    {
    public:
        AZ_CLASS_ALLOCATOR(TextureBlockPacker3D, AZ::SystemAllocator, 0);

        const static AZ::s32 s_invalidBlockID;

        // constructor
        // Arguments:
        //   dwLogHeight - e.g. specify 5 for 32, keep is small like ~ 5 or 6, don't use pixel size
        //   dwLogHeight - e.g. specify 5 for 32, keep is small like ~ 5 or 6, don't use pixel size
        TextureBlockPacker3D(const AZ::u32 dwLogWidth, const AZ::u32 dwLogHeight, const AZ::u32 dwLogDepth, const bool bNonPow = false);

        // Arguments:
        //   dwLogHeight - e.g. specify 5 for 32
        //   dwLogHeight - e.g. specify 5 for 32
        // Returns:
        //   Block ID or s_invalidBlockID if there was no free space
        AZ::s32 AddBlock(const AZ::u32 dwLogWidth, const AZ::u32 dwLogHeight, const AZ::u32 dwLogDepth, const AZ::Aabb& worldBox);

        // Arguments:
        //   blockID - as it was returned from AddBlock()
        TextureBlock3D* GetBlockInfo(const AZ::s32 blockID);

        // Arguments:
        //   blockID - as it was returned from AddBlock()
        void RemoveBlock(const AZ::s32 blockID);

        AZ::u32 GetNumBlocks() const
        {
            return m_blocks.size();
        }

    private: // ----------------------------------------------------------

        AZStd::vector<TextureBlock3D>   m_blocks;
        AZStd::vector<AZ::s32>          m_blockBitmap;
        AZStd::vector<AZ::u32>          m_blockUsageGrid;
        AZ::u32                         m_width;
        AZ::u32                         m_height;
        AZ::u32                         m_depth;
        void FillRect(const TextureBlock3D& rect, AZ::s32 value);

        void FreeBlock(TextureBlock3D& blockInfo);

        bool IsFree(const TextureBlock3D& rect) const;

        AZ::u32 GetUsedSlotsCount(const TextureBlock3D& rect) const;

        void UpdateUsageGrid(const TextureBlock3D& rectIn);

        AZ::s32 FindFreeBlockIDOrCreateNew();
    };
}