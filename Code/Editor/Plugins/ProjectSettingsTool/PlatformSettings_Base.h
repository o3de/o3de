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
    class BaseSettings
    {
    public:
        AZ_TYPE_INFO(BaseSettings, "{3202E013-46EC-4E97-989A-84934CA15C59}");
        AZ_CLASS_ALLOCATOR(BaseSettings, AZ::SystemAllocator);

        BaseSettings()
            : m_projectName("")
            , m_productName("")
            , m_executableName("")
            , m_projectPath("")
            , m_sysDllGame("")
            , m_projectOutputFolder("")
            , m_codeFolder("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_projectName;
        AZStd::string m_productName;
        AZStd::string m_executableName;
        AZStd::string m_projectPath;
        AZStd::string m_sysDllGame;
        AZStd::string m_projectOutputFolder;
        AZStd::string m_codeFolder;
    };
} // namespace ProjectSettingsTool
