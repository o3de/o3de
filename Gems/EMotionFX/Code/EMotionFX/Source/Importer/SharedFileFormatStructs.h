/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MCore/Source/Config.h>

namespace EMotionFX
{
    namespace FileFormat // so now we are in the namespace EMotionFX::FileFormat
    {
        // shared chunk ID's
        enum
        {
            SHARED_CHUNK_MOTIONEVENTTABLE   = 50,
            SHARED_CHUNK_TIMESTAMP          = 51
        };

        struct FileChunk
        {
            uint32 mChunkID;        // the chunk ID
            uint32 mSizeInBytes;    // the size in bytes of this chunk (excluding this chunk struct)
            uint32 mVersion;        // the version of the chunk
        };

        // color [0..1] range
        struct FileColor
        {
            float mR; // red
            float mG; // green
            float mB; // blue
            float mA; // alpha
        };

        struct FileVector2
        {
            float mX;
            float mY;
        };

        struct FileVector3
        {
            float mX; // x+ = to the right
            float mY; // y+ = forward
            float mZ; // z+ = up
        };

        // a compressed 3D vector
        struct File16BitVector3
        {
            uint16 mX;  // x+ = to the right
            uint16 mY;  // y+ = forward
            uint16 mZ;  // z+ = up
        };

        // a compressed 3D vector
        struct File8BitVector3
        {
            uint8 mX;   // x+ = to the right
            uint8 mY;   // y+ = forward
            uint8 mZ;   // z+ = up
        };

        struct FileQuaternion
        {
            float mX;
            float mY;
            float mZ;
            float mW;
        };

        // the 16 bit component quaternion
        struct File16BitQuaternion
        {
            int16   mX;
            int16   mY;
            int16   mZ;
            int16   mW;
        };

        // a time stamp chunk
        struct FileTime
        {
            uint16  mYear;
            int8    mMonth;
            int8    mDay;
            int8    mHours;
            int8    mMinutes;
            int8    mSeconds;
        };
    } // namespace FileFormat
} // namespace EMotionFX
