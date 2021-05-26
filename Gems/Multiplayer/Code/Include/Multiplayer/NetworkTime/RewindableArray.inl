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
