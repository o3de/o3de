/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/Attribute.h>
#include <EMotionFX/Source/AnimGraphNetworkSerializer.h>


namespace EMotionFX
{
    namespace Network
    {
        void AnimGraphSnapshotChunkSerializer::Serialize(MCore::Attribute& attribute, const char* context)
        {
            AZ_UNUSED(context);
            attribute.NetworkSerialize(*this);
        }
    }
}
