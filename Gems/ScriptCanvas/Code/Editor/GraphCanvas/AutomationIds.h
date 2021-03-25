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
