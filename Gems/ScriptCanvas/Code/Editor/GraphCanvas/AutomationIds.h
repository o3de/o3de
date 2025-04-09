/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        static const AZ::Crc32 NodePaletteDockWidget = AZ_CRC_CE("SC_NodePaletteDockWidget");
        static const AZ::Crc32 NodePaletteWidget = AZ_CRC_CE("SC_NodePaletteWidget");

        static const AZ::Crc32 CreateScriptCanvasButton = AZ_CRC_CE("SC_CreateRuntimeScriptCanvas");
        static const AZ::Crc32 CreateScriptCanvasFunctionButton = AZ_CRC_CE("SC_CreateScriptCanvasFunction");
    }
}
