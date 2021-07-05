/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            Application(int argc, char** argv);
            ~Application() override = default;
            
            const char* GetConfigFilePath() const;
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
            void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;

        protected:
            void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;

            AZ::IO::FixedMaxPath m_configFilePath;
        };
    } // namespace SerializeContextTools
} // namespace AZ
