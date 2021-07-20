/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace ScriptCanvasEditor
{
    namespace AutomationIds
    {
        static const AZ::Crc32 NodePaletteDockWidget = AZ_CRC("SC_NodePaletteDockWidget", 0xc86d3ceb);
        static const AZ::Crc32 NodePaletteWidget = AZ_CRC("SC_NodePaletteWidget", 0x8c80dbb1);

        static const AZ::Crc32 CreateScriptCanvasButton = AZ_CRC("SC_CreateRuntimeScriptCanvas", 0x07075136);
        static const AZ::Crc32 CreateScriptCanvasFunctionButton = AZ_CRC("SC_CreateScriptCanvasFunction", 0x3b3be68a);
    }
}
