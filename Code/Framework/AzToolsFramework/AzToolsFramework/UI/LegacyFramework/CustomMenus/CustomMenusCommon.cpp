/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/LegacyFramework/CustomMenus/CustomMenusCommon.h>

namespace LegacyFramework
{
    namespace CustomMenusCommon
    {
        const AZ::Crc32 WorldEditor::Application = AZ_CRC_CE("World Editor");
        const AZ::Crc32 WorldEditor::File = AZ_CRC_CE("World Editor - File");
        const AZ::Crc32 WorldEditor::Debug = AZ_CRC_CE("World Editor - Debug");
        const AZ::Crc32 WorldEditor::Edit = AZ_CRC_CE("World Editor - Edit");
        const AZ::Crc32 WorldEditor::Build = AZ_CRC_CE("World Editor - Build");

        const AZ::Crc32 LUAEditor::Application = AZ_CRC_CE("LUAEditor");
        const AZ::Crc32 LUAEditor::File = AZ_CRC_CE("LUAEditor - File");
        const AZ::Crc32 LUAEditor::Edit = AZ_CRC_CE("LUAEditor - Edit");
        const AZ::Crc32 LUAEditor::View = AZ_CRC_CE("LUAEditor - View");
        const AZ::Crc32 LUAEditor::Debug = AZ_CRC_CE("LUAEditor - Debug");
        const AZ::Crc32 LUAEditor::SourceControl = AZ_CRC_CE("LUAEditor - SourceControl");
        const AZ::Crc32 LUAEditor::Options = AZ_CRC_CE("LUAEditor - Options");

        const AZ::Crc32 Viewport::Layout = AZ_CRC_CE("Viewport - Layout");
        const AZ::Crc32 Viewport::Grid = AZ_CRC_CE("Viewport - Grid");
        const AZ::Crc32 Viewport::View = AZ_CRC_CE("Viewport - View");
    } // namespace LegacyFramework
} // namespace CustomMenusCommon
