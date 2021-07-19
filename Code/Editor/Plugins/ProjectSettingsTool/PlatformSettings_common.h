/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>

#include "Utils.h"

namespace ProjectSettingsTool
{
    namespace Attributes
    {
        static const AZ::Crc32 FuncValidator = AZ_CRC("FuncValidator");
        static const AZ::Crc32 SelectFunction = AZ_CRC("SelectFunction");
        static const AZ::Crc32 LinkOptional = AZ_CRC("LinkOptional");
        static const AZ::Crc32 Obfuscated = AZ_CRC("ObfuscatedText");
        // Used as a tooltip and for distinguising linked properties
        static const AZ::Crc32 PropertyIdentfier = AZ_CRC("PropertyIdentfier");
        static const AZ::Crc32 LinkedProperty = AZ_CRC("LinkedProperty");
        static const AZ::Crc32 DefaultPath = AZ_CRC("DefaultPath");
        static const AZ::Crc32 DefaultImagePreview = AZ_CRC("DefaultImagePreview");
        static const AZ::Crc32 ObfuscatedText = AZ_CRC("ObfuscatedText");
        static const AZ::Crc32 ClearButton = AZ_CRC("ClearButton");
        static const AZ::Crc32 RemovableReadOnly = AZ_CRC("RemovableReadOnly");
    } // namespace Attributes

    namespace Handlers
    {
        static const AZ::Crc32 ImagePreview = AZ_CRC("ImagePreview");
        static const AZ::Crc32 LinkedLineEdit = AZ_CRC("LinkedLineEdit");
        static const AZ::Crc32 FileSelect = AZ_CRC("FileSelect");
        static const AZ::Crc32 QValidatedLineEdit = AZ_CRC("QValLineEdit");
        static const AZ::Crc32 QValidatedBrowseEdit = AZ_CRC("QValBrowseEdit");
    } // namespace Handlers

    namespace Identfiers
    {
        static const char* ProjectName = "Base - Project Name";
        static const char* ProductName = "Base - Product Name";
        static const char* ExecutableName = "Base - Executable Name";

        static const char* AndroidPackageName = "Android - Package Name";
        static const char* AndroidVersionName = "Android - Version Name";
        static const char* AndroidIconDefault = "Android - Icon Default";
        static const char* AndroidLandDefault = "Android - Land Default";
        static const char* AndroidPortDefault = "Android - Port Default";

        static const char* IosBundleName = "iOS - Bundle Name";
        static const char* IosDisplayName = "iOS - Display Name";
        static const char* IosExecutableName = "iOS - Executable Name";
        static const char* IosBundleIdentifer = "iOS - Bundle Identifer";
        static const char* IosVersionName = "iOS - Version Name";
    } // namespace Identfiers
} // namespace ProjectSettingsTool
