/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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

#include <AzCore/Math/Crc.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace ScriptCanvasEditor
{
    namespace SlotGroups
    {
        static const GraphCanvas::SlotGroup DynamicPropertiesGroup = AZ_CRC("ScriptCanvas_DynamicPropertiesGroup", 0x545b0552);
        static const GraphCanvas::SlotGroup EBusConnectionSlotGroup = AZ_CRC("ScriptCanvas_EBusConnectionSlotGroup", 0xc470f5c7);
    }

    namespace PropertySlotIds
    {
        static const AZ::Crc32 DefaultValue = AZ_CRC("ScriptCanvas_Property_DefaultValue", 0xf837b153);
    }
}