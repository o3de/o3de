/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    struct SlotMetadata
    {
        AZ_TYPE_INFO(SlotMetadata, "{315C9851-1747-4D68-9A4A-B699FA9FC754}");
        AZ_CLASS_ALLOCATOR(SlotMetadata, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflectContext);
        SlotId m_slotId;
        Data::Type m_dataType;
    };
}
