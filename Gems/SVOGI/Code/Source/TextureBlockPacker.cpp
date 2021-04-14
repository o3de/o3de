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

// Description : 3D block atlas


#include "SVOGI_precompiled.h"


#include "TextureBlockPacker.h"

namespace SVOGI
{
    const AZ::u32 nHS = 4;

    const AZ::u8 TextureBlock3D::s_freeBlock = 0xff;

    const AZ::s32 TextureBlockPacker3D::s_invalidBlockID = -1;

    TextureBlockPacker3D::TextureBlockPacker3D(const AZ::u32 dwLogWidth, const AZ::u32 dwLogHeight, const AZ::u32 dwLogDepth, bool bNonPow)
    {
        if (bNonPow)
        {
            m_width = dwLogWidth;
            m_height = dwLogHeight;
            m_depth = dwLogDepth;
        }
        else
        {
            m_width = 1 << dwLogWidth;
            m_height = 1 << dwLogHeight;
            m_depth = 1 << dwLogDepth;
        }

        m_blockBitmap.resize(m_width * m_height * m_depth, s_invalidBlockID);
        m_blockUsageGrid.resize(m_width * m_height * m_depth / nHS / nHS / nHS, 0);
        m_blocks.reserve(m_width * m_height * m_depth);
    }

    TextureBlock3D* TextureBlockPacker3D::GetBlockInfo(const AZ::s32 blockID)
    {
        if (blockID < 0 || blockID >= static_cast<AZ::s32>(m_blocks.size()))
        {
            return nullptr;
        }

        TextureBlock3D* ref = &m_blocks[blockID];
        if (ref->IsFree())
        {
            return nullptr;
        }

        return ref;
    }

    void TextureBlockPacker3D::RemoveBlock(const AZ::s32 blockID)
    {
        TextureBlock3D* blockInfo = GetBlockInfo(blockID);
        if (blockInfo)
        {
            FreeBlock(*blockInfo);
        }
    }

    void TextureBlockPacker3D::FreeBlock(TextureBlock3D& blockInfo)
    {
        if (!blockInfo.IsFree())
        {
            FillRect(blockInfo, s_invalidBlockID);

            blockInfo.MarkFree();
        }
    }

