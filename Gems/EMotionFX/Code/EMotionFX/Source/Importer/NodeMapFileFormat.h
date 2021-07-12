/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the shared structs
#include "SharedFileFormatStructs.h"


namespace EMotionFX
{
    namespace FileFormat
    {
        // the chunks
        enum
        {
            CHUNK_NODEMAP   = 600
        };


        struct NodeMap_Header
        {
            uint8 mFourCC[4];   // must be "MOS "
            uint8 mHiVersion;   // high version (2  in case of v2.34)
            uint8 mLoVersion;   // low version  (34 in case of v2.34)
            uint8 mEndianType;  // the endian in which the data is saved [0=little, 1=big]
        };


        struct NodeMapChunk
        {
            uint32  mNumEntries;// the number of mapping entries

            // followed by:
            // String sourceActorFileName
            // for all mNumEntries
            //      String firstNodeName;
            //      String secondNodeName;
        };
    } // namespace FileFormat
} // namespace EMotionFX
