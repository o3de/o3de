/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Crc.h>

namespace LegacyFramework
{
    namespace CustomMenusCommon
    {
        struct WorldEditor
        {
            static const AZ::Crc32 Application;
            static const AZ::Crc32 File;
            static const AZ::Crc32 Debug;
            static const AZ::Crc32 Edit;
            static const AZ::Crc32 Build;
        };
        struct LUAEditor
        {
            static const AZ::Crc32 Application;
            static const AZ::Crc32 File;
            static const AZ::Crc32 Edit;
            static const AZ::Crc32 View;
            static const AZ::Crc32 Debug;
            static const AZ::Crc32 SourceControl;
            static const AZ::Crc32 Options;
        };
        struct Viewport
        {
            static const AZ::Crc32 Layout;
            static const AZ::Crc32 Grid;
            static const AZ::Crc32 View;
        };
    } // namespace CustomMenusCommon
} // namespace LegacyFramework
