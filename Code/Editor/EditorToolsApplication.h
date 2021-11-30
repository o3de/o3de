/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include "Core/EditorMetricsPlainTextNameRegistration.h"
#include "EditorToolsApplicationAPI.h"

namespace EditorInternal
{
    // Override the ToolsApplication so that we can special case when the config file is not
    // found and give the user of the Editor a specific message about it.
    class EditorToolsApplication
        : public AzToolsFramework::ToolsApplication
        , public EditorToolsApplicationRequests::Bus::Handler
        , public AzToolsFramework::ViewportInteraction::EditorModifierKeyRequestBus::Handler
        , public AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Handler
    {
    public:
        EditorToolsApplication(int* argc, char*** argv);
        ~EditorToolsApplication();

        bool IsStartupAborted() const;

        void RegisterCoreComponents() override;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

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

        // EditorModifierKeyRequestBus overrides ...
        AzToolsFramework::ViewportInteraction::KeyboardModifiers QueryKeyboardModifiers() override;

        // EditorViewportInputTimeNowRequestBus overrides ...
        AZStd::chrono::milliseconds EditorViewportInputTimeNow() override;

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


