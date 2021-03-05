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


        // a chunk
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


        // a 2D vector
        struct FileVector2
        {
            float mX;
            float mY;
        };


        // a 3D vector
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


        // a quaternion
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


        // attribute
        struct FileAttribute
        {
            uint32  mDataType;
            uint32  mNumBytes;
            uint32  mFlags;

            // followed by:
            //      uint8   mData[mNumBytes];
        };
    } // namespace FileFormat
}   // namespace EMotionFX
