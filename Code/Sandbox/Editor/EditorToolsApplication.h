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

#include "Core/EditorMetricsPlainTextNameRegistration.h"
#include "EditorToolsApplicationAPI.h"

namespace EditorInternal
{
    // Override the ToolsApplication so that we can special case when the config file is not
    // found and give the user of the Editor a specific message about it.
    class EditorToolsApplication
        : public AzToolsFramework::ToolsApplication
        , public EditorToolsApplicationRequests::Bus::Handler
    {
    public:
        EditorToolsApplication(int* argc, char*** argv);
        ~EditorToolsApplication();

        bool IsStartupAborted() const;

        void RegisterCoreComponents() override;

        AZ::ComponentTypeList GetRequiredSystemComponents() const;

        void StartCommon(AZ::Entity* systemEntity) override;

        using AzToolsFramework::ToolsApplication::Start;
        bool Start();

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::ApplicationRequests::Bus
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        // From AzToolsFramework::ToolsApplication
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;

    protected:
        // From EditorToolsApplicationRequests
        bool OpenLevel(AZStd::string_view levelName) override;
        bool OpenLevelNoPrompt(AZStd::string_view levelName) override;

        int CreateLevel(AZStd::string_view levelName, bool bUseTerrain) override;
        int CreateLevelNoPrompt(AZStd::string_view levelName, int terrainExportTextureSize, bool useTerrain) override;

        void Exit() override;
        void ExitNoPrompt() override;

        AZStd::string GetGameFolder() const override;
        AZStd::string GetCurrentLevelName() const override;
        AZStd::string GetCurrentLevelPath() const override;

        const char* GetOldCryLevelExtension() const override;
        const char* GetLevelExtension() const override;

    private:
        static constexpr char DefaultLevelFolder[] = "Levels";

        bool m_StartupAborted = false;
        Editor::EditorMetricsPlainTextNameRegistrationBusListener m_metricsPlainTextRegistrar;
    };

}


