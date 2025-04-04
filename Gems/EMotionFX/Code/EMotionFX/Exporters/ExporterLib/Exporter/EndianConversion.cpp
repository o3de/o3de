/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Quaternion.h>

namespace ExporterLib
{
    void CopyVector2(EMotionFX::FileFormat::FileVector2& to, const AZ::Vector2& from)
    {
        to.m_x = from.GetX();
        to.m_y = from.GetY();
    }

    void CopyVector(EMotionFX::FileFormat::FileVector3& to, const AZ::PackedVector3f& from)
    {
        to.m_x = from.GetX();
        to.m_y = from.GetY();
        to.m_z = from.GetZ();
    }

    void CopyQuaternion(EMotionFX::FileFormat::FileQuaternion& to, const AZ::Quaternion& from)
    {
        AZ::Quaternion q = from;
        if (q.GetW() < 0.0f)
        {
            q = -q;
        }

        to.m_x = q.GetX();
        to.m_y = q.GetY();
        to.m_z = q.GetZ();
        to.m_w = q.GetW();
    }


    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, const AZ::Quaternion& from)
    {
        AZ::Quaternion q = from;
        if (q.GetW() < 0.0f)
        {
            q = -q;
        }

        const MCore::Compressed16BitQuaternion compressedQuat(q);
        to.m_x = compressedQuat.m_x;
        to.m_y = compressedQuat.m_y;
        to.m_z = compressedQuat.m_z;
        to.m_w = compressedQuat.m_w;
    }


    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, const MCore::Compressed16BitQuaternion& from)
    {
        MCore::Compressed16BitQuaternion q = from;
        if (q.m_w < 0)
        {
            q.m_x = -q.m_x;
            q.m_y = -q.m_y;
            q.m_z = -q.m_z;
            q.m_w = -q.m_w;
        }

        to.m_x = q.m_x;
        to.m_y = q.m_y;
        to.m_z = q.m_z;
        to.m_w = q.m_w;
    }


    void ConvertUnsignedInt(uint32* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }

    void ConvertUnsignedInt(uint64* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt64(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }

    void ConvertInt(int* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertSignedInt32(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertUnsignedShort(uint16* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt16(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFloat(float* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileChunk(EMotionFX::FileFormat::FileChunk* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(&value->m_chunkId,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_sizeInBytes,  EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_version,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileColor(EMotionFX::FileFormat::FileColor* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->m_r, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_g, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_b, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_a, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileVector2(EMotionFX::FileFormat::FileVector2* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->m_x, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_y, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileVector3(EMotionFX::FileFormat::FileVector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->m_x, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_y, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_z, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFile16BitVector3(EMotionFX::FileFormat::File16BitVector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt16(&value->m_x, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->m_y, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->m_z, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileQuaternion(EMotionFX::FileFormat::FileQuaternion* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->m_x, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_y, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_z, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_w, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFile16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertSignedInt16(&value->m_x, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->m_y, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->m_z, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->m_w, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileMotionEvent(EMotionFX::FileFormat::FileMotionEvent* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->m_startTime,         EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->m_endTime,           EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_eventTypeIndex,    EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_mirrorTypeIndex,   EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->m_paramIndex,        EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileMotionEventTable(EMotionFX::FileFormat::FileMotionEventTrack* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(&value->m_numEvents,            EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_numTypeStrings,       EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_numParamStrings,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->m_numMirrorTypeStrings, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }

    void ConvertVector3(AZ::PackedVector3f* value, MCore::Endian::EEndianType targetEndianType)
    {
        float* data = reinterpret_cast<float*>(value);
        MCore::Endian::ConvertFloat(&data[0], EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&data[1], EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&data[2], EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }
} // namespace ExporterLib
