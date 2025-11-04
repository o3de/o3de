/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

namespace ScriptCanvas
{
    namespace Attributes
    {
        namespace Slot
        {
            const static AZ::Crc32 Type = AZ_CRC_CE("SlotType");
        }

        namespace Node
        {
            const static AZ::Crc32 TitlePaletteOverride = AZ_CRC_CE("TitlePaletteOverride");

            //! ScriptCanvas needs to know whether some nodes should be executed as soon as the graph is 
            //! activated. This is the case of the OnGraphStart event, but it's valid for any nodes
            //! that need to process without an explicit execution in signal.
            const static AZ::Crc32 GraphEntryPoint = AZ_CRC_CE("ScriptCanvasGraphEntryPoint");

            const static AZ::Crc32 NodeType = AZ_CRC_CE("ScriptCanvasNodeType");
        }

        namespace UIHandlers
        {
            const static AZ::Crc32 GenericLineEdit = AZ_CRC_CE("GenericLineEdit");
        }

        namespace NodePalette
        {
            // Attribute that can be used to store a function which creates a custom NodePaletteTreeItem
            const static AZ::Crc32 TreeItemOverride = AZ_CRC_CE("TreeItemOverride");
        }
        
        const static AZ::Crc32 StringToProperty = AZ_CRC_CE("StringToProperty");
        const static AZ::Crc32 PropertyToString = AZ_CRC_CE("PropertyToString");

        const static AZ::Crc32 Input = AZ_CRC_CE("Input");
        const static AZ::Crc32 Output = AZ_CRC_CE("Output");
        const static AZ::Crc32 Setter = AZ_CRC_CE("Setter");
        const static AZ::Crc32 Getter = AZ_CRC_CE("Getter");
        const static AZ::Crc32 AutoExpose = AZ_CRC_CE("AutoExpose");
        const static AZ::Crc32 Contract  = AZ_CRC_CE("Contract");

        const static AZ::Crc32 AllowSetterSlot  = AZ_CRC_CE("AllowSetterSlot");
        const static AZ::Crc32 AllowGetterSlot  = AZ_CRC_CE("AllowGetterSlot");
        const static AZ::Crc32 ShowSetterByDefault  = AZ_CRC_CE("ShowSetterByDefault");
        const static AZ::Crc32 ShowGetterByDefault  = AZ_CRC_CE("ShowGetterByDefault");
    }
}