    AZ::s32 TextureBlockPacker3D::AddBlock(const AZ::u32 dwLogWidth, const AZ::u32 dwLogHeight, const AZ::u32 dwLogDepth, const AZ::Aabb &worldBox)
    {
        if (!dwLogWidth || !dwLogHeight || !dwLogDepth)
        {
            assert(!"Empty block");
        }

        AZ::u32 dwLocalWidth = dwLogWidth;
        AZ::u32 dwLocalHeight = dwLogHeight;
        AZ::u32 dwLocalDepth = dwLogDepth;

        AZ::u32 nCountNeeded = dwLocalWidth * dwLocalHeight * dwLocalDepth;

        AZ::u32 dwW = m_width / nHS;
        AZ::u32 dwH = m_height / nHS;
        AZ::u32 dwD = m_depth / nHS;

        for (AZ::u32 nZ = 0; nZ < dwD; nZ++)
        {
            for (AZ::u32 nY = 0; nY < dwH; nY++)
            {
                for (AZ::u32 nX = 0; nX < dwW; nX++)
                {
                    AZ::u32      dwMinX = nX * nHS;
                    AZ::u32      dwMinY = nY * nHS;
                    AZ::u32      dwMinZ = nZ * nHS;

                    AZ::u32      dwMaxX = (nX + 1) * nHS;
                    AZ::u32      dwMaxY = (nY + 1) * nHS;
                    AZ::u32      dwMaxZ = (nZ + 1) * nHS;

                    AZ::u32 nCountFree = nHS * nHS * nHS - m_blockUsageGrid[nX + nY * dwW + nZ * dwW * dwH];

                    if (nCountNeeded <= nCountFree)
                    {
                        TextureBlock3D testblock;

                        for (AZ::u32 dwZ = dwMinZ; dwZ < dwMaxZ; dwZ += dwLocalDepth)
                        {
                            for (AZ::u32 dwY = dwMinY; dwY < dwMaxY; dwY += dwLocalHeight)
                            {
                                for (AZ::u32 dwX = dwMinX; dwX < dwMaxX; dwX += dwLocalWidth)
                                {
                                    testblock.m_minX = dwX;
                                    testblock.m_maxX = dwX + dwLocalWidth;
                                    testblock.m_minY = dwY;
                                    testblock.m_maxY = dwY + dwLocalHeight;
                                    testblock.m_minZ = dwZ;
                                    testblock.m_maxZ = dwZ + dwLocalDepth;

                                    testblock.m_worldBox = worldBox;

                                    testblock.m_textureBox.SetMin(AZ::Vector3((float)testblock.m_minX / (float)m_width, (float)testblock.m_minY / (float)m_height, (float)testblock.m_minZ / (float)m_depth));
                                    testblock.m_textureBox.SetMax(AZ::Vector3((float)testblock.m_maxX / (float)m_width, (float)testblock.m_maxY / (float)m_height, (float)testblock.m_maxZ / (float)m_depth));

                                    testblock.m_atlasOffset = testblock.m_minZ * m_width * m_height + testblock.m_minY * m_width + testblock.m_minX;

                                    if (IsFree(testblock))
                                    {
                                        AZ::s32 blockID = FindFreeBlockIDOrCreateNew();

                                        m_blocks[blockID] = testblock;

                                        FillRect(testblock, blockID);

                                        return blockID;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return s_invalidBlockID;    // no space left to this block
    }

    void TextureBlockPacker3D::UpdateUsageGrid(const TextureBlock3D& rectIn)
    {
        TextureBlock3D rectUM;

        rectUM.m_minX = rectIn.m_minX / nHS;
        rectUM.m_minY = rectIn.m_minY / nHS;
        rectUM.m_minZ = rectIn.m_minZ / nHS;

        rectUM.m_maxX = (rectIn.m_maxX - 1) / nHS + 1;
        rectUM.m_maxY = (rectIn.m_maxY - 1) / nHS + 1;
        rectUM.m_maxZ = (rectIn.m_maxZ - 1) / nHS + 1;

        AZ::u32 dwW = m_width / nHS;
        AZ::u32 dwH = m_height / nHS;

        for (AZ::u32 dwZ = rectUM.m_minZ; dwZ < rectUM.m_maxZ; ++dwZ)
        {
            for (AZ::u32 dwY = rectUM.m_minY; dwY < rectUM.m_maxY; ++dwY)
            {
                for (AZ::u32 dwX = rectUM.m_minX; dwX < rectUM.m_maxX; ++dwX)
                {
                    TextureBlock3D rectTest = rectUM;

                    rectTest.m_minX = dwX * nHS;
                    rectTest.m_minY = dwY * nHS;
                    rectTest.m_minZ = dwZ * nHS;

                    rectTest.m_maxX = (dwX + 1) * nHS;
                    rectTest.m_maxY = (dwY + 1) * nHS;
                    rectTest.m_maxZ = (dwZ + 1) * nHS;

                    m_blockUsageGrid[dwX + dwY * dwW + dwZ * dwW * dwH] = GetUsedSlotsCount(rectTest);
                }
            }
        }
    }

    void TextureBlockPacker3D::FillRect(const TextureBlock3D& rect, AZ::s32 value)
    {
        for (AZ::u32 dwZ = rect.m_minZ; dwZ < rect.m_maxZ; ++dwZ)
        {
            for (AZ::u32 dwY = rect.m_minY; dwY < rect.m_maxY; ++dwY)
            {
                for (AZ::u32 dwX = rect.m_minX; dwX < rect.m_maxX; ++dwX)
                {
                    m_blockBitmap[dwX + dwY * m_width + dwZ * m_width * m_height] = value;
                }
            }
        }

        UpdateUsageGrid(rect);
    }

    AZ::u32 TextureBlockPacker3D::GetUsedSlotsCount(const TextureBlock3D& rect) const
    {
        AZ::u32 nCount = 0;

        for (AZ::u32 dwZ = rect.m_minZ; dwZ < rect.m_maxZ; ++dwZ)
        {
            for (AZ::u32 dwY = rect.m_minY; dwY < rect.m_maxY; ++dwY)
            {
                for (AZ::u32 dwX = rect.m_minX; dwX < rect.m_maxX; ++dwX)
                {
                    if (m_blockBitmap[dwX + dwY * m_width + dwZ * m_width * m_height] != s_invalidBlockID)
                    {
                        nCount++;
                    }
                }
            }
        }

        return nCount;
    }

    bool TextureBlockPacker3D::IsFree(const TextureBlock3D& rect) const
    {
        for (AZ::u32 dwZ = rect.m_minZ; dwZ < rect.m_maxZ; ++dwZ)
        {
            for (AZ::u32 dwY = rect.m_minY; dwY < rect.m_maxY; ++dwY)
            {
                for (AZ::u32 dwX = rect.m_minX; dwX < rect.m_maxX; ++dwX)
                {
                    if (m_blockBitmap[dwX + dwY * m_width + dwZ * m_width * m_height] != s_invalidBlockID)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    AZ::s32 TextureBlockPacker3D::FindFreeBlockIDOrCreateNew()
    {
        AZ::s32 index = 0;
        for (; index < m_blocks.size(); ++index)
        {
            if (m_blocks[index].IsFree())
            {
                return index; // Found free block
            }
        }

        // Create new block
        m_blocks.push_back(TextureBlock3D());

        return index;
    }
}
