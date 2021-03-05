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

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        class Application final
            : public AzToolsFramework::ToolsApplication
        {
        public:
            Application(int* argc, char*** argv);
            ~Application() override = default;
            
            const char* GetConfigFilePath() const;

        protected:
            void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;

            AZ::IO::FixedMaxPath m_configFilePath;
        };
    } // namespace SerializeContextTools
} // namespace AZ
