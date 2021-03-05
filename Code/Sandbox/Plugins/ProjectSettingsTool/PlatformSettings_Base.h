/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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

#include <AzCore/Serialization/EditContext.h>

namespace ProjectSettingsTool
{
    class BaseSettings
    {
    public:
        AZ_TYPE_INFO(BaseSettings, "{3202E013-46EC-4E97-989A-84934CA15C59}");
        AZ_CLASS_ALLOCATOR(BaseSettings, AZ::SystemAllocator, 0);

        BaseSettings()
            : m_projectName("")
            , m_productName("")
            , m_executableName("")
            , m_sysGameFolder("")
            , m_sysDllGame("")
            , m_projectOutputFolder("")
            , m_codeFolder("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_projectName;
        AZStd::string m_productName;
        AZStd::string m_executableName;
        AZStd::string m_sysGameFolder;
        AZStd::string m_sysDllGame;
        AZStd::string m_projectOutputFolder;
        AZStd::string m_codeFolder;
    };
} // namespace ProjectSettingsTool
