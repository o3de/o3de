/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContext.h>

namespace ProjectSettingsTool
{
    enum class ValidationMode
    {
        Disabled,
        Enabled,
        Verbose,
        GPU
    };

    class WindowsGraphics
    {
    public:
        AZ_TYPE_INFO(WindowsGraphics, "{D9E6C2BD-0A10-4E36-B9E4-7F8F7D8F0C11}");
        AZ_CLASS_ALLOCATOR(WindowsGraphics, AZ::SystemAllocator);

        WindowsGraphics()
            : m_graphicsAPI("DX12")
            , m_validationMode(ValidationMode::Disabled)
        {
        }

        static void Reflect(AZ::ReflectContext* context);
        void SaveRHISettings(const AZ::IO::Path& rhiRegistryPath) const;
        static const char* ValidationModeToString(ValidationMode mode);

        AZStd::string m_graphicsAPI;
        ValidationMode m_validationMode;
    };

    class WindowsSettings
    {
    public:
        AZ_TYPE_INFO(WindowsSettings, "{9F84B3EE-D077-4693-86A0-C43AA455C90A}");
        AZ_CLASS_ALLOCATOR(WindowsSettings, AZ::SystemAllocator);

        WindowsSettings() = default;

        static void Reflect(AZ::ReflectContext* context);

        WindowsGraphics m_graphics;
    };
} // namespace ProjectSettingsTool
