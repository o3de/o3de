/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICAUTILS_H
#define GM_REPLICAUTILS_H

#include <AzCore/Math/Crc.h>

#include <GridMate/Serialize/Buffer.h>

#define GM_CRC_REPLICA_DATA 0

namespace GridMate
{
#if (GM_CRC_REPLICA_DATA)
    template<typename T>
    void SafeGuardRead(ReadBuffer* buffer, T function)
    {
        AZ::u32 size;
        AZ::u32 crc;

        if (!buffer->Read(size))
        {
            AZ_Assert(readSize == size, "Read the wrong amount");
            return;
        }
        if (!buffer->Read(crc))
        {
            AZ_Assert(readSize == size, "Read the wrong amount");
            return;
        }

        const char* currentPos = buffer->GetCurrent();

        AZ::Crc32 msgCrc(static_cast<const void*>(currentPos), size);
        AZ_Assert(static_cast<AZ::u32>(msgCrc) == crc, "CRC is wrong");

        function();

        AZ::u32 readSize = static_cast<AZ::u32>(buffer->GetCurrent() - currentPos);
        (void) readSize;
        AZ_Assert(readSize == size, "Read the wrong amount");
    }

    template<typename T>
    void SafeGuardWrite(WriteBuffer* buffer, T function)
    {
        auto sizeMarker = buffer->InsertMarker<AZ::u32>();
        auto crcMarker = buffer->InsertMarker<AZ::u32>();

        size_t oldSize = buffer->Size();

        function();

        size_t newSize = buffer->Size();

        AZ::Crc32 msgCrc(static_cast<const void*>(buffer->Get() + oldSize), newSize - oldSize);

        sizeMarker.SetData(static_cast<AZ::u32>(newSize - oldSize));
        crcMarker.SetData(msgCrc);
    }
#else
    template<typename T>
    void SafeGuardRead(ReadBuffer*, T function)
    {
        function();
    }

    template<typename T>
    void SafeGuardWrite(WriteBuffer*, T function)
    {
        function();
    }
#endif
}

#define GM_ENABLE_PROFILE_USER_CALLBACKS 1

#if (GM_ENABLE_PROFILE_USER_CALLBACKS)
#define GM_PROFILE_USER_CALLBACK(callback) AZ_PROFILE_SCOPE(GridMate, "GridMate User Code: %s", callback);
#else
#define GM_PROFILE_USER_CALLBACK(callback)
#endif

#endif // GM_REPLICAUTILS_H
