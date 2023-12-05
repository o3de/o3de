/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// swap bytes for an uint16
MCORE_INLINE void Endian::ConvertUnsignedInt16(uint16* value, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        *value = (*value & 0xFF00) >> 8 | (*value & 0x00FF) << 8;
        value++;
    }
}


// swap bytes for an uint32
MCORE_INLINE void Endian::ConvertUnsignedInt32(uint32* value, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        uint32 arg = *value;
        *value = (arg & 0xFF000000) >> 24 | (arg & 0x00FF0000) >> 8 | (arg & 0x0000FF00) << 8 | (arg & 0x000000FF) << 24;
        value++;
    }
}

MCORE_INLINE void Endian::ConvertUnsignedInt64(uint64* value, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        uint64 arg = *value;
        *value = (arg >> 56) + ((arg >> 40) & 0xFF00) + ((arg >> 24) & 0xFF0000) + ((arg >> 8) & 0xFF000000)
            + ((arg & 0xFF000000) << 8) + ((arg & 0xFF0000) << 24) + ((arg & 0xFF00) << 40) + (arg << 56);
        value++;
    }
}


// swap bytes for a short
MCORE_INLINE void Endian::ConvertSignedInt16(int16* value, uint32 count)
{
    ConvertUnsignedInt16((uint16*)value, count);
}


// swap bytes for an int32
MCORE_INLINE void Endian::ConvertSignedInt32(int32* value, uint32 count)
{
    ConvertUnsignedInt32((uint32*)value, count);
}


// swap bytes for a float
MCORE_INLINE void Endian::ConvertFloat(float* value, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        uint8* byteData = (uint8*)value;
        SwapBytes(&byteData[0], &byteData[3]);
        SwapBytes(&byteData[1], &byteData[2]);
        /*      uint32 ui = *((uint32*)value);
                ui = (ui & 0xFF000000) >> 24 | (ui & 0x00FF0000) >> 8 | (ui & 0x0000FF00) << 8 | (ui & 0x000000FF) << 24;
                *value = *((float*)&ui);*/
        value++;
    }
}


// swap bytes
MCORE_INLINE void Endian::SwapBytes(uint8* byteA, uint8* byteB)
{
    // make sure we don't try to swap the same memory address
    MCORE_ASSERT(byteA != byteB);

    // perform the swap
    uint8 tempValue = *byteA;
    *byteA = *byteB;
    *byteB = tempValue;
}


// convert a 16 bit value's endian
MCORE_INLINE void Endian::ConvertEndian16(uint8* buffer, uint32 count)
{
    // for all 16 bit values in the buffer we want to process
    for (uint32 i = 0; i < count; ++i)
    {
        SwapBytes(&buffer[0], &buffer[1]);  // swap the two high and low byte
        buffer += 2;                        // go to the next 16 bit value
    }
}


// convert a 32 bit value's endian
MCORE_INLINE void Endian::ConvertEndian32(uint8* buffer, uint32 count)
{
    // for all 32 bit values in the buffer we want to process
    for (uint32 i = 0; i < count; ++i)
    {
        SwapBytes(&buffer[0], &buffer[3]);
        SwapBytes(&buffer[1], &buffer[2]);
        buffer += 4;    // go to the next 32 bit value
    }
}


// convert a 64 bit value's endian
MCORE_INLINE void Endian::ConvertEndian64(uint8* buffer, uint32 count)
{
    // for all 64 bit values in the buffer we want to process
    for (uint32 i = 0; i < count; ++i)
    {
        SwapBytes(&buffer[0], &buffer[7]);
        SwapBytes(&buffer[1], &buffer[6]);
        SwapBytes(&buffer[2], &buffer[5]);
        SwapBytes(&buffer[3], &buffer[4]);
        buffer += 8;    // go to the next 64 bit value
    }
}


// convert a float
MCORE_INLINE void Endian::ConvertFloat(float* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert the float into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32((uint8*)value, count);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32   ((uint8*)value, count);
        break;
    default:
        ;
    }
    ;
}


// convert an int32
MCORE_INLINE void Endian::ConvertSignedInt32(int32* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32((uint8*)value, count);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32   ((uint8*)value, count);
        break;
    default:
        ;
    }
    ;
}


// convert an uint32
MCORE_INLINE void Endian::ConvertUnsignedInt32(uint32* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32((uint8*)value, count);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32   ((uint8*)value, count);
        break;
    default:
        ;
    }
    ;
}

MCORE_INLINE void Endian::ConvertUnsignedInt64(uint64* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN64((uint8*)value, count);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN64   ((uint8*)value, count);
        break;
    }
}


// convert a short
MCORE_INLINE void Endian::ConvertSignedInt16(int16* value, EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN16((uint8*)value, count);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN16   ((uint8*)value, count);
        break;
    default:
        ;
    }
    ;
}


// convert an uint16
MCORE_INLINE void Endian::ConvertUnsignedInt16(uint16* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN16((uint8*)value, count);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN16   ((uint8*)value, count);
        break;
    default:
        ;
    }
    ;
}


// convert a Vector2
MCORE_INLINE void Endian::ConvertVector2(AZ::Vector2* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32  ((uint8*)value, count << 1);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32     ((uint8*)value, count << 1);
        break;
    default:
        ;
    }
    ;
}


// convert a Vector3
MCORE_INLINE void Endian::ConvertVector3(AZ::Vector3* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32  ((uint8*)value, count * 3);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32     ((uint8*)value, count * 3);
        break;
    default:
        ;
    }
    ;
}


// convert a Vector4
MCORE_INLINE void Endian::ConvertVector4(AZ::Vector4* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32  ((uint8*)value, count << 2);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32     ((uint8*)value, count << 2);
        break;
    default:
        ;
    }
    ;
}


