/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the standard headers
#include <AzCore/Math/Quaternion.h>
#include "StandardHeaders.h"
#include "Vector.h"
#include "CompressedQuaternion.h"

namespace AZ
{
    class Vector2;
}

namespace MCore
{
    /**
     * The endian class is a collection of static functions that allow you to convert specific object types
     * into another endian conversion. This is useful when loading files from disk, which are stored in another endian
     * than the platform you are running your code on.
     */
    class MCORE_API Endian
    {
    public:
        /**
         * The supported endian conversion types.
         */
        enum EEndianType
        {
            ENDIAN_LITTLE   = 0,    /**< Least significant bits have lowest memory address */
            ENDIAN_BIG      = 1     /**< Most significant bits have lowest memory address */
        };

        /**
         * Swap the endian of one or more unsigned shorts.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertUnsignedInt16(uint16* value, uint32 count = 1);

        /**
         * Swap the endian of one or more unsigned ints.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertUnsignedInt32(uint32* value, uint32 count = 1);

        static MCORE_INLINE void ConvertUnsignedInt64(uint64* value, uint32 count = 1);

        /**
         * Swap the endian of one or more shorts.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertSignedInt16(int16* value, uint32 count = 1);

        /**
         * Swap the endian of one or more ints.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertSignedInt32(int32* value, uint32 count = 1);

        /**
         * Swap the endian of one or more floats.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertFloat(float* value, uint32 count = 1);

        /**
         * Swap the endian of one or more Vector2 objects.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertVector2(AZ::Vector2* value, uint32 count = 1);

        /**
         * Swap the endian of one or more PackedVector3f objects.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertVector3(AZ::Vector3* value, uint32 count = 1);

        /**
         * Swap the endian of one or more Vector4 objects.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertVector4(AZ::Vector4* value, uint32 count = 1);

        /**
         * Swap the endian of one or more Quaternion objects.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void ConvertQuaternion(AZ::Quaternion* value, uint32 count = 1);

        /**
         * Swap the endian of one or more 16-bit compressed Quaternion objects.
         * @param value The value to convert the endian for.
         * @param count The number of items to convert. Please note that the array specified by value must be large enough!
         */
        static MCORE_INLINE void Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, uint32 count = 1);

        /**
         * Swap two bytes values. This means that after executing this method, the memory address of byteA will
         * contain the value of the byte stored at memory address byteB, and the other way around.
         * @param byteA The memory address of byte number one.
         * @param byteB The memory address of byte number two.
         */
        static MCORE_INLINE void SwapBytes(uint8* byteA, uint8* byteB);

        /**
         * Switches the endian (little or big) of a set of 16 bit values.
         * This means the byte order gets inverted.
         * A byte order of (B1 B2) gets converted into (B2 B1).
         * @param buffer The input buffer that points to the data containing the values to be converted.
         * @param count The number of 16 bit values to convert.
         */
        static MCORE_INLINE void ConvertEndian16(uint8* buffer, uint32 count = 1);

        /**
         * Switches the endian (little or big) of a set of 32 bit values.
         * This means the byte order gets inverted.
         * A byte order of (B1 B2 B3 B4) gets converted into (B4 B3 B2 B1).
         * @param buffer The input buffer that points to the data containing the values to be converted.
         * @param count The number of 32 bit values to convert.
         */
        static MCORE_INLINE void ConvertEndian32(uint8* buffer, uint32 count = 1);

        /**
         * Switches the endian (little or big) of a set of 64 bit values.
         * This means the byte order gets inverted.
         * A byte order of (B1 B2 B3 B4 B5 B6 B7 B8) gets converted into (B8 B7 B6 B5 B4 B3 B2 B1).
         * @param buffer The input buffer that points to the data containing the values to be converted.
         * @param count The number of 64 bit values to convert.
         */
        static MCORE_INLINE void ConvertEndian64(uint8* buffer, uint32 count = 1);

