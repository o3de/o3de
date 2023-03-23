/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace CompressionLZ4
{
    // System Component TypeIds
    inline constexpr const char* CompressionLZ4SystemComponentTypeId = "{A98C22C1-119B-4015-BA82-F0F838A682D8}";
    inline constexpr const char* CompressionLZ4EditorSystemComponentTypeId = "{CA6A6069-FCA7-447B-8DB5-5A934ED00EBA}";

    // Module derived classes TypeIds
    inline constexpr const char* CompressionLZ4ModuleInterfaceTypeId = "{E4BF1468-95D6-4E9F-9142-C634C4EF89D4}";
    inline constexpr const char* CompressionLZ4ModuleTypeId = "{22D5D280-7E6A-43A6-B69B-386BD02AF4BD}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* CompressionLZ4EditorModuleTypeId = CompressionLZ4ModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* CompressionLZ4RequestsTypeId = "{509B82E3-0C06-4EF7-BFD4-041C955C4CDD}";
} // namespace CompressionLZ4
