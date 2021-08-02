/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Multiplayer
{
    template <typename TYPE, uint32_t SIZE>
    bool RewindableArray<TYPE, SIZE>::Serialize(AzNetworking::ISerializer& serializer)
    {
        for (uint32_t i = 0; i < SIZE; ++i)
        {
            if(!this[i].Serialize(serializer))
            {
                return false;
            }
        }

        return serializer.IsValid();
    }

    template <typename TYPE, uint32_t SIZE>
    bool RewindableArray<TYPE, SIZE>::Serialize(AzNetworking::ISerializer& serializer, AzNetworking::IBitset& deltaRecord)
    {
        for (uint32_t i = 0; i < SIZE; ++i)
        {
            if (deltaRecord.GetBit(i))
            {
                serializer.ClearTrackedChangesFlag();
                if(!this[i].Serialize(serializer))
                {
                    return false;
                }

                if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && !serializer.GetTrackedChangesFlag())
                {
                    deltaRecord.SetBit(i, false);
                }
            }
        }

        return serializer.IsValid();
    }
}
