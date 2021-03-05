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
