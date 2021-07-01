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
            CHUNK_MOTIONSET             = 500
        };


        struct MotionSet_Header
        {
            uint8 mFourCC[4];   // must be "MOS "
            uint8 mHiVersion;   // high version (2  in case of v2.34)
            uint8 mLoVersion;   // low version  (34 in case of v2.34)
            uint8 mEndianType;  // the endian in which the data is saved [0=little, 1=big]
        };


        struct MotionSetsChunk
        {
            uint32  mNumSets;               // the number of motion sets

            // followed by:
            // motionSets[mNumSets]
        };


        struct MotionSetChunk
        {
            uint32  mNumChildSets;              // the number of child motion sets
            uint32  mNumMotionEntries;          // the number of motion entries

            // followed by:
            // string : the name of the parent set
            // string : the name of the motion set
            // string : the filename and path information (e.g. "Root/Motions/Human_Male/" // obsolete, this got removed and is now just an empty string
            // motionEntries[mNumMotionEntries]: motion entries
            //     MotionEntry:
            //     string : motion filename without path (e.g. "Walk.motion")
            //     string : motion set string id (e.g. "WALK")
        };
    } // namespace FileFormat
} // namespace EMotionFX

