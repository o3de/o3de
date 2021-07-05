/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        to.mX = from.GetX();
        to.mY = from.GetY();
    }

    void CopyVector(EMotionFX::FileFormat::FileVector3& to, const AZ::PackedVector3f& from)
    {
        to.mX = from.GetX();
        to.mY = from.GetY();
        to.mZ = from.GetZ();
    }

    void CopyQuaternion(EMotionFX::FileFormat::FileQuaternion& to, const AZ::Quaternion& from)
    {
        AZ::Quaternion q = from;
        if (q.GetW() < 0.0f)
        {
            q = -q;
        }

        to.mX = q.GetX();
        to.mY = q.GetY();
        to.mZ = q.GetZ();
        to.mW = q.GetW();
    }


    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, const AZ::Quaternion& from)
    {
        AZ::Quaternion q = from;
        if (q.GetW() < 0.0f)
        {
            q = -q;
        }

        const MCore::Compressed16BitQuaternion compressedQuat(q);
        to.mX = compressedQuat.mX;
        to.mY = compressedQuat.mY;
        to.mZ = compressedQuat.mZ;
        to.mW = compressedQuat.mW;
    }


    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, const MCore::Compressed16BitQuaternion& from)
    {
        MCore::Compressed16BitQuaternion q = from;
        if (q.mW < 0)
        {
            q.mX = -q.mX;
            q.mY = -q.mY;
            q.mZ = -q.mZ;
            q.mW = -q.mW;
        }

        to.mX = q.mX;
        to.mY = q.mY;
        to.mZ = q.mZ;
        to.mW = q.mW;
    }


    void CopyColor(const MCore::RGBAColor& from, EMotionFX::FileFormat::FileColor& to)
    {
        to.mR = from.r;
        to.mG = from.g;
        to.mB = from.b;
        to.mA = from.a;
    }


    void ConvertUnsignedInt(uint32* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
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
        MCore::Endian::ConvertUnsignedInt32(&value->mChunkID,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mSizeInBytes,  EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mVersion,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileColor(EMotionFX::FileFormat::FileColor* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mR, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mG, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mB, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mA, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileVector2(EMotionFX::FileFormat::FileVector2* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileVector3(EMotionFX::FileFormat::FileVector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFile16BitVector3(EMotionFX::FileFormat::File16BitVector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt16(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileQuaternion(EMotionFX::FileFormat::FileQuaternion* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mW, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFile16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertSignedInt16(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->mW, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileMotionEvent(EMotionFX::FileFormat::FileMotionEvent* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mStartTime,         EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mEndTime,           EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mEventTypeIndex,    EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mMirrorTypeIndex,   EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->mParamIndex,        EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileMotionEventTable(EMotionFX::FileFormat::FileMotionEventTrack* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(&value->mNumEvents,            EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mNumTypeStrings,       EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mNumParamStrings,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mNumMirrorTypeStrings, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertRGBAColor(MCore::RGBAColor* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->r, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->g, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->b, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->a, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertVector3(AZ::PackedVector3f* value, MCore::Endian::EEndianType targetEndianType)
    {
        float* data = reinterpret_cast<float*>(value);
        MCore::Endian::ConvertFloat(&data[0], EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&data[1], EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&data[2], EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }
} // namespace ExporterLib
