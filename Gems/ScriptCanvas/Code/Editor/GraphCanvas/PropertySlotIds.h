/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace ScriptCanvasEditor
{
    namespace SlotGroups
    {
        static const GraphCanvas::SlotGroup DynamicPropertiesGroup = AZ_CRC_CE("ScriptCanvas_DynamicPropertiesGroup");
        static const GraphCanvas::SlotGroup EBusConnectionSlotGroup = AZ_CRC_CE("ScriptCanvas_EBusConnectionSlotGroup");
    }

    namespace PropertySlotIds
    {
        static const AZ::Crc32 DefaultValue = AZ_CRC_CE("ScriptCanvas_Property_DefaultValue");
    }
}
