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
            uint32 m_chunkId;        // the chunk ID
            uint32 m_sizeInBytes;    // the size in bytes of this chunk (excluding this chunk struct)
            uint32 m_version;        // the version of the chunk
        };

        // color [0..1] range
        struct FileColor
        {
            float m_r; // red
            float m_g; // green
            float m_b; // blue
            float m_a; // alpha
        };

        struct FileVector2
        {
            float m_x;
            float m_y;
        };

        struct FileVector3
        {
            float m_x; // x+ = to the right
            float m_y; // y+ = forward
            float m_z; // z+ = up
        };

        // a compressed 3D vector
        struct File16BitVector3
        {
            uint16 m_x;  // x+ = to the right
            uint16 m_y;  // y+ = forward
            uint16 m_z;  // z+ = up
        };

        // a compressed 3D vector
        struct File8BitVector3
        {
            uint8 m_x;   // x+ = to the right
            uint8 m_y;   // y+ = forward
            uint8 m_z;   // z+ = up
        };

        struct FileQuaternion
        {
            float m_x;
            float m_y;
            float m_z;
            float m_w;
        };

        // the 16 bit component quaternion
        struct File16BitQuaternion
        {
            int16   m_x;
            int16   m_y;
            int16   m_z;
            int16   m_w;
        };

        // a time stamp chunk
        struct FileTime
        {
            uint16  m_year;
            int8    m_month;
            int8    m_day;
            int8    m_hours;
            int8    m_minutes;
            int8    m_seconds;
        };
    } // namespace FileFormat
} // namespace EMotionFX