// convert a Quaternion
MCORE_INLINE void Endian::ConvertQuaternion(AZ::Quaternion* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN32  ((uint8*)value, count << 2);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN32     ((uint8*)value, count << 2);
        break;
    default:
        ;
    }
    ;
}


// convert a 16 bit compressed quaternion
MCORE_INLINE void Endian::Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, Endian::EEndianType sourceEndianType, uint32 count)
{
    // convert into the new endian, depending on the platform we are running on
    switch (sourceEndianType)
    {
    case ENDIAN_LITTLE:
        MCORE_FROM_LITTLE_ENDIAN16  ((uint8*)value, count << 2);
        break;
    case ENDIAN_BIG:
        MCORE_FROM_BIG_ENDIAN16     ((uint8*)value, count << 2);
        break;
    default:
        ;
    }
    ;
}


//-----------------------------------------------------------------------

// convert a float into another endian type
MCORE_INLINE void Endian::ConvertFloat(float* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertFloat(value, count);
}


// convert an int32 into another endian type
MCORE_INLINE void Endian::ConvertSignedInt32(int32* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertSignedInt32(value, count);
}


// convert an uint32 into another endian type
MCORE_INLINE void Endian::ConvertUnsignedInt32(uint32* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertUnsignedInt32(value, count);
}

// convert an uint64 into another endian type
MCORE_INLINE void Endian::ConvertUnsignedInt64(uint64* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertUnsignedInt64(value, count);
}


// convert a short into another endian type
MCORE_INLINE void Endian::ConvertSignedInt16(int16* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertSignedInt16(value, count);
}


// convert an uint16 into another endian type
MCORE_INLINE void Endian::ConvertUnsignedInt16(uint16* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertUnsignedInt16(value, count);
}


// convert a Vector2 into another endian type
MCORE_INLINE void Endian::ConvertVector2(AZ::Vector2* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertFloat((float*)value, count << 1);
}


// convert a Vector3 into another endian type
MCORE_INLINE void Endian::ConvertVector3(AZ::Vector3* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertFloat((float*)value, count * 3);
}


// convert a Vector4 into another endian type
MCORE_INLINE void Endian::ConvertVector4(AZ::Vector4* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertFloat((float*)value, count << 2);
}


// convert a Quaternion into another endian type
MCORE_INLINE void Endian::ConvertQuaternion(AZ::Quaternion* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertFloat((float*)value, count << 2);
}


// convert a Quaternion into another endian type
MCORE_INLINE void Endian::Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count)
{
    // if we don't need to convert anything
    if (sourceEndianType == targetEndianType)
    {
        return;
    }

    // perform conversion
    ConvertSignedInt16((int16*)value, count << 2);
}

// convert a Vector2
MCORE_INLINE void Endian::ConvertVector2(AZ::Vector2* value, uint32 count)
{
    ConvertFloat((float*)value, count << 1);
}


// convert a Vector3
MCORE_INLINE void Endian::ConvertVector3(AZ::Vector3* value, uint32 count)
{
    ConvertFloat((float*)value, 3 * count);
}


// convert a Vector4
MCORE_INLINE void Endian::ConvertVector4(AZ::Vector4* value, uint32 count)
{
    ConvertFloat((float*)value, count << 2);
}


// convert a Quaternion
MCORE_INLINE void Endian::ConvertQuaternion(AZ::Quaternion* value, uint32 count)
{
    ConvertFloat((float*)value, count << 2);
}


// convert a 16-bit Quaternion
MCORE_INLINE void Endian::Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, uint32 count)
{
    ConvertSignedInt16((int16*)value, count << 2);
}


// convert a float into another endian type
MCORE_INLINE void Endian::ConvertFloatTo(float* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertFloat(value, count);
}


// convert an int32 into another endian type
MCORE_INLINE void Endian::ConvertSignedInt32To(int32* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertSignedInt32(value, count);
}


// convert an uint32 into another endian type
MCORE_INLINE void Endian::ConvertUnsignedInt32To(uint32* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertUnsignedInt32(value, count);
}


// convert a short into another endian type
MCORE_INLINE void Endian::ConvertSignedInt16To(int16* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertSignedInt16(value, count);
}


// convert an uint16 into another endian type
MCORE_INLINE void Endian::ConvertUnsignedInt16To(uint16* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertUnsignedInt16(value, count);
}


// convert a Vector2 into another endian type
MCORE_INLINE void Endian::ConvertVector2To(AZ::Vector2* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertFloat((float*)value, count << 1);
}


// convert a Vector3 into another endian type
MCORE_INLINE void Endian::ConvertVector3To(AZ::Vector3* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertFloat((float*)value, count * 3);
}


// convert a Vector4 into another endian type
MCORE_INLINE void Endian::ConvertVector4To(AZ::Vector4* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertFloat((float*)value, count << 2);
}


// convert a Quaternion into another endian type
MCORE_INLINE void Endian::ConvertQuaternionTo(AZ::Quaternion* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertFloat((float*)value, count << 2);
}


// convert a Quaternion into another endian type
MCORE_INLINE void Endian::Convert16BitQuaternionTo(MCore::Compressed16BitQuaternion* value, EEndianType targetEndianType, uint32 count)
{
    // do nothing if we are already in the right endian
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
    if (targetEndianType == MCore::Endian::ENDIAN_LITTLE)
    {
        return;
    }
    #else
    if (targetEndianType == MCore::Endian::ENDIAN_BIG)
    {
        return;
    }
    #endif

    // perform conversion
    ConvertSignedInt16((int16*)value, count << 2);
}