        /**
         * Convert one or more 32 bit floating point values into the endian used by our current platform.
         * @param value The value(s) to convert. The number of values to follow at the specified address must be at least the number
         *              that you pass to the count parameter.
         * @param sourceEndianType The endian type where the float is currently stored in.
         * @param count The number of floats to convert.
         */
        static MCORE_INLINE void ConvertFloat(float* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert one or more 32 bit integer values into the endian used by our current platform.
         * @param value The value(s) to convert. The number of values to follow at the specified address must be at least the number
         *              that you pass to the count parameter.
         * @param sourceEndianType The endian type where the int32 is currently stored in.
         * @param count The number of integers to convert.
         */
        static MCORE_INLINE void ConvertSignedInt32(int32* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert one or more 32 bit unsigned integer values into the endian used by our current platform.
         * @param value The value(s) to convert. The number of values to follow at the specified address must be at least the number
         *              that you pass to the count parameter.
         * @param sourceEndianType The endian type where the int32 is currently stored in.
         * @param count The number of integers to convert.
         */
        static MCORE_INLINE void ConvertUnsignedInt32(uint32* value, EEndianType sourceEndianType, uint32 count = 1);

        static MCORE_INLINE void ConvertUnsignedInt64(uint64* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert one or more 16 bit short values into the endian used by our current platform.
         * @param value The value(s) to convert. The number of values to follow at the specified address must be at least the number
         *              that you pass to the count parameter.
         * @param sourceEndianType The endian type where the short is currently stored in.
         * @param count The number of shorts to convert.
         */
        static MCORE_INLINE void ConvertSignedInt16(int16* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert one or more 16 bit uint16 values into the endian used by our current platform.
         * @param value The value(s) to convert. The number of values to follow at the specified address must be at least the number
         *              that you pass to the count parameter.
         * @param sourceEndianType The endian type where the short is currently stored in.
         * @param count The number of shorts to convert.
         */
        static MCORE_INLINE void ConvertUnsignedInt16(uint16* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert a Vector2 object into the endian used by our current platform.
         * @param value The object to convert the endian for.
         * @param sourceEndianType The endian type where the object is currently stored in.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector2(AZ::Vector2* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
        * Convert a PackedVector3f object into the endian used by our current platform.
        * @param value The object to convert the endian for.
        * @param sourceEndianType The endian type where the object is currently stored in.
        * @param count The number of objects to convert. This allows conversion of arrays at once.
        */
        static MCORE_INLINE void ConvertVector3(AZ::Vector3* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert a Vector4 object into the endian used by our current platform.
         * @param value The object to convert the endian for.
         * @param sourceEndianType The endian type where the object is currently stored in.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector4(AZ::Vector4* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert a Quaternion object into the endian used by our current platform.
         * @param value The object to convert the endian for.
         * @param sourceEndianType The endian type where the object is currently stored in.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertQuaternion(AZ::Quaternion* value, EEndianType sourceEndianType, uint32 count = 1);

        /**
         * Convert a 16-bit compressed Quaternion object into the endian used by our current platform.
         * @param value The object to convert the endian for.
         * @param sourceEndianType The endian type where the object is currently stored in.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, EEndianType sourceEndianType, uint32 count = 1);


        /**
         * Convert a floating point value into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertFloat(float* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a 32 bit int32 into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertSignedInt32(int32* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a 32 bit uint32 into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertUnsignedInt32(uint32* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        static MCORE_INLINE void ConvertUnsignedInt64(uint64* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert an 16 bit short into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertSignedInt16(int16* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert an unsigned 16 bit short into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertUnsignedInt16(uint16* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Vector2 object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector2(AZ::Vector2* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Vector3 object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector3(AZ::Vector3* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Vector4 object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector4(AZ::Vector4* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Quaternion object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertQuaternion(AZ::Quaternion* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a 16-bit compressed Quaternion object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param sourceEndianType The endian type where the value is currently stored in, before conversion.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, EEndianType sourceEndianType, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a floating point value into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertFloatTo(float* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a 32 bit int32 into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertSignedInt32To(int32* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a 32 bit uint32 into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertUnsignedInt32To(uint32* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert an 16 bit short into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertSignedInt16To(int16* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert an unsigned 16 bit short into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertUnsignedInt16To(uint16* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Vector2 object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector2To(AZ::Vector2* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Vector3 object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector3To(AZ::Vector3* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Vector4 object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertVector4To(AZ::Vector4* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a Quaternion object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void ConvertQuaternionTo(AZ::Quaternion* value, EEndianType targetEndianType, uint32 count = 1);

        /**
         * Convert a 16-bit compressed Quaternion object into another endian type.
         * @param value A pointer to the object to convert/modify.
         * @param targetEndianType The endian type that the value should be converted into.
         * @param count The number of objects to convert. This allows conversion of arrays at once.
         */
        static MCORE_INLINE void Convert16BitQuaternionTo(MCore::Compressed16BitQuaternion* value, EEndianType targetEndianType, uint32 count = 1);
    };

    // include the inline code
#include "Endian.inl"
}   // namespace MCore
